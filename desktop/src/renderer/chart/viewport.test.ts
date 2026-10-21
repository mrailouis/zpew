import { describe, expect, it } from "vitest";

import {
    barToPixel,
    clampViewport,
    fitViewport,
    MIN_VISIBLE_BARS,
    panViewport,
    pixelToBar,
    visibleRange,
    zoomViewport,
} from "./viewport";

describe("viewport", () => {
    it("fits all bars with a small right margin", () => {
        expect(fitViewport(100)).toEqual({ first: 0, count: 103 });
    });

    it("zooms around the anchor so the bar under the cursor stays put", () => {
        const zoomed = zoomViewport({ first: 0, count: 100 }, 2, 0.5, 1000);
        expect(zoomed).toEqual({ first: 25, count: 50 });
        const anchoredBefore = 0 + 0.3 * 100;
        const right = zoomViewport({ first: 0, count: 100 }, 4, 0.3, 1000);
        expect(right.first + 0.3 * right.count).toBeCloseTo(anchoredBefore, 12);
    });

    it("never shows fewer than the minimum or many more bars than exist", () => {
        expect(zoomViewport({ first: 0, count: 20 }, 100, 0.5, 1000).count).toBe(MIN_VISIBLE_BARS);
        expect(zoomViewport({ first: 0, count: 100 }, 0.01, 0.5, 200).count).toBe(203);
    });

    it("stops panning before the data scrolls out of view", () => {
        expect(panViewport({ first: 0, count: 50 }, -1000, 200).first).toBe(-40);
        expect(panViewport({ first: 0, count: 50 }, 1000, 200).first).toBe(190);
        expect(clampViewport({ first: 5, count: 50 }, 200)).toEqual({ first: 5, count: 50 });
    });

    it("reports the integer bar range that is at least partly visible", () => {
        expect(visibleRange({ first: 2.5, count: 10 }, 100)).toEqual({ start: 2, end: 13 });
        expect(visibleRange({ first: -5, count: 20 }, 8)).toEqual({ start: 0, end: 8 });
    });

    it("converts between bar indices and pixel centres", () => {
        const viewport = { first: 10, count: 20 };
        const x = barToPixel(15, viewport, 0, 200);
        expect(x).toBeCloseTo(55, 12);
        expect(pixelToBar(x, viewport, 0, 200)).toBe(15);
    });
});
