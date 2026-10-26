"""Fetch daily USDT spot klines from Binance Vision and record them in data/manifest.json."""

from __future__ import annotations

import argparse
import concurrent.futures
import csv
import datetime as dt
import hashlib
import io
import json
import math
import os
import re
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
import zipfile
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Iterable

REPO_ROOT = Path(__file__).resolve().parent.parent
DATA_DIR = REPO_ROOT / "data"
SNAPSHOT_DIR = DATA_DIR / "snapshots"
CACHE_DIR = SNAPSHOT_DIR / ".binance-cache"
MANIFEST_PATH = DATA_DIR / "manifest.json"
UNIVERSE_PATH = DATA_DIR / "universe.csv"

LISTING_ROOT = "https://s3-ap-northeast-1.amazonaws.com/data.binance.vision"
DOWNLOAD_ROOT = "https://data.binance.vision"
KLINE_PREFIX = "data/spot/monthly/klines/"
TERMS_URL = "https://data.binance.vision/Binance_Vision-Terms_of_Use.pdf"
SOURCE = "binance_vision"
QUOTE = "USDT"
ASSET_CLASS = "crypto"
CALENDAR = "daily"
CSV_HEADER = "date,open,high,low,close,volume"
UNIVERSE_HEADER = ["symbol", "status", "reason", "first_date", "last_date", "rows"]
SCHEMA_VERSION = 1

MIN_BARS = 2000
MAX_ATTEMPTS = 4
WORKERS = 4
MICROSECOND_THRESHOLD = 10**14
MILLISECONDS_PER_DAY = 86_400_000

PEGGED_BASES = frozenset({
    "AEUR", "BFUSD", "BKRW", "BUSD", "DAI", "EUR", "EURI", "FDUSD", "GBP", "PAX", "PAXG",
    "PYUSD", "RLUSD", "SUSD", "TUSD", "USD1", "USDC", "USDE", "USDP", "USDS", "USDSB",
    "UST", "XUSD",
})
LEVERAGED_TOKEN = re.compile(r"^[A-Z0-9]+(UP|DOWN|BULL|BEAR)$")
LEVERAGED_EXCEPTIONS = frozenset({"JUP", "SUPER"})


class FetchError(Exception):
    pass


@dataclass(frozen=True)
class Bar:
    date: dt.date
    open: float
    high: float
    low: float
    close: float
    volume: float


@dataclass(frozen=True)
class Selection:
    symbol: str
    status: str
    reason: str
    bars: tuple[Bar, ...] = ()


def base_asset(symbol: str) -> str:
    if not symbol.endswith(QUOTE) or len(symbol) == len(QUOTE):
        raise FetchError(f"{symbol} is not a {QUOTE} pair")
    return symbol[:-len(QUOTE)]


def exclusion_reason(symbol: str) -> str:
    base = base_asset(symbol)
    if base in PEGGED_BASES:
        return "pegged token"
    if LEVERAGED_TOKEN.match(base) and base not in LEVERAGED_EXCEPTIONS:
        return "leveraged token"
    return ""


def month_key(key: str, symbol: str) -> str | None:
    match = re.fullmatch(rf"{re.escape(KLINE_PREFIX)}{symbol}/1d/{symbol}-1d-(\d{{4}}-\d{{2}})\.zip",
                         key)
    return match.group(1) if match else None


def months_through(months: Iterable[str], end: dt.date) -> list[str]:
    last = f"{end.year:04d}-{end.month:02d}"
    return sorted(month for month in months if month <= last)


def parse_checksum(text: str, file_name: str) -> str:
    fields = text.split()
    if len(fields) != 2 or fields[1] != file_name or not re.fullmatch(r"[0-9a-f]{64}", fields[0]):
        raise FetchError(f"malformed checksum file for {file_name}: {text[:120]!r}")
    return fields[0]


def open_time_to_date(raw: str, symbol: str) -> dt.date:
    value = int(raw)
    milliseconds = value // 1000 if value >= MICROSECOND_THRESHOLD else value
    if milliseconds % MILLISECONDS_PER_DAY != 0:
        raise FetchError(f"{symbol}: kline open time {raw} is not midnight UTC")
    return dt.date(1970, 1, 1) + dt.timedelta(days=milliseconds // MILLISECONDS_PER_DAY)


def parse_klines(text: str, symbol: str) -> list[Bar]:
    bars: list[Bar] = []
    for number, row in enumerate(csv.reader(io.StringIO(text)), start=1):
        if not row:
            continue
        if number == 1 and not row[0].strip().isdigit():
            continue
        if len(row) < 6:
            raise FetchError(f"{symbol}: kline row {number} has {len(row)} fields")
        bars.append(Bar(
            date=open_time_to_date(row[0], symbol),
            open=float(row[1]),
            high=float(row[2]),
            low=float(row[3]),
            close=float(row[4]),
            volume=float(row[5]),
        ))
    return bars


def read_kline_zip(data: bytes, symbol: str) -> list[Bar]:
    with zipfile.ZipFile(io.BytesIO(data)) as archive:
        names = [name for name in archive.namelist() if name.endswith(".csv")]
        if len(names) != 1:
            raise FetchError(f"{symbol}: expected one CSV in the archive, found {len(names)}")
        return parse_klines(archive.read(names[0]).decode("ascii"), symbol)


def drop_identical_duplicates(bars: list[Bar]) -> tuple[list[Bar], int]:
    kept: list[Bar] = []
    for bar in bars:
        if kept and bar == kept[-1]:
            continue
        kept.append(bar)
    return kept, len(bars) - len(kept)


def latest_contiguous_run(bars: list[Bar]) -> list[Bar]:
    start = len(bars) - 1
    while start > 0 and bars[start].date - bars[start - 1].date == dt.timedelta(days=1):
        start -= 1
    return bars[max(start, 0):]


def validate_bars(bars: list[Bar], symbol: str) -> list[str]:
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
        if index > 0 and bar.date <= bars[index - 1].date:
            problems.append(f"{where}: date not after {bars[index - 1].date.isoformat()}")
    return problems


def select(symbol: str, bars: list[Bar], end: dt.date) -> Selection:
    reason = exclusion_reason(symbol)
    if reason:
        return Selection(symbol, "excluded", reason)
    in_range, duplicates = drop_identical_duplicates([bar for bar in bars if bar.date <= end])
    problems = validate_bars(in_range, symbol)
    if problems:
        shown = "; ".join(problems[:5])
        raise FetchError(f"{symbol}: {len(problems)} invalid bars: {shown}")
    run = latest_contiguous_run(in_range)
    dropped = len(in_range) - len(run)
    notes = []
    if duplicates:
        notes.append(f"{duplicates} identical duplicate rows removed")
    if len(run) < MIN_BARS:
        detail = f" after a gap dropped {dropped} earlier bars" if dropped else ""
        reason = "; ".join([f"{len(run)} contiguous bars{detail}", *notes])
        return Selection(symbol, "excluded", reason, tuple(run))
    if dropped:
        notes.insert(0, f"gap dropped {dropped} earlier bars")
    return Selection(symbol, "included", "; ".join(notes), tuple(run))


def format_number(value: float) -> str:
    text = repr(float(value))
    return text[:-2] if text.endswith(".0") else text


def to_csv(bars: Iterable[Bar]) -> bytes:
    lines = [CSV_HEADER]
    for bar in bars:
        fields = (bar.open, bar.high, bar.low, bar.close, bar.volume)
        lines.append(",".join([bar.date.isoformat(), *map(format_number, fields)]))
    return ("\n".join(lines) + "\n").encode("ascii")


def universe_rows(selections: Iterable[Selection]) -> list[list[str]]:
    rows = []
    for selection in selections:
        bars = selection.bars
        rows.append([
            selection.symbol,
            selection.status,
            selection.reason,
            bars[0].date.isoformat() if bars else "",
            bars[-1].date.isoformat() if bars else "",
            str(len(bars)) if bars else "",
        ])
    return rows


def source_url(symbol: str) -> str:
    return f"{DOWNLOAD_ROOT}/?prefix={KLINE_PREFIX}{symbol}/1d/"


def manifest_entry(symbol: str, bars: tuple[Bar, ...], file: str, digest: str,
                   fetched_at: str) -> dict:
    return {
        "symbol": symbol,
        "name": f"{base_asset(symbol)} / {QUOTE}",
        "asset_class": ASSET_CLASS,
        "calendar": CALENDAR,
        "source": SOURCE,
        "source_url": source_url(symbol),
        "terms_url": TERMS_URL,
        "fetched_at": fetched_at,
        "first_date": bars[0].date.isoformat(),
        "last_date": bars[-1].date.isoformat(),
        "rows": len(bars),
        "file": file,
        "sha256": digest,
    }


class BinanceVisionClient:
    def __init__(self, cache_dir: Path = CACHE_DIR,
                 sleep: Callable[[float], None] = time.sleep) -> None:
        self._cache_dir = cache_dir
        self._sleep = sleep

    def get(self, url: str) -> bytes:
        for attempt in range(1, MAX_ATTEMPTS + 1):
            request = urllib.request.Request(url, headers={"User-Agent": "zpew-fetch-binance"})
            try:
                with urllib.request.urlopen(request, timeout=120) as response:
                    return response.read()
            except urllib.error.HTTPError as error:
                if error.code in (429, 500, 502, 503, 504) and attempt < MAX_ATTEMPTS:
                    self._sleep(30 * attempt)
                    continue
                raise FetchError(f"HTTP {error.code} for {url}") from None
            except urllib.error.URLError as error:
                if attempt < MAX_ATTEMPTS:
                    self._sleep(10 * attempt)
                    continue
                raise FetchError(f"network error for {url}: {error.reason}") from None
        raise FetchError(f"gave up on {url}")

    def list_prefix(self, prefix: str) -> tuple[list[str], list[str]]:
        keys: list[str] = []
        prefixes: list[str] = []
        marker = ""
        while True:
            query = urllib.parse.urlencode({"delimiter": "/", "prefix": prefix, "marker": marker})
            text = self.get(f"{LISTING_ROOT}?{query}").decode("utf-8")
            page_keys = re.findall(r"<Key>(.*?)</Key>", text)
            page_prefixes = [p for p in re.findall(r"<Prefix>(.*?)</Prefix>", text) if p != prefix]
            keys.extend(page_keys)
            prefixes.extend(page_prefixes)
            if "<IsTruncated>true</IsTruncated>" not in text:
                return keys, prefixes
            next_marker = re.findall(r"<NextMarker>(.*?)</NextMarker>", text)
            marker = next_marker[0] if next_marker else (page_keys + page_prefixes)[-1]

    def usdt_symbols(self) -> list[str]:
        _, prefixes = self.list_prefix(KLINE_PREFIX)
        symbols = [p[len(KLINE_PREFIX):].rstrip("/") for p in prefixes]
        return sorted(s for s in symbols if s.endswith(QUOTE) and len(s) > len(QUOTE))

    def months(self, symbol: str) -> list[str]:
        keys, _ = self.list_prefix(f"{KLINE_PREFIX}{symbol}/1d/")
        return sorted(m for m in (month_key(key, symbol) for key in keys) if m)

    def month_zip(self, symbol: str, month: str) -> bytes:
        name = f"{symbol}-1d-{month}.zip"
        url = f"{DOWNLOAD_ROOT}/{KLINE_PREFIX}{symbol}/1d/{name}"
        cached = self._cache_dir / symbol / name
        expected = parse_checksum(self.get(url + ".CHECKSUM").decode("ascii"), name)
        if cached.is_file() and hashlib.sha256(cached.read_bytes()).hexdigest() == expected:
            return cached.read_bytes()
        data = self.get(url)
        actual = hashlib.sha256(data).hexdigest()
        if actual != expected:
            raise FetchError(f"{name}: SHA-256 {actual} does not match Binance's {expected}")
        write_atomically(cached, data)
        return data

    def bars(self, symbol: str, months: list[str]) -> list[Bar]:
        bars: list[Bar] = []
        for month in months:
            bars.extend(read_kline_zip(self.month_zip(symbol, month), symbol))
        return bars


def write_atomically(path: Path, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_bytes(data)
    os.replace(temporary, path)


def utc_now() -> str:
    return dt.datetime.now(dt.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def log(message: str) -> None:
    print(message, flush=True)


def could_qualify(months: list[str]) -> bool:
    return len(months) * 31 >= MIN_BARS


def write_universe(selections: list[Selection], path: Path = UNIVERSE_PATH) -> None:
    stream = io.StringIO(newline="")
    writer = csv.writer(stream, lineterminator="\n")
    writer.writerow(UNIVERSE_HEADER)
    writer.writerows(universe_rows(selections))
    write_atomically(path, stream.getvalue().encode("utf-8"))


def write_snapshots(selections: list[Selection], fetched_at: str) -> list[dict]:
    entries = []
    for selection in selections:
        if selection.status != "included":
            continue
        data = to_csv(selection.bars)
        file = f"snapshots/{selection.symbol}.csv"
        write_atomically(DATA_DIR / file, data)
        entries.append(manifest_entry(selection.symbol, selection.bars, file,
                                      hashlib.sha256(data).hexdigest(), fetched_at))
    return entries


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--end", required=True, type=dt.date.fromisoformat,
                        help="last date to include (YYYY-MM-DD); fixed for reproducibility")
    parser.add_argument("--workers", type=int, default=WORKERS,
                        help=f"parallel downloads (default: {WORKERS})")
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    if args.end >= dt.datetime.now(dt.timezone.utc).date():
        raise FetchError("--end must be a completed day in the past")
    if not 1 <= args.workers <= 8:
        raise FetchError("--workers must be between 1 and 8")

    client = BinanceVisionClient()
    symbols = client.usdt_symbols()
    log(f"{len(symbols)} {QUOTE} pairs listed; checking monthly coverage")

    def candidate(symbol: str) -> tuple[str, list[str]]:
        return symbol, months_through(client.months(symbol), args.end)

    def fetch(item: tuple[str, list[str]]) -> Selection:
        symbol, months = item
        try:
            if exclusion_reason(symbol):
                return select(symbol, [], args.end)
            return select(symbol, client.bars(symbol, months), args.end)
        except FetchError as error:
            return Selection(symbol, "failed", str(error))

    with concurrent.futures.ThreadPoolExecutor(max_workers=args.workers) as pool:
        coverage = list(pool.map(candidate, symbols))
        candidates = [item for item in coverage if could_qualify(item[1])]
        log(f"{len(candidates)} pairs have enough months; downloading")
        selections = list(pool.map(fetch, candidates))

    order = {"included": 0, "failed": 1, "excluded": 2}
    selections.sort(key=lambda s: (order[s.status], s.bars[0].date if s.bars else dt.date.max,
                                   s.symbol))
    write_universe(selections)
    entries = write_snapshots(selections, utc_now())
    manifest = {"schema_version": SCHEMA_VERSION, "assets": entries}
    write_atomically(MANIFEST_PATH, (json.dumps(manifest, indent=2) + "\n").encode("utf-8"))

    for selection in selections:
        detail = f" ({selection.reason})" if selection.reason else ""
        log(f"{selection.symbol}: {selection.status}{detail}")
    log(f"\n{len(entries)} snapshots written; selection recorded in {UNIVERSE_PATH.name}")
    failures = [s for s in selections if s.status == "failed"]
    if failures:
        log(f"{len(failures)} pair(s) failed; they are listed in {UNIVERSE_PATH.name}")
        return 1
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main(sys.argv[1:]))
    except FetchError as error:
        print(f"error: {error}", file=sys.stderr)
        sys.exit(2)
