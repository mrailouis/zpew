"""Draw the published result figures as light and dark SVG files from a zpew run."""

from __future__ import annotations

import argparse
import csv
import math
import statistics
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Sequence
from xml.sax.saxutils import escape

GOLDEN_FRACTION = 0.6180339887498949
WIDTH = 720
LABEL_WIDTH = 104
VALUE_WIDTH = 92
ROW_HEIGHT = 40
PANEL_TITLE_HEIGHT = 44
AXIS_HEIGHT = 30
PANEL_GAP = 22
DOT_RADIUS = 3.2
FONT = "-apple-system, BlinkMacSystemFont, 'Segoe UI', system-ui, sans-serif"

MODEL_LABELS = {"rv21": "RV21", "rv63": "RV63", "ewma": "EWMA", "garch": "GARCH(1,1)"}


class PlotError(Exception):
    pass


@dataclass(frozen=True)
class Theme:
    name: str
    text: str
    muted: str
    faint: str
    grid: str
    reference: str
    dot: str
    dot_opacity: float


THEMES = (
    Theme("light", "#0d0d0d", "#5d5d5d", "#8f8f8f", "#ebebeb", "#5d5d5d", "#2a78d6", 0.55),
    Theme("dark", "#ececec", "#afafaf", "#7b7b7b", "#353535", "#afafaf", "#3987e5", 0.6),
)


@dataclass(frozen=True)
class Row:
    label: str
    values: tuple[float, ...]


@dataclass(frozen=True)
class Panel:
    title: str
    rows: tuple[Row, ...]
    reference: float
    reference_label: str
    format_tick: Callable[[float], str]
    format_median: Callable[[float], str]


@dataclass(frozen=True)
class Figure:
    name: str
    title: str
    description: str
    panels: tuple[Panel, ...]


def nice_step(span: float, target_ticks: int = 6) -> float:
    if not math.isfinite(span) or span <= 0.0:
        raise PlotError(f"cannot choose ticks for a span of {span}")
    raw = span / target_ticks
    magnitude = 10.0 ** math.floor(math.log10(raw))
    for factor in (1.0, 2.0, 5.0, 10.0):
        if raw <= factor * magnitude:
            return factor * magnitude
    return 10.0 * magnitude


def nice_domain(values: Sequence[float], include: float) -> tuple[float, float, list[float]]:
    low = min(min(values), include)
    high = max(max(values), include)
    if high == low:
        high = low + 1.0
    step = nice_step(high - low)
    start = math.floor(low / step) * step
    end = math.ceil(high / step) * step
    count = round((end - start) / step)
    ticks = [round(start + i * step, 10) for i in range(count + 1)]
    return start, end, ticks


def jitter(index: int) -> float:
    return (index * GOLDEN_FRACTION) % 1.0 - 0.5


def signed_percent(value: float, digits: int = 1) -> str:
    text = f"{value:+.{digits}f}%"
    if text in (f"+{0:.{digits}f}%", f"-{0:.{digits}f}%"):
        text = f"{0:.{digits}f}%"
    return text.replace("-", "−")


def plain_percent(value: float, digits: int = 0) -> str:
    return f"{value:.{digits}f}%".replace("-", "−")


def read_csv(path: Path) -> list[dict[str, str]]:
    if not path.is_file():
        raise PlotError(f"{path} does not exist; run zpew-cli eval and backtest first")
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def collect(rows: list[dict[str, str]], value: str, **match: str) -> tuple[float, ...]:
    selected = [float(r[value]) for r in rows if all(r[k] == v for k, v in match.items())]
    if not selected:
        raise PlotError(f"no rows match {match}")
    if not all(math.isfinite(v) for v in selected):
        raise PlotError(f"non-finite {value} in rows matching {match}")
    return tuple(sorted(selected))


def improvement_figure(improvements: list[dict[str, str]], mode: str) -> Figure:
    panels = []
    for proxy, title in (("squared_return", "Against squared returns"),
                         ("parkinson", "Against the Parkinson range")):
        rows = tuple(
            Row(MODEL_LABELS[model], collect(improvements, "qlike_vs_baseline_pct",
                                             window_mode=mode, model=model, proxy=proxy))
            for model in ("rv63", "ewma", "garch"))
        panels.append(Panel(title, rows, 0.0, "RV21", plain_percent, signed_percent))
    count = len(panels[0].rows[0].values)
    return Figure(
        "qlike-improvement",
        "QLIKE improvement over RV21 per pair",
        f"Each dot is one of {count} crypto pairs: the percentage reduction in QLIKE loss "
        f"against the RV21 baseline, {mode} window. Dots right of the zero line beat the "
        "baseline. The value on the right is the median.",
        tuple(panels))


def breach_figure(backtest: list[dict[str, str]], mode: str) -> Figure:
    panels = []
    for confidence, title in (("0.95", "95% VaR"), ("0.99", "99% VaR")):
        expected = 100.0 * (1.0 - float(confidence))
        rows = tuple(
            Row(MODEL_LABELS[model], tuple(100.0 * v for v in collect(
                backtest, "breach_rate", window_mode=mode, model=model, confidence=confidence)))
            for model in ("rv21", "rv63", "ewma", "garch"))
        panels.append(Panel(title, rows, expected, f"expected {expected:.0f}%",
                            lambda v: plain_percent(v, 0 if v == round(v) else 1),
                            lambda v: plain_percent(v, 2)))
    count = len(panels[0].rows[0].values)
    return Figure(
        "var-breaches",
        "VaR breach rate per pair",
        f"Each dot is one of {count} crypto pairs: the share of days whose return fell below "
        f"the one-day normal VaR, {mode} window. The dashed line is the expected rate. "
        "The value on the right is the median.",
        tuple(panels))


def panel_height(panel: Panel) -> int:
    return PANEL_TITLE_HEIGHT + ROW_HEIGHT * len(panel.rows) + AXIS_HEIGHT


def figure_height(figure: Figure) -> int:
    return sum(panel_height(p) for p in figure.panels) + PANEL_GAP * (len(figure.panels) - 1)


def text(x: float, y: float, content: str, fill: str, size: int = 12, anchor: str = "start",
         weight: int = 400) -> str:
    return (f'<text x="{x:.1f}" y="{y:.1f}" fill="{fill}" font-size="{size}" '
            f'font-weight="{weight}" text-anchor="{anchor}">{escape(content)}</text>')


def line(x1: float, y1: float, x2: float, y2: float, stroke: str, width: float = 1.0,
         dash: str = "") -> str:
    dashed = f' stroke-dasharray="{dash}"' if dash else ""
    return (f'<line x1="{x1:.1f}" y1="{y1:.1f}" x2="{x2:.1f}" y2="{y2:.1f}" '
            f'stroke="{stroke}" stroke-width="{width}"{dashed}/>')


def draw_panel(panel: Panel, top: float, theme: Theme) -> list[str]:
    left = LABEL_WIDTH
    right = WIDTH - VALUE_WIDTH
    every_value = [v for row in panel.rows for v in row.values]
    start, end, ticks = nice_domain(every_value, panel.reference)

    def to_x(value: float) -> float:
        return left + (value - start) / (end - start) * (right - left)

    rows_top = top + PANEL_TITLE_HEIGHT
    rows_bottom = rows_top + ROW_HEIGHT * len(panel.rows)
    parts = [text(0, top + 16, panel.title, theme.text, 13, weight=600),
             text(WIDTH, top + 16, "median", theme.faint, 11, "end")]
    for tick in ticks:
        x = to_x(tick)
        parts.append(line(x, rows_top, x, rows_bottom, theme.grid))
        parts.append(text(x, rows_bottom + 18, panel.format_tick(tick), theme.faint, 11, "middle"))
    reference_x = to_x(panel.reference)
    parts.append(line(reference_x, rows_top - 16, reference_x, rows_bottom, theme.reference, 1.2,
                      "" if panel.reference == 0.0 else "4 3"))
    parts.append(text(reference_x + 4, rows_top - 6, panel.reference_label, theme.muted, 10))
    for index, row in enumerate(panel.rows):
        centre = rows_top + ROW_HEIGHT * (index + 0.5)
        parts.append(text(0, centre + 4, row.label, theme.text, 12))
        for position, value in enumerate(row.values):
            y = centre + jitter(position) * (ROW_HEIGHT - 14)
            parts.append(f'<circle cx="{to_x(value):.1f}" cy="{y:.1f}" r="{DOT_RADIUS}" '
                         f'fill="{theme.dot}" fill-opacity="{theme.dot_opacity}"/>')
        median = statistics.median(row.values)
        median_x = to_x(median)
        parts.append(line(median_x, centre - ROW_HEIGHT / 2 + 5, median_x,
                          centre + ROW_HEIGHT / 2 - 5, theme.text, 2.0))
        parts.append(text(WIDTH, centre + 4, panel.format_median(median), theme.text, 12, "end"))
    return parts


def render(figure: Figure, theme: Theme) -> str:
    height = figure_height(figure)
    body: list[str] = []
    top = 0.0
    for panel in figure.panels:
        body.extend(draw_panel(panel, top, theme))
        top += panel_height(panel) + PANEL_GAP
    return "\n".join([
        f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {WIDTH} {height}" '
        f'width="{WIDTH}" height="{height}" role="img" font-family="{FONT}">',
        f"<title>{escape(figure.title)}</title>",
        f"<desc>{escape(figure.description)}</desc>",
        *body,
        "</svg>",
        "",
    ])


def figures(results: Path, mode: str) -> list[Figure]:
    improvements = read_csv(results / "eval" / "improvements.csv")
    backtest = read_csv(results / "backtest" / "backtest.csv")
    return [improvement_figure(improvements, mode), breach_figure(backtest, mode)]


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--results", type=Path, default=Path("results/default"),
                        help="run directory written by zpew-cli (default: results/default)")
    parser.add_argument("--output", type=Path, required=True, help="directory for the SVG files")
    parser.add_argument("--window-mode", default="sliding", choices=("sliding", "expanding"))
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    args.output.mkdir(parents=True, exist_ok=True)
    for figure in figures(args.results, args.window_mode):
        for theme in THEMES:
            path = args.output / f"{figure.name}-{theme.name}.svg"
            path.write_text(render(figure, theme), encoding="utf-8")
            print(f"wrote {path}")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main(sys.argv[1:]))
    except PlotError as error:
        print(f"error: {error}", file=sys.stderr)
        sys.exit(2)
