"""Cross-check zpew's EWMA forecasts against an independent Python implementation."""

from __future__ import annotations

import argparse
import csv
import json
import math
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
MANIFEST_PATH = REPO_ROOT / "data" / "manifest.json"


def read_closes(snapshot: Path) -> tuple[list[str], list[float]]:
    with snapshot.open(newline="", encoding="ascii") as stream:
        rows = list(csv.DictReader(stream))
    return [row["date"] for row in rows], [float(row["close"]) for row in rows]


def log_returns(closes: list[float]) -> list[float]:
    return [math.log(closes[i] / closes[i - 1]) for i in range(1, len(closes))]


def ewma_forecasts(returns: list[float], lam: float, seed: int) -> list[float]:
    if not 0.0 < lam < 1.0:
        raise ValueError(f"lambda must be in (0, 1), got {lam}")
    if seed < 1 or len(returns) < seed:
        raise ValueError(f"need at least {seed} returns")
    total = 0.0
    for r in returns[:seed]:
        total += r * r
    variance = total / seed
    forecasts = []
    for r in returns[seed:]:
        forecasts.append(variance)
        variance = (lam * variance) + ((1.0 - lam) * (r * r))
    return forecasts


def run_cli(cli: Path, symbol: str, lam: float, seed: int) -> list[tuple[str, float, float]]:
    output = subprocess.run(
        [str(cli), "forecast", "--symbol", symbol, "--model", "ewma", "--lambda", repr(lam),
         "--seed", str(seed), "--manifest", str(MANIFEST_PATH)],
        check=True, capture_output=True, text=True).stdout
    lines = [line for line in output.splitlines() if not line.startswith("#")]
    reader = csv.DictReader(lines)
    return [(row["date"], float(row["return"]), float(row["forecast_variance"])) for row in reader]


def relative_difference(a: float, b: float) -> float:
    return abs(a - b) / max(abs(a), abs(b), sys.float_info.min)


def check_asset(cli: Path, entry: dict, lam: float, seed: int) -> tuple[int, float, float]:
    dates, closes = read_closes(MANIFEST_PATH.parent / entry["file"])
    returns = log_returns(closes)
    expected = ewma_forecasts(returns, lam, seed)
    actual = run_cli(cli, entry["symbol"], lam, seed)
    if len(actual) != len(expected):
        raise AssertionError(f"{len(actual)} forecasts from zpew, {len(expected)} expected")
    worst_return = 0.0
    worst_forecast = 0.0
    for offset, (date, r, forecast) in enumerate(actual):
        index = seed + offset
        if date != dates[index + 1]:
            raise AssertionError(f"row {offset}: date {date}, expected {dates[index + 1]}")
        worst_return = max(worst_return, relative_difference(r, returns[index]))
        worst_forecast = max(worst_forecast, relative_difference(forecast, expected[offset]))
    return len(actual), worst_return, worst_forecast


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cli", required=True, type=Path, help="path to the zpew-cli binary")
    parser.add_argument("--lambda", dest="lam", default=0.94, type=float)
    parser.add_argument("--seed", default=21, type=int)
    parser.add_argument("--tolerance", default=1e-12, type=float,
                        help="maximum allowed relative difference")
    parser.add_argument("--only", default="", help="comma-separated symbols")
    args = parser.parse_args(argv)

    manifest = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))
    wanted = {s.strip().upper() for s in args.only.split(",") if s.strip()}
    entries = [e for e in manifest["assets"] if not wanted or e["symbol"] in wanted]

    failures = 0
    overall = 0.0
    print(f"{'symbol':<8} {'forecasts':>9}  {'max rel diff (return)':>21}  {'max rel diff (forecast)':>23}")
    for entry in entries:
        try:
            count, worst_return, worst_forecast = check_asset(args.cli, entry, args.lam, args.seed)
        except (AssertionError, ValueError, subprocess.CalledProcessError) as error:
            failures += 1
            print(f"{entry['symbol']:<8} FAILED: {error}")
            continue
        overall = max(overall, worst_return, worst_forecast)
        status = "" if max(worst_return, worst_forecast) <= args.tolerance else "  EXCEEDS TOLERANCE"
        failures += bool(status)
        print(f"{entry['symbol']:<8} {count:>9}  {worst_return:>21.3e}  {worst_forecast:>23.3e}{status}")

    print(f"\n{len(entries) - failures} of {len(entries)} assets within {args.tolerance:g}; "
          f"largest relative difference {overall:.3e}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
