"""Fetch daily OHLCV snapshots from Tiingo and record them in data/manifest.json."""

from __future__ import annotations

import argparse
import collections
import csv
import datetime as dt
import hashlib
import json
import math
import os
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Iterable

REPO_ROOT = Path(__file__).resolve().parent.parent
DATA_DIR = REPO_ROOT / "data"
SNAPSHOT_DIR = DATA_DIR / "snapshots"
MANIFEST_PATH = DATA_DIR / "manifest.json"
UNIVERSE_PATH = DATA_DIR / "universe.csv"
FETCH_STATE_PATH = SNAPSHOT_DIR / ".fetch_state.json"

API_ROOT = "https://api.tiingo.com"
TERMS_URL = "https://www.tiingo.com/about/pricing"
SOURCE = "tiingo"
CSV_HEADER = "date,open,high,low,close,volume"
SCHEMA_VERSION = 1

REQUESTS_PER_HOUR = 50
REQUESTS_PER_DAY = 1000
RATE_LIMIT_RETRY_SECONDS = 15 * 60
MAX_ATTEMPTS = 4


class FetchError(Exception):
    pass


@dataclass(frozen=True)
class Asset:
    symbol: str
    tiingo_ticker: str
    endpoint: str
    asset_class: str
    calendar: str
    name: str


@dataclass(frozen=True)
class Bar:
    date: dt.date
    open: float
    high: float
    low: float
    close: float
    volume: float


def parse_env_file(text: str) -> dict[str, str]:
    values: dict[str, str] = {}
    for number, raw in enumerate(text.splitlines(), start=1):
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        if line.startswith("export "):
            line = line[len("export "):].lstrip()
        key, separator, value = line.partition("=")
        key = key.strip()
        if not separator or not key:
            raise FetchError(f".env line {number} is not KEY=VALUE")
        value = value.strip()
        if len(value) >= 2 and value[0] == value[-1] and value[0] in "\"'":
            value = value[1:-1]
        values[key] = value
    return values


def load_api_key(env_path: Path = REPO_ROOT / ".env") -> str:
    key = os.environ.get("TIINGO_API_KEY", "").strip()
    if not key and env_path.is_file():
        key = parse_env_file(env_path.read_text(encoding="utf-8")).get("TIINGO_API_KEY", "")
    if not key:
        raise FetchError(
            "TIINGO_API_KEY is not set. Add it to .env or the environment; see README.md.")
    return key


def read_universe(path: Path = UNIVERSE_PATH) -> list[Asset]:
    with path.open(newline="", encoding="utf-8") as stream:
        assets = [Asset(**row) for row in csv.DictReader(stream)]
    symbols = [asset.symbol for asset in assets]
    duplicates = sorted({s for s in symbols if symbols.count(s) > 1})
    if duplicates:
        raise FetchError(f"duplicate symbols in {path.name}: {', '.join(duplicates)}")
    for asset in assets:
        if asset.endpoint not in ("daily", "crypto"):
            raise FetchError(f"{asset.symbol}: unknown endpoint '{asset.endpoint}'")
        if asset.calendar not in ("weekdays", "daily"):
            raise FetchError(f"{asset.symbol}: unknown calendar '{asset.calendar}'")
    return assets


def parse_midnight_utc(timestamp: str, symbol: str) -> dt.date:
    value = dt.datetime.fromisoformat(timestamp.replace("Z", "+00:00"))
    if value.utcoffset() != dt.timedelta(0) or value.time() != dt.time(0, 0):
        raise FetchError(f"{symbol}: bar timestamp {timestamp} is not midnight UTC")
    return value.date()


def bars_from_daily(rows: list[dict], symbol: str) -> list[Bar]:
    return [
        Bar(
            date=parse_midnight_utc(row["date"], symbol),
            open=float(row["adjOpen"]),
            high=float(row["adjHigh"]),
            low=float(row["adjLow"]),
            close=float(row["adjClose"]),
            volume=float(row["adjVolume"]),
        )
        for row in rows
    ]


def bars_from_crypto(payload: list[dict], symbol: str) -> list[Bar]:
    if len(payload) != 1:
        raise FetchError(f"{symbol}: expected one crypto series, got {len(payload)}")
    return [
        Bar(
            date=parse_midnight_utc(row["date"], symbol),
            open=float(row["open"]),
            high=float(row["high"]),
            low=float(row["low"]),
            close=float(row["close"]),
            volume=float(row["volume"]),
        )
        for row in payload[0]["priceData"]
    ]


def validate_bars(bars: list[Bar], calendar: str, symbol: str) -> list[str]:
    problems: list[str] = []
    if len(bars) < 2:
        problems.append(f"{symbol}: only {len(bars)} bars")
    for index, bar in enumerate(bars):
        where = f"{symbol} {bar.date.isoformat()}"
        prices = {"open": bar.open, "high": bar.high, "low": bar.low, "close": bar.close}
        for name, price in prices.items():
            if not math.isfinite(price) or price <= 0.0:
                problems.append(f"{where}: {name} {price} is not finite and positive")
        if not math.isfinite(bar.volume) or bar.volume < 0.0:
            problems.append(f"{where}: volume {bar.volume} is not finite and non-negative")
        if bar.high < max(bar.open, bar.close):
            problems.append(f"{where}: high {bar.high} below max(open, close)")
        if bar.low > min(bar.open, bar.close):
            problems.append(f"{where}: low {bar.low} above min(open, close)")
        if calendar == "weekdays" and bar.date.weekday() >= 5:
            problems.append(f"{where}: weekend bar on a weekdays calendar")
        if index > 0 and bar.date <= bars[index - 1].date:
            problems.append(f"{where}: date not after {bars[index - 1].date.isoformat()}")
    return problems


def format_number(value: float) -> str:
    text = repr(float(value))
    return text[:-2] if text.endswith(".0") else text


def to_csv(bars: Iterable[Bar]) -> bytes:
    lines = [CSV_HEADER]
    for bar in bars:
        fields = (bar.open, bar.high, bar.low, bar.close, bar.volume)
        lines.append(",".join([bar.date.isoformat(), *map(format_number, fields)]))
    return ("\n".join(lines) + "\n").encode("ascii")


class RequestPacer:
    def __init__(self, per_hour: int = REQUESTS_PER_HOUR, per_day: int = REQUESTS_PER_DAY,
                 clock: Callable[[], float] = time.monotonic,
                 sleep: Callable[[float], None] = time.sleep) -> None:
        self._limits = ((per_hour, 3600.0), (per_day, 86400.0))
        self._history: collections.deque[float] = collections.deque()
        self._clock = clock
        self._sleep = sleep

    def wait_for_slot(self) -> float:
        waited = 0.0
        while True:
            now = self._clock()
            while self._history and now - self._history[0] >= 86400.0:
                self._history.popleft()
            delay = 0.0
            for limit, window in self._limits:
                recent = [t for t in self._history if now - t < window]
                if len(recent) >= limit:
                    delay = max(delay, recent[-limit] + window - now)
            if delay <= 0.0:
                self._history.append(now)
                return waited
            self._sleep(delay)
            waited += delay


class TiingoClient:
    def __init__(self, api_key: str, pacer: RequestPacer) -> None:
        self._api_key = api_key
        self._pacer = pacer

    def get_json(self, url: str) -> object:
        for attempt in range(1, MAX_ATTEMPTS + 1):
            waited = self._pacer.wait_for_slot()
            if waited > 0:
                log(f"  waited {waited / 60:.1f} min for the rate limit")
            request = urllib.request.Request(url, headers={
                "Authorization": f"Token {self._api_key}",
                "Content-Type": "application/json",
                "User-Agent": "zpew-fetch-data",
            })
            try:
                with urllib.request.urlopen(request, timeout=120) as response:
                    return json.load(response)
            except urllib.error.HTTPError as error:
                if error.code == 429 and attempt < MAX_ATTEMPTS:
                    log(f"  rate limited by Tiingo; retrying in {RATE_LIMIT_RETRY_SECONDS // 60} min")
                    time.sleep(RATE_LIMIT_RETRY_SECONDS)
                    continue
                body = error.read(300).decode("utf-8", "replace")
                raise FetchError(f"HTTP {error.code} for {url}: {body}") from None
            except urllib.error.URLError as error:
                if attempt < MAX_ATTEMPTS:
                    log(f"  network error ({error.reason}); retrying")
                    time.sleep(30 * attempt)
                    continue
                raise FetchError(f"network error for {url}: {error.reason}") from None
        raise FetchError(f"gave up on {url}")


def request_url(asset: Asset, start: dt.date, end: dt.date) -> str:
    if asset.endpoint == "daily":
        path = f"/tiingo/daily/{urllib.parse.quote(asset.tiingo_ticker)}/prices"
        query = {"startDate": start.isoformat(), "endDate": end.isoformat(), "format": "json"}
    else:
        path = "/tiingo/crypto/prices"
        query = {"tickers": asset.tiingo_ticker, "startDate": start.isoformat(),
                 "endDate": end.isoformat(), "resampleFreq": "1day"}
    return f"{API_ROOT}{path}?{urllib.parse.urlencode(query)}"


def fetch_page(client: TiingoClient, asset: Asset, start: dt.date, end: dt.date) -> list[Bar]:
    payload = client.get_json(request_url(asset, start, end))
    if not isinstance(payload, list):
        raise FetchError(f"{asset.symbol}: unexpected response {str(payload)[:200]}")
    if asset.endpoint == "crypto" and not payload:
        return []
    bars = (bars_from_daily(payload, asset.symbol) if asset.endpoint == "daily"
            else bars_from_crypto(payload, asset.symbol))
    return [bar for bar in bars if start <= bar.date <= end]


def fetch_bars(client: TiingoClient, asset: Asset, start: dt.date, end: dt.date,
               fetch: Callable[[dt.date, dt.date], list[Bar]] | None = None) -> list[Bar]:
    fetch = fetch or (lambda s, e: fetch_page(client, asset, s, e))
    bars: list[Bar] = []
    page_start = start
    while page_start <= end:
        page = fetch(page_start, end)
        if not page:
            break
        bars.extend(page)
        if page[-1].date >= end:
            break
        page_start = page[-1].date + dt.timedelta(days=1)
    return bars


def utc_now() -> str:
    return dt.datetime.now(dt.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def write_atomically(path: Path, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_bytes(data)
    os.replace(temporary, path)


def manifest_entry(asset: Asset, url: str, bars: list[Bar], file: str, digest: str) -> dict:
    return {
        "symbol": asset.symbol,
        "name": asset.name,
        "asset_class": asset.asset_class,
        "calendar": asset.calendar,
        "source": SOURCE,
        "source_url": url,
        "terms_url": TERMS_URL,
        "fetched_at": utc_now(),
        "first_date": bars[0].date.isoformat(),
        "last_date": bars[-1].date.isoformat(),
        "rows": len(bars),
        "file": file,
        "sha256": digest,
    }


def load_manifest_entries(path: Path = MANIFEST_PATH) -> dict[str, dict]:
    if not path.is_file():
        return {}
    manifest = json.loads(path.read_text(encoding="utf-8"))
    return {entry["symbol"]: entry for entry in manifest["assets"]}


def write_manifest(entries: dict[str, dict], universe: list[Asset],
                   path: Path = MANIFEST_PATH) -> None:
    order = [asset.symbol for asset in universe if asset.symbol in entries]
    manifest = {"schema_version": SCHEMA_VERSION, "assets": [entries[s] for s in order]}
    write_atomically(path, (json.dumps(manifest, indent=2) + "\n").encode("utf-8"))


def is_current(entry: dict | None) -> bool:
    if entry is None:
        return False
    snapshot = DATA_DIR / entry["file"]
    return snapshot.is_file() and hashlib.sha256(snapshot.read_bytes()).hexdigest() == entry["sha256"]


def check_fetch_state(start: dt.date, end: dt.date, refresh: bool) -> None:
    state = {"start": start.isoformat(), "end": end.isoformat()}
    if FETCH_STATE_PATH.is_file() and not refresh:
        previous = json.loads(FETCH_STATE_PATH.read_text(encoding="utf-8"))
        if previous != state:
            raise FetchError(
                f"existing snapshots cover {previous['start']} to {previous['end']}; "
                "pass the same --start/--end to resume, or --refresh to refetch everything")
    write_atomically(FETCH_STATE_PATH, (json.dumps(state) + "\n").encode("utf-8"))


def log(message: str) -> None:
    print(message, flush=True)


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--end", required=True, type=dt.date.fromisoformat,
                        help="last date to include (YYYY-MM-DD); fixed for reproducibility")
    parser.add_argument("--start", default=dt.date(1990, 1, 1), type=dt.date.fromisoformat,
                        help="first date to request (default: 1990-01-01)")
    parser.add_argument("--only", default="", help="comma-separated symbols to fetch")
    parser.add_argument("--refresh", action="store_true",
                        help="refetch assets even if their snapshot is current")
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    if args.end <= args.start:
        raise FetchError("--end must be after --start")
    if args.end >= dt.datetime.now(dt.timezone.utc).date():
        raise FetchError("--end must be a completed day in the past")

    universe = read_universe()
    wanted = {s.strip().upper() for s in args.only.split(",") if s.strip()}
    unknown = wanted - {asset.symbol for asset in universe}
    if unknown:
        raise FetchError(f"unknown symbols: {', '.join(sorted(unknown))}")
    selected = [asset for asset in universe if not wanted or asset.symbol in wanted]

    api_key = load_api_key()
    check_fetch_state(args.start, args.end, args.refresh)
    entries = load_manifest_entries()
    client = TiingoClient(api_key, RequestPacer())

    failures: list[str] = []
    for position, asset in enumerate(selected, start=1):
        prefix = f"[{position}/{len(selected)}] {asset.symbol}"
        if not args.refresh and is_current(entries.get(asset.symbol)):
            log(f"{prefix}: current, skipped")
            continue
        url = request_url(asset, args.start, args.end)
        try:
            bars = fetch_bars(client, asset, args.start, args.end)
            problems = validate_bars(bars, asset.calendar, asset.symbol)
            if problems:
                shown = "\n    ".join(problems[:10])
                more = f"\n    ... and {len(problems) - 10} more" if len(problems) > 10 else ""
                raise FetchError(f"{asset.symbol}: {len(problems)} invalid bars\n    {shown}{more}")
        except FetchError as error:
            failures.append(str(error))
            log(f"{prefix}: FAILED\n  {error}")
            continue

        data = to_csv(bars)
        file = f"snapshots/{asset.symbol}.csv"
        write_atomically(DATA_DIR / file, data)
        entries[asset.symbol] = manifest_entry(
            asset, url, bars, file, hashlib.sha256(data).hexdigest())
        write_manifest(entries, universe)
        log(f"{prefix}: {len(bars)} bars, {bars[0].date} to {bars[-1].date}")

    if failures:
        log(f"\n{len(failures)} asset(s) failed; the manifest lists only verified snapshots.")
        return 1
    log("\nAll selected assets are current.")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main(sys.argv[1:]))
    except FetchError as error:
        print(f"error: {error}", file=sys.stderr)
        sys.exit(2)
