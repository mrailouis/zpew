import datetime as dt
import hashlib
import unittest

import fetch_data
from fetch_data import Bar, FetchError


def bar(day: int, **overrides: float) -> Bar:
    values = {"open": 100.0, "high": 102.0, "low": 99.0, "close": 101.0, "volume": 1000.0}
    values.update(overrides)
    return Bar(date=dt.date(2024, 1, day), **values)


class ParseEnvFileTest(unittest.TestCase):
    def test_reads_keys_and_ignores_comments_and_blank_lines(self) -> None:
        text = "# comment\n\nTIINGO_API_KEY=abc123\nexport OTHER = 'x y'\nQUOTED=\"z\"\n"
        self.assertEqual(fetch_data.parse_env_file(text),
                         {"TIINGO_API_KEY": "abc123", "OTHER": "x y", "QUOTED": "z"})

    def test_rejects_lines_without_equals(self) -> None:
        with self.assertRaises(FetchError):
            fetch_data.parse_env_file("TIINGO_API_KEY\n")


class TimestampTest(unittest.TestCase):
    def test_accepts_midnight_utc(self) -> None:
        self.assertEqual(fetch_data.parse_midnight_utc("2024-01-05T00:00:00.000Z", "SPY"),
                         dt.date(2024, 1, 5))
        self.assertEqual(fetch_data.parse_midnight_utc("2024-01-05T00:00:00+00:00", "BTCUSD"),
                         dt.date(2024, 1, 5))

    def test_rejects_other_times_and_offsets(self) -> None:
        for timestamp in ("2024-01-05T05:00:00.000Z", "2024-01-05T00:00:00-05:00"):
            with self.subTest(timestamp=timestamp), self.assertRaises(FetchError):
                fetch_data.parse_midnight_utc(timestamp, "SPY")


class ConversionTest(unittest.TestCase):
    def test_daily_rows_use_adjusted_fields(self) -> None:
        rows = [{"date": "2024-01-05T00:00:00.000Z", "open": 1, "high": 1, "low": 1, "close": 1,
                 "volume": 1, "adjOpen": 10.5, "adjHigh": 11, "adjLow": 10, "adjClose": 10.75,
                 "adjVolume": 300}]
        self.assertEqual(fetch_data.bars_from_daily(rows, "SPY"),
                         [Bar(dt.date(2024, 1, 5), 10.5, 11.0, 10.0, 10.75, 300.0)])

    def test_crypto_payload_reads_price_data(self) -> None:
        payload = [{"ticker": "btcusd", "priceData": [
            {"date": "2024-01-06T00:00:00+00:00", "open": 1, "high": 2, "low": 0.5,
             "close": 1.5, "volume": 7, "volumeNotional": 10.5, "tradesDone": 3}]}]
        self.assertEqual(fetch_data.bars_from_crypto(payload, "BTCUSD"),
                         [Bar(dt.date(2024, 1, 6), 1.0, 2.0, 0.5, 1.5, 7.0)])

    def test_crypto_payload_must_hold_one_series(self) -> None:
        with self.assertRaises(FetchError):
            fetch_data.bars_from_crypto([], "BTCUSD")


class ValidationTest(unittest.TestCase):
    def test_good_bars_pass(self) -> None:
        self.assertEqual(fetch_data.validate_bars([bar(4), bar(5)], "weekdays", "SPY"), [])

    def test_each_rule_is_reported(self) -> None:
        cases = {
            "only 1 bars": [bar(4)],
            "close 0.0 is not finite and positive": [bar(4), bar(5, close=0.0, low=0.0)],
            "volume -1.0": [bar(4), bar(5, volume=-1.0)],
            "high 100.5 below max(open, close)": [bar(4), bar(5, high=100.5)],
            "low 100.5 above min(open, close)": [bar(4), bar(5, low=100.5)],
            "weekend bar": [bar(5), bar(6)],
            "date not after 2024-01-05": [bar(5), bar(5)],
            "high nan is not finite and positive": [bar(4), bar(5, high=float("nan"))],
        }
        for expected, bars in cases.items():
            with self.subTest(expected=expected):
                problems = fetch_data.validate_bars(bars, "weekdays", "SPY")
                self.assertTrue(any(expected in p for p in problems), problems)

    def test_weekend_bars_are_fine_on_the_daily_calendar(self) -> None:
        self.assertEqual(fetch_data.validate_bars([bar(6), bar(7)], "daily", "BTCUSD"), [])


class CsvTest(unittest.TestCase):
    def test_matches_the_snapshot_format_exactly(self) -> None:
        bars = [bar(4, open=100.25, volume=1200.0), bar(5, close=1e-05, low=1e-05, open=0.5,
                                                         high=0.5, volume=0.0)]
        self.assertEqual(fetch_data.to_csv(bars),
                         b"date,open,high,low,close,volume\n"
                         b"2024-01-04,100.25,102,99,101,1200\n"
                         b"2024-01-05,0.5,0.5,1e-05,1e-05,0\n")

    def test_numbers_round_trip_exactly(self) -> None:
        for value in (0.1, 123.456789012345, 1e-300, 1.7976931348623157e308, 3.0):
            with self.subTest(value=value):
                self.assertEqual(float(fetch_data.format_number(value)), value)

    def test_manifest_hash_is_the_sha256_of_the_written_bytes(self) -> None:
        data = fetch_data.to_csv([bar(4), bar(5)])
        entry = fetch_data.manifest_entry(
            fetch_data.Asset("SPY", "spy", "daily", "us_equity_broad", "weekdays", "S&P 500"),
            "https://api.tiingo.com/x", [bar(4), bar(5)], "snapshots/SPY.csv",
            hashlib.sha256(data).hexdigest())
        self.assertEqual(entry["rows"], 2)
        self.assertEqual(entry["first_date"], "2024-01-04")
        self.assertEqual(entry["last_date"], "2024-01-05")
        self.assertRegex(entry["fetched_at"], r"^\d{4}-\d\d-\d\dT\d\d:\d\d:\d\dZ$")
        self.assertEqual(len(entry["sha256"]), 64)


class RequestUrlTest(unittest.TestCase):
    def test_urls_never_contain_the_api_key(self) -> None:
        asset = fetch_data.Asset("BTCUSD", "btcusd", "crypto", "crypto", "daily", "Bitcoin")
        url = fetch_data.request_url(asset, dt.date(2010, 1, 1), dt.date(2026, 9, 30))
        self.assertEqual(url, "https://api.tiingo.com/tiingo/crypto/prices?tickers=btcusd"
                              "&startDate=2010-01-01&endDate=2026-09-30&resampleFreq=1day")
        self.assertNotIn("token", url.lower())


class PaginationTest(unittest.TestCase):
    def test_follows_truncated_pages_until_the_end_date(self) -> None:
        calls: list[tuple[dt.date, dt.date]] = []
        pages = {dt.date(2024, 1, 1): [bar(1), bar(2)], dt.date(2024, 1, 3): [bar(3), bar(4)]}

        def fetch(start: dt.date, end: dt.date) -> list[Bar]:
            calls.append((start, end))
            return pages.get(start, [])

        bars = fetch_data.fetch_bars(None, None, dt.date(2024, 1, 1), dt.date(2024, 1, 4), fetch)
        self.assertEqual([b.date.day for b in bars], [1, 2, 3, 4])
        self.assertEqual(calls, [(dt.date(2024, 1, 1), dt.date(2024, 1, 4)),
                                 (dt.date(2024, 1, 3), dt.date(2024, 1, 4))])

    def test_stops_on_an_empty_page_for_series_that_end_early(self) -> None:
        calls: list[dt.date] = []

        def fetch(start: dt.date, end: dt.date) -> list[Bar]:
            calls.append(start)
            return [bar(1), bar(2)] if start == dt.date(2024, 1, 1) else []

        bars = fetch_data.fetch_bars(None, None, dt.date(2024, 1, 1), dt.date(2024, 1, 31), fetch)
        self.assertEqual(len(bars), 2)
        self.assertEqual(calls, [dt.date(2024, 1, 1), dt.date(2024, 1, 3)])


class WindowedPaginationTest(unittest.TestCase):
    def test_skips_empty_windows_before_the_series_begins(self) -> None:
        calls: list[tuple[dt.date, dt.date]] = []
        data = {dt.date(2024, 1, d): bar(d) for d in (8, 9, 10, 11)}

        def fetch(start: dt.date, end: dt.date) -> list[Bar]:
            calls.append((start, end))
            return [b for day, b in sorted(data.items()) if start <= day <= end]

        bars = fetch_data.fetch_bars(None, None, dt.date(2024, 1, 1), dt.date(2024, 1, 11),
                                     fetch, window_days=3)
        self.assertEqual([b.date.day for b in bars], [8, 9, 10, 11])
        self.assertEqual(calls[0], (dt.date(2024, 1, 1), dt.date(2024, 1, 3)))
        self.assertEqual(calls[1], (dt.date(2024, 1, 4), dt.date(2024, 1, 6)))

    def test_a_series_that_never_starts_returns_nothing(self) -> None:
        bars = fetch_data.fetch_bars(None, None, dt.date(2024, 1, 1), dt.date(2024, 1, 20),
                                     lambda s, e: [], window_days=7)
        self.assertEqual(bars, [])


class RequestPacerTest(unittest.TestCase):
    def test_spaces_requests_to_the_hourly_limit(self) -> None:
        now = [0.0]
        sleeps: list[float] = []

        def sleep(seconds: float) -> None:
            sleeps.append(seconds)
            now[0] += seconds

        pacer = fetch_data.RequestPacer(per_hour=3, per_day=100, clock=lambda: now[0], sleep=sleep)
        for _ in range(3):
            self.assertEqual(pacer.wait_for_slot(), 0.0)
        self.assertEqual(pacer.wait_for_slot(), 3600.0)
        self.assertEqual(sleeps, [3600.0])

    def test_respects_the_daily_limit(self) -> None:
        now = [0.0]
        pacer = fetch_data.RequestPacer(per_hour=100, per_day=2, clock=lambda: now[0],
                                        sleep=lambda s: now.__setitem__(0, now[0] + s))
        pacer.wait_for_slot()
        pacer.wait_for_slot()
        self.assertEqual(pacer.wait_for_slot(), 86400.0)


class UniverseTest(unittest.TestCase):
    def test_committed_universe_is_well_formed(self) -> None:
        universe = fetch_data.read_universe()
        self.assertGreaterEqual(len(universe), 100)
        self.assertTrue(all(a.calendar == "daily" for a in universe if a.endpoint == "crypto"))
        self.assertTrue(all(a.calendar == "weekdays" for a in universe if a.endpoint == "daily"))


if __name__ == "__main__":
    unittest.main()
