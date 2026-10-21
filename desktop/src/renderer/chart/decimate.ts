export interface Candle {
    readonly first: number;
    readonly last: number;
    readonly open: number;
    readonly high: number;
    readonly low: number;
    readonly close: number;
}

export interface Ohlc {
    readonly open: ArrayLike<number>;
    readonly high: ArrayLike<number>;
    readonly low: ArrayLike<number>;
    readonly close: ArrayLike<number>;
}

export function barsPerColumn(pixelsPerBar: number): number {
    return pixelsPerBar >= 1 ? 1 : Math.ceil(1 / pixelsPerBar);
}

export function aggregateCandles(
    ohlc: Ohlc,
    start: number,
    end: number,
    groupSize: number,
): Candle[] {
    const candles: Candle[] = [];
    const firstGroup = Math.floor(start / groupSize) * groupSize;
    for (let groupStart = firstGroup; groupStart < end; groupStart += groupSize) {
        const from = Math.max(groupStart, start);
        const to = Math.min(groupStart + groupSize, end);
        let high = Number.NEGATIVE_INFINITY;
        let low = Number.POSITIVE_INFINITY;
        for (let i = from; i < to; i += 1) {
            high = Math.max(high, ohlc.high[i] ?? Number.NEGATIVE_INFINITY);
            low = Math.min(low, ohlc.low[i] ?? Number.POSITIVE_INFINITY);
        }
        candles.push({
            first: from,
            last: to - 1,
            open: ohlc.open[from] ?? Number.NaN,
            high,
            low,
            close: ohlc.close[to - 1] ?? Number.NaN,
        });
    }
    return candles;
}
