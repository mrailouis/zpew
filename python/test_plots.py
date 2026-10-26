import csv
import tempfile
import unittest
import xml.etree.ElementTree as ElementTree
from pathlib import Path

import plots
from plots import PlotError

SVG = "{http://www.w3.org/2000/svg}"


def write_csv(path: Path, header: list[str], rows: list[list[object]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.writer(stream)
        writer.writerow(header)
        writer.writerows(rows)


def write_run(root: Path) -> None:
    improvements = []
    for symbol, offset in (("AUSDT", 0.0), ("BUSDT", 5.0), ("CUSDT", -2.0)):
        for model, base in (("rv63", 1.0), ("ewma", 4.0), ("garch", 8.0)):
            for proxy, scale in (("squared_return", 1.0), ("parkinson", 3.0)):
                for mode in ("sliding", "expanding"):
                    improvements.append([symbol, mode, model, proxy, 0.5, (base + offset) * scale])
    write_csv(root / "eval" / "improvements.csv",
              ["symbol", "window_mode", "model", "proxy", "mse_vs_baseline_pct",
               "qlike_vs_baseline_pct"], improvements)
    backtest = []
    for symbol, rate in (("AUSDT", 0.04), ("BUSDT", 0.05), ("CUSDT", 0.06)):
        for model in ("rv21", "rv63", "ewma", "garch"):
            for confidence, scale in (("0.95", 1.0), ("0.99", 0.3)):
                for mode in ("sliding", "expanding"):
                    backtest.append([symbol, mode, model, confidence, 100, 5, rate * scale,
                                     0.05, 0.1, 0.7])
    write_csv(root / "backtest" / "backtest.csv",
              ["symbol", "window_mode", "model", "confidence", "observations", "breaches",
               "breach_rate", "expected_rate", "kupiec_lr", "kupiec_p_value"], backtest)


class TickTest(unittest.TestCase):
    def test_nice_step_uses_one_two_five_steps(self) -> None:
        self.assertEqual(plots.nice_step(6.0), 1.0)
        self.assertEqual(plots.nice_step(9.0), 2.0)
        self.assertEqual(plots.nice_step(25.0), 5.0)
        self.assertEqual(plots.nice_step(70.0), 20.0)
        self.assertAlmostEqual(plots.nice_step(0.03), 0.005, places=12)

    def test_nice_step_rejects_empty_spans(self) -> None:
        for span in (0.0, -1.0, float("nan")):
            with self.subTest(span=span), self.assertRaises(PlotError):
                plots.nice_step(span)

    def test_domain_covers_values_and_reference_on_tick_boundaries(self) -> None:
        start, end, ticks = plots.nice_domain([-5.3, 29.5], 0.0)
        self.assertEqual((start, end), (-10.0, 30.0))
        self.assertEqual(ticks, [-10.0, 0.0, 10.0, 20.0, 30.0])

    def test_domain_includes_a_reference_outside_the_values(self) -> None:
        start, end, _ = plots.nice_domain([3.0, 4.0], 5.0)
        self.assertLessEqual(start, 3.0)
        self.assertGreaterEqual(end, 5.0)

    def test_domain_handles_identical_values(self) -> None:
        start, end, ticks = plots.nice_domain([2.0, 2.0], 2.0)
        self.assertLess(start, end)
        self.assertGreaterEqual(len(ticks), 2)


class FormatTest(unittest.TestCase):
    def test_signed_percent_uses_a_true_minus_and_no_negative_zero(self) -> None:
        self.assertEqual(plots.signed_percent(8.86), "+8.9%")
        self.assertEqual(plots.signed_percent(-5.31), "−5.3%")
        self.assertEqual(plots.signed_percent(-0.04), "0.0%")

    def test_plain_percent(self) -> None:
        self.assertEqual(plots.plain_percent(-10.0), "−10%")
        self.assertEqual(plots.plain_percent(5.042, 2), "5.04%")


class JitterTest(unittest.TestCase):
    def test_jitter_is_deterministic_and_bounded(self) -> None:
        values = [plots.jitter(i) for i in range(500)]
        self.assertEqual(values, [plots.jitter(i) for i in range(500)])
        self.assertTrue(all(-0.5 <= v < 0.5 for v in values))
        self.assertGreater(len(set(round(v, 6) for v in values)), 450)


class FigureTest(unittest.TestCase):
    def test_improvement_rows_are_sorted_per_model_and_proxy(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            write_run(root)
            figure = plots.figures(root, "sliding")[0]
        squared, parkinson = figure.panels
        self.assertEqual([row.label for row in squared.rows], ["RV63", "EWMA", "GARCH(1,1)"])
        self.assertEqual(squared.rows[2].values, (6.0, 8.0, 13.0))
        self.assertEqual(parkinson.rows[2].values, (18.0, 24.0, 39.0))

    def test_breach_rates_are_percentages_with_expected_references(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            write_run(root)
            figure = plots.figures(root, "sliding")[1]
        ninety_five, ninety_nine = figure.panels
        self.assertAlmostEqual(ninety_five.reference, 5.0, places=12)
        self.assertAlmostEqual(ninety_nine.reference, 1.0, places=12)
        for got, want in zip(ninety_five.rows[0].values, (4.0, 5.0, 6.0)):
            self.assertAlmostEqual(got, want, places=12)

    def test_missing_rows_and_files_fail_loudly(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            with self.assertRaises(PlotError):
                plots.figures(root, "sliding")
            write_run(root)
            rows = plots.read_csv(root / "eval" / "improvements.csv")
            with self.assertRaises(PlotError):
                plots.collect(rows, "qlike_vs_baseline_pct", model="none")

    def test_non_finite_values_fail_loudly(self) -> None:
        rows = [{"model": "garch", "value": "nan"}]
        with self.assertRaises(PlotError):
            plots.collect(rows, "value", model="garch")


class RenderTest(unittest.TestCase):
    def test_renders_valid_svg_with_one_dot_per_value_in_both_themes(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            write_run(root)
            out = root / "figures"
            self.assertEqual(plots.main(["--results", str(root), "--output", str(out)]), 0)
            names = sorted(p.name for p in out.iterdir())
            self.assertEqual(names, ["qlike-improvement-dark.svg", "qlike-improvement-light.svg",
                                     "var-breaches-dark.svg", "var-breaches-light.svg"])
            for name, rows in (("qlike-improvement", 6), ("var-breaches", 8)):
                for theme in ("light", "dark"):
                    tree = ElementTree.parse(out / f"{name}-{theme}.svg").getroot()
                    self.assertEqual(len(tree.findall(f"{SVG}circle")), rows * 3)
                    self.assertIsNotNone(tree.find(f"{SVG}title"))
                    self.assertIsNone(tree.find(f"{SVG}style"))

    def test_themes_differ_only_in_colour(self) -> None:
        figure = plots.Figure("f", "t", "d", (plots.Panel(
            "p", (plots.Row("A", (1.0, 2.0)),), 0.0, "zero", plots.plain_percent,
            plots.signed_percent),))
        light, dark = (plots.render(figure, theme) for theme in plots.THEMES)
        self.assertNotEqual(light, dark)
        self.assertEqual(light.count("<circle"), dark.count("<circle"))


if __name__ == "__main__":
    unittest.main()
