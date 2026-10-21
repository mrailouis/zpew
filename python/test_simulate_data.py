import datetime as dt
import hashlib
import json
import random
import tempfile
import unittest
from pathlib import Path

import simulate_data
from simulate_data import SPECS


class SimulateDataTest(unittest.TestCase):
    def test_same_seed_gives_identical_files(self) -> None:
        with tempfile.TemporaryDirectory() as a, tempfile.TemporaryDirectory() as b:
            simulate_data.generate(Path(a), 7)
            simulate_data.generate(Path(b), 7)
            for spec in SPECS:
                name = f"snapshots/{spec.symbol}.csv"
                self.assertEqual((Path(a) / name).read_bytes(), (Path(b) / name).read_bytes())

    def test_different_seeds_give_different_paths(self) -> None:
        first = simulate_data.simulate(SPECS[0], random.Random(1))
        second = simulate_data.simulate(SPECS[0], random.Random(2))
        self.assertNotEqual(first[-1].close, second[-1].close)

    def test_bars_respect_ohlc_ordering_and_positivity(self) -> None:
        for spec in SPECS:
            with self.subTest(symbol=spec.symbol):
                for bar in simulate_data.simulate(spec, random.Random(3)):
                    self.assertGreater(bar.low, 0.0)
                    self.assertLessEqual(bar.low, min(bar.open, bar.close))
                    self.assertGreaterEqual(bar.high, max(bar.open, bar.close))
                    self.assertGreaterEqual(bar.volume, 0)

    def test_weekday_calendar_skips_weekends_and_holidays(self) -> None:
        dates = simulate_data.session_dates(SPECS[1])
        self.assertTrue(all(d.weekday() < 5 for d in dates))
        self.assertNotIn(dt.date(2024, 12, 25), dates)
        self.assertEqual(dates, sorted(set(dates)))

    def test_daily_calendar_has_every_day(self) -> None:
        crypto = next(s for s in SPECS if s.calendar == "daily")
        dates = simulate_data.session_dates(crypto)
        self.assertEqual((dates[-1] - dates[0]).days + 1, len(dates))

    def test_outage_removes_exactly_its_sessions(self) -> None:
        spec = SPECS[0]
        assert spec.outage is not None
        first, count = spec.outage
        dates = set(simulate_data.session_dates(spec))
        missing = [first + dt.timedelta(days=i) for i in range(count)]
        self.assertTrue(all(d.weekday() < 5 for d in missing))
        self.assertTrue(dates.isdisjoint(missing))
        self.assertIn(first - dt.timedelta(days=3), dates)

    def test_manifest_hashes_and_counts_match_the_files(self) -> None:
        with tempfile.TemporaryDirectory() as out:
            manifest = simulate_data.generate(Path(out), 11)
            self.assertEqual(manifest, json.loads((Path(out) / "manifest.json").read_text()))
            for entry in manifest["assets"]:
                data = (Path(out) / entry["file"]).read_bytes()
                self.assertEqual(entry["sha256"], hashlib.sha256(data).hexdigest())
                self.assertEqual(entry["rows"], data.count(b"\n") - 1)
                self.assertEqual(entry["source"], "simulated")
                self.assertTrue(entry["name"].startswith("Simulated"))


if __name__ == "__main__":
    unittest.main()
