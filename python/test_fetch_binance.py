import datetime as dt
import hashlib
import io
import tempfile
import unittest
import zipfile
from pathlib import Path

import fetch_binance
from fetch_binance import Bar, FetchError

BTC_2017_08_17_MS = 1502928000000
ETH_2025_01_01_US = 1735689600000000


def bar(day: dt.date, **overrides: float) -> Bar:
    values = {"open": 100.0, "high": 102.0, "low": 99.0, "close": 101.0, "volume": 1000.0}
    values.update(overrides)
    return Bar(date=day, **values)


def days(start: dt.date, count: int) -> list[Bar]:
    return [bar(start + dt.timedelta(days=i)) for i in range(count)]


def kline_zip(text: str, name: str = "BTCUSDT-1d-2024-01.csv") -> bytes:
    stream = io.BytesIO()
    with zipfile.ZipFile(stream, "w") as archive:
        archive.writestr(name, text)
    return stream.getvalue()


class ExclusionTest(unittest.TestCase):
    def test_pegged_and_leveraged_tokens_are_excluded(self) -> None:
        for symbol in ("USDCUSDT", "TUSDUSDT", "EURUSDT", "PAXGUSDT", "FDUSDUSDT"):
            with self.subTest(symbol=symbol):
                self.assertEqual(fetch_binance.exclusion_reason(symbol), "pegged token")
        for symbol in ("BTCUPUSDT", "ETHDOWNUSDT", "BNBBULLUSDT", "XRPBEARUSDT"):
            with self.subTest(symbol=symbol):
                self.assertEqual(fetch_binance.exclusion_reason(symbol), "leveraged token")

    def test_ordinary_coins_and_lookalike_names_are_kept(self) -> None:
        for symbol in ("BTCUSDT", "JUPUSDT", "SUPERUSDT", "UNIUSDT"):
            with self.subTest(symbol=symbol):
                self.assertEqual(fetch_binance.exclusion_reason(symbol), "")

    def test_rejects_non_usdt_symbols(self) -> None:
        for symbol in ("BTCBUSD", "USDT"):
            with self.subTest(symbol=symbol), self.assertRaises(FetchError):
                fetch_binance.base_asset(symbol)


class ListingTest(unittest.TestCase):
    def test_month_key_matches_only_the_symbols_daily_zips(self) -> None:
        prefix = "data/spot/monthly/klines/BTCUSDT/1d/"
        self.assertEqual(fetch_binance.month_key(prefix + "BTCUSDT-1d-2017-08.zip", "BTCUSDT"),
                         "2017-08")
        for key in (prefix + "BTCUSDT-1d-2017-08.zip.CHECKSUM",
                    prefix + "ETHUSDT-1d-2017-08.zip",
                    "data/spot/monthly/klines/BTCUSDT/1h/BTCUSDT-1h-2017-08.zip"):
            with self.subTest(key=key):
                self.assertIsNone(fetch_binance.month_key(key, "BTCUSDT"))

    def test_months_through_drops_months_after_the_end(self) -> None:
        months = ["2026-10", "2026-08", "2026-09"]
        self.assertEqual(fetch_binance.months_through(months, dt.date(2026, 9, 30)),
                         ["2026-08", "2026-09"])

    def test_could_qualify_needs_enough_months(self) -> None:
        self.assertFalse(fetch_binance.could_qualify(["m"] * 64))
        self.assertTrue(fetch_binance.could_qualify(["m"] * 65))


class ChecksumTest(unittest.TestCase):
    def test_parses_binance_checksum_files(self) -> None:
        digest = "a" * 64
        text = f"{digest}  BTCUSDT-1d-2024-01.zip\n"
        self.assertEqual(fetch_binance.parse_checksum(text, "BTCUSDT-1d-2024-01.zip"), digest)

    def test_rejects_wrong_names_and_malformed_digests(self) -> None:
        for text in (f"{'a' * 64}  ETHUSDT-1d-2024-01.zip", "abc  BTCUSDT-1d-2024-01.zip", ""):
            with self.subTest(text=text), self.assertRaises(FetchError):
                fetch_binance.parse_checksum(text, "BTCUSDT-1d-2024-01.zip")


class TimestampTest(unittest.TestCase):
    def test_reads_millisecond_and_microsecond_open_times(self) -> None:
        self.assertEqual(fetch_binance.open_time_to_date(str(BTC_2017_08_17_MS), "BTCUSDT"),
                         dt.date(2017, 8, 17))
        self.assertEqual(fetch_binance.open_time_to_date(str(ETH_2025_01_01_US), "ETHUSDT"),
                         dt.date(2025, 1, 1))

    def test_rejects_open_times_that_are_not_midnight_utc(self) -> None:
        for raw in (str(BTC_2017_08_17_MS + 3_600_000), str(ETH_2025_01_01_US + 1000)):
            with self.subTest(raw=raw), self.assertRaises(FetchError):
                fetch_binance.open_time_to_date(raw, "BTCUSDT")


class KlineParsingTest(unittest.TestCase):
    ROW = f"{BTC_2017_08_17_MS},4261.48,4485.39,4200.74,4285.08,795.150377,1503014399999,0,0,0,0,0"

    def test_parses_ohlcv_fields(self) -> None:
        bars = fetch_binance.parse_klines(self.ROW + "\n", "BTCUSDT")
        self.assertEqual(bars, [Bar(dt.date(2017, 8, 17), 4261.48, 4485.39, 4200.74, 4285.08,
                                    795.150377)])

    def test_skips_a_header_row_and_accepts_crlf(self) -> None:
        text = "open_time,open,high,low,close,volume\r\n" + self.ROW + "\r\n"
        self.assertEqual(len(fetch_binance.parse_klines(text, "BTCUSDT")), 1)

    def test_rejects_short_rows(self) -> None:
        with self.assertRaises(FetchError):
            fetch_binance.parse_klines(f"{BTC_2017_08_17_MS},1,2,3\n", "BTCUSDT")

    def test_reads_the_single_csv_in_an_archive(self) -> None:
        self.assertEqual(len(fetch_binance.read_kline_zip(kline_zip(self.ROW), "BTCUSDT")), 1)

    def test_rejects_archives_without_exactly_one_csv(self) -> None:
        stream = io.BytesIO()
        with zipfile.ZipFile(stream, "w") as archive:
            archive.writestr("a.csv", self.ROW)
            archive.writestr("b.csv", self.ROW)
        with self.assertRaises(FetchError):
            fetch_binance.read_kline_zip(stream.getvalue(), "BTCUSDT")


class ContiguityTest(unittest.TestCase):
    def test_keeps_everything_without_gaps(self) -> None:
        bars = days(dt.date(2024, 1, 1), 10)
        self.assertEqual(fetch_binance.latest_contiguous_run(bars), bars)

    def test_keeps_only_the_run_after_the_last_gap(self) -> None:
        early = days(dt.date(2024, 1, 1), 5)
        late = days(dt.date(2024, 1, 8), 4)
        self.assertEqual(fetch_binance.latest_contiguous_run(early + late), late)

    def test_handles_empty_and_single_bar_input(self) -> None:
        self.assertEqual(fetch_binance.latest_contiguous_run([]), [])
        single = days(dt.date(2024, 1, 1), 1)
        self.assertEqual(fetch_binance.latest_contiguous_run(single), single)


class ValidationTest(unittest.TestCase):
    def test_accepts_valid_bars(self) -> None:
        self.assertEqual(fetch_binance.validate_bars(days(dt.date(2024, 1, 1), 3), "X"), [])

    def test_reports_each_broken_rule(self) -> None:
        day = dt.date(2024, 1, 1)
        cases = {
            "not finite and positive": [bar(day, open=0.0), bar(day + dt.timedelta(days=1))],
            "volume": [bar(day, volume=-1.0), bar(day + dt.timedelta(days=1))],
            "below max": [bar(day, high=100.5), bar(day + dt.timedelta(days=1))],
            "above min": [bar(day, low=100.5), bar(day + dt.timedelta(days=1))],
            "not after": [bar(day), bar(day)],
            "only 1 bars": [bar(day)],
        }
        for expected, bars in cases.items():
            with self.subTest(rule=expected):
                problems = fetch_binance.validate_bars(bars, "X")
                self.assertTrue(any(expected in p for p in problems), problems)


class SelectionTest(unittest.TestCase):
    END = dt.date(2026, 9, 30)

    def test_includes_a_long_enough_contiguous_history(self) -> None:
        bars = days(dt.date(2020, 1, 1), fetch_binance.MIN_BARS)
        selection = fetch_binance.select("BTCUSDT", bars, self.END)
        self.assertEqual((selection.status, selection.reason), ("included", ""))
        self.assertEqual(len(selection.bars), fetch_binance.MIN_BARS)

    def test_excludes_short_histories(self) -> None:
        bars = days(dt.date(2020, 1, 1), fetch_binance.MIN_BARS - 1)
        selection = fetch_binance.select("BTCUSDT", bars, self.END)
        self.assertEqual(selection.status, "excluded")

    def test_drops_bars_after_the_end_date(self) -> None:
        bars = days(self.END - dt.timedelta(days=fetch_binance.MIN_BARS - 1),
                    fetch_binance.MIN_BARS + 5)
        selection = fetch_binance.select("BTCUSDT", bars, self.END)
        self.assertEqual(selection.bars[-1].date, self.END)

    def test_a_gap_drops_the_earlier_segment_and_is_recorded(self) -> None:
        early = days(dt.date(2015, 1, 1), 100)
        late = days(dt.date(2016, 1, 1), fetch_binance.MIN_BARS)
        selection = fetch_binance.select("LUNAUSDT", early + late, self.END)
        self.assertEqual(selection.status, "included")
        self.assertEqual(selection.bars[0].date, dt.date(2016, 1, 1))
        self.assertIn("dropped 100", selection.reason)

    def test_a_gap_can_leave_too_few_bars(self) -> None:
        early = days(dt.date(2015, 1, 1), fetch_binance.MIN_BARS)
        late = days(dt.date(2025, 1, 1), 10)
        selection = fetch_binance.select("LUNAUSDT", early + late, self.END)
        self.assertEqual(selection.status, "excluded")
        self.assertIn("gap", selection.reason)

    def test_excluded_tokens_are_not_validated(self) -> None:
        selection = fetch_binance.select("USDCUSDT", [], self.END)
        self.assertEqual((selection.status, selection.reason), ("excluded", "pegged token"))

    def test_identical_duplicate_rows_are_removed_and_recorded(self) -> None:
        bars = days(dt.date(2020, 1, 1), fetch_binance.MIN_BARS)
        bars.insert(10, bars[10])
        selection = fetch_binance.select("AXSUSDT", bars, self.END)
        self.assertEqual(selection.status, "included")
        self.assertEqual(len(selection.bars), fetch_binance.MIN_BARS)
        self.assertIn("1 identical duplicate rows removed", selection.reason)

    def test_conflicting_duplicate_dates_fail_loudly(self) -> None:
        bars = days(dt.date(2020, 1, 1), fetch_binance.MIN_BARS)
        bars.insert(10, bar(bars[10].date, close=100.5))
        with self.assertRaises(FetchError):
            fetch_binance.select("AXSUSDT", bars, self.END)

    def test_invalid_bars_fail_loudly(self) -> None:
        bars = days(dt.date(2020, 1, 1), fetch_binance.MIN_BARS)
        bars[5] = bar(bars[5].date, high=50.0)
        with self.assertRaises(FetchError):
            fetch_binance.select("BTCUSDT", bars, self.END)


class OutputTest(unittest.TestCase):
    def test_csv_matches_the_snapshot_format(self) -> None:
        data = fetch_binance.to_csv([bar(dt.date(2024, 1, 2), volume=0.5)])
        self.assertEqual(data, b"date,open,high,low,close,volume\n2024-01-02,100,102,99,101,0.5\n")

    def test_manifest_entry_records_provenance(self) -> None:
        bars = tuple(days(dt.date(2024, 1, 1), 3))
        entry = fetch_binance.manifest_entry("BTCUSDT", bars, "snapshots/BTCUSDT.csv", "f" * 64,
                                             "2026-10-26T00:00:00Z")
        self.assertEqual(entry["name"], "BTC / USDT")
        self.assertEqual(entry["calendar"], "daily")
        self.assertEqual(entry["source"], "binance_vision")
        self.assertEqual((entry["first_date"], entry["last_date"], entry["rows"]),
                         ("2024-01-01", "2024-01-03", 3))
        self.assertIn("BTCUSDT/1d/", entry["source_url"])

    def test_universe_rows_record_excluded_pairs(self) -> None:
        selections = [
            fetch_binance.Selection("BTCUSDT", "included", "", tuple(days(dt.date(2024, 1, 1), 2))),
            fetch_binance.Selection("USDCUSDT", "excluded", "pegged token"),
        ]
        self.assertEqual(fetch_binance.universe_rows(selections), [
            ["BTCUSDT", "included", "", "2024-01-01", "2024-01-02", "2"],
            ["USDCUSDT", "excluded", "pegged token", "", "", ""],
        ])


class CachedDownloadTest(unittest.TestCase):
    def test_uses_a_cached_archive_that_matches_the_checksum(self) -> None:
        archive = kline_zip(KlineParsingTest.ROW)
        digest = hashlib.sha256(archive).hexdigest()
        name = "BTCUSDT-1d-2017-08.zip"
        with tempfile.TemporaryDirectory() as directory:
            cache = Path(directory)
            (cache / "BTCUSDT").mkdir()
            (cache / "BTCUSDT" / name).write_bytes(archive)
            client = fetch_binance.BinanceVisionClient(cache_dir=cache)
            requested: list[str] = []

            def get(url: str) -> bytes:
                requested.append(url)
                return f"{digest}  {name}\n".encode("ascii")

            client.get = get
            self.assertEqual(client.month_zip("BTCUSDT", "2017-08"), archive)
            self.assertEqual(len(requested), 1)

    def test_rejects_a_download_that_does_not_match_the_checksum(self) -> None:
        name = "BTCUSDT-1d-2017-08.zip"
        with tempfile.TemporaryDirectory() as directory:
            client = fetch_binance.BinanceVisionClient(cache_dir=Path(directory))

            def get(url: str) -> bytes:
                if url.endswith(".CHECKSUM"):
                    return f"{'0' * 64}  {name}\n".encode("ascii")
                return b"tampered"

            client.get = get
            with self.assertRaises(FetchError):
                client.month_zip("BTCUSDT", "2017-08")


if __name__ == "__main__":
    unittest.main()
