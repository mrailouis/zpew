import { describe, expect, it } from "vitest";

import { aggregateCandles, barsPerColumn } from "./decimate";

const ohlc = {
    open: [10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21],
    high: [11, 15, 13, 14, 15, 30, 17, 18, 19, 20, 21, 22],
    low: [9, 10, 2, 12, 13, 14, 15, 16, 17, 18, 19, 20],
    close: [10.5, 11.5, 12.5, 13.5, 14.5, 15.5, 16.5, 17.5, 18.5, 19.5, 20.5, 21.5],
};

describe("barsPerColumn", () => {
    it("draws one bar per candle when bars are at least a pixel wide", () => {
        expect(barsPerColumn(3)).toBe(1);
        expect(barsPerColumn(1)).toBe(1);
    });

    it("groups enough bars to fill one pixel when bars are narrower", () => {
        expect(barsPerColumn(0.25)).toBe(4);
        expect(barsPerColumn(0.3)).toBe(4);
    });
});

describe("aggregateCandles", () => {
    it("takes the first open, highest high, lowest low and last close of each group", () => {
        const candles = aggregateCandles(ohlc, 0, 8, 4);
        expect(candles).toEqual([
            { first: 0, last: 3, open: 10, high: 15, low: 2, close: 13.5 },
            { first: 4, last: 7, open: 14, high: 30, low: 13, close: 17.5 },
        ]);
    });

    it("aligns groups to fixed bar boundaries so panning does not reshape candles", () => {
        const candles = aggregateCandles(ohlc, 5, 12, 4);
        expect(candles.map((c) => [c.first, c.last])).toEqual([
            [5, 7],
            [8, 11],
        ]);
    });

    it("returns single bars unchanged for a group size of one", () => {
        const candles = aggregateCandles(ohlc, 2, 4, 1);
        expect(candles).toEqual([
            { first: 2, last: 2, open: 12, high: 13, low: 2, close: 12.5 },
            { first: 3, last: 3, open: 13, high: 14, low: 12, close: 13.5 },
        ]);
    });
});
