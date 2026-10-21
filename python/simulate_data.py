from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import json
import math
import random
from dataclasses import dataclass
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
OUTPUT_DIR = REPO_ROOT / "data" / "synthetic"
CSV_HEADER = "date,open,high,low,close,volume"
SCHEMA_VERSION = 1
SOURCE = "simulated"
SOURCE_URL = "https://github.com/mrailouis/zpew/blob/main/python/simulate_data.py"
TERMS_URL = "https://github.com/mrailouis/zpew/blob/main/LICENSE"
GENERATED_AT = "2026-10-21T00:00:00Z"
DEFAULT_SEED = 20261021
END_DATE = dt.date(2026, 9, 30)
HOLIDAYS = ((1, 1), (7, 4), (12, 25))
STUDENT_DF = 6


@dataclass(frozen=True)
class Spec:
    symbol: str
    name: str
    asset_class: str
    calendar: str
    start: dt.date
    price: float
    annual_vol: float
    persistence: float
    shock: float
    volume: float
    outage: tuple[dt.date, int] | None = None


SPECS = (
    Spec("SIMEQ", "Simulated equity index", "us_equity_broad", "weekdays",
         dt.date(2000, 1, 3), 100.0, 0.18, 0.985, 0.09, 8.0e7,
         (dt.date(2011, 8, 15), 4)),
    Spec("SIMTECH", "Simulated technology sector", "us_equity_sector", "weekdays",
         dt.date(2003, 1, 2), 40.0, 0.26, 0.98, 0.1, 2.5e7),
    Spec("SIMBOND", "Simulated government bond fund", "rates", "weekdays",
         dt.date(2004, 1, 2), 90.0, 0.07, 0.99, 0.06, 6.0e6),
    Spec("SIMGOLD", "Simulated gold fund", "commodity", "weekdays",
         dt.date(2006, 1, 3), 55.0, 0.16, 0.975, 0.08, 9.0e6),
    Spec("SIMCOIN", "Simulated cryptocurrency", "crypto", "daily",
         dt.date(2016, 1, 1), 400.0, 0.7, 0.95, 0.12, 3.0e9),
)


@dataclass(frozen=True)
class Bar:
    date: dt.date
    open: float
    high: float
    low: float
    close: float
    volume: int


def is_session(date: dt.date, calendar: str) -> bool:
    if calendar == "daily":
        return True
    return date.weekday() < 5 and (date.month, date.day) not in HOLIDAYS


def session_dates(spec: Spec) -> list[dt.date]:
    skipped: set[dt.date] = set()
    if spec.outage is not None:
        first, count = spec.outage
        date = first
        while len(skipped) < count:
            if is_session(date, spec.calendar):
                skipped.add(date)
            date += dt.timedelta(days=1)
    dates = []
    date = spec.start
    while date <= END_DATE:
        if is_session(date, spec.calendar) and date not in skipped:
            dates.append(date)
        date += dt.timedelta(days=1)
    return dates


def student_shock(rng: random.Random) -> float:
    chi = sum(rng.gauss(0.0, 1.0) ** 2 for _ in range(STUDENT_DF))
    scale = math.sqrt((STUDENT_DF - 2) / STUDENT_DF)
    return rng.gauss(0.0, 1.0) / math.sqrt(chi / STUDENT_DF) * scale


def simulate(spec: Spec, rng: random.Random) -> list[Bar]:
    periods = 365 if spec.calendar == "daily" else 252
    long_run = spec.annual_vol ** 2 / periods
    alpha = spec.shock
    beta = spec.persistence - alpha
    omega = long_run * (1.0 - alpha - beta)
    variance = long_run
    close = spec.price
    bars = []
    for date in session_dates(spec):
        sigma = math.sqrt(variance)
        ret = sigma * student_shock(rng) - 0.5 * variance + 0.0002
        open_ = close * math.exp(0.25 * sigma * rng.gauss(0.0, 1.0))
        new_close = close * math.exp(ret)
        top = max(open_, new_close)
        bottom = min(open_, new_close)
        high = top * math.exp(abs(rng.gauss(0.0, 0.5 * sigma)))
        low = bottom * math.exp(-abs(rng.gauss(0.0, 0.5 * sigma)))
        volume = spec.volume * math.exp(0.3 * rng.gauss(0.0, 1.0)) * (1.0 + 25.0 * abs(ret))
        bars.append(Bar(date, round(open_, 6), round(high, 6), round(low, 6),
                        round(new_close, 6), int(volume)))
        variance = omega + alpha * ret * ret + beta * variance
        close = new_close
    return bars


def to_csv(bars: list[Bar]) -> bytes:
    lines = [CSV_HEADER]
    lines += [f"{b.date.isoformat()},{b.open:.6f},{b.high:.6f},{b.low:.6f},{b.close:.6f},{b.volume}"
              for b in bars]
    return ("\n".join(lines) + "\n").encode("utf-8")


def manifest_entry(spec: Spec, bars: list[Bar], data: bytes) -> dict:
    return {
        "symbol": spec.symbol,
        "name": spec.name,
        "asset_class": spec.asset_class,
        "calendar": spec.calendar,
        "source": SOURCE,
        "source_url": SOURCE_URL,
        "terms_url": TERMS_URL,
        "fetched_at": GENERATED_AT,
        "first_date": bars[0].date.isoformat(),
        "last_date": bars[-1].date.isoformat(),
        "rows": len(bars),
        "file": f"snapshots/{spec.symbol}.csv",
        "sha256": hashlib.sha256(data).hexdigest(),
    }


def generate(output: Path, seed: int) -> dict:
    snapshots = output / "snapshots"
    snapshots.mkdir(parents=True, exist_ok=True)
    entries = []
    for index, spec in enumerate(SPECS):
        bars = simulate(spec, random.Random(seed * 1000 + index))
        data = to_csv(bars)
        (snapshots / f"{spec.symbol}.csv").write_bytes(data)
        entries.append(manifest_entry(spec, bars, data))
    manifest = {"schema_version": SCHEMA_VERSION, "assets": entries}
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    return manifest


def main() -> None:
    parser = argparse.ArgumentParser(description="Write simulated OHLCV snapshots for demos and screenshots.")
    parser.add_argument("--output", type=Path, default=OUTPUT_DIR)
    parser.add_argument("--seed", type=int, default=DEFAULT_SEED)
    args = parser.parse_args()
    manifest = generate(args.output, args.seed)
    for entry in manifest["assets"]:
        print(f"{entry['symbol']:8} {entry['rows']:6} bars  {entry['first_date']} to {entry['last_date']}")
    print(f"manifest: {args.output / 'manifest.json'}")


if __name__ == "__main__":
    main()
