import { describe, expect, it } from "vitest";

import { padExtent, visibleExtent } from "./extent";

describe("visibleExtent", () => {
    it("takes the minimum and maximum over the visible range of every series", () => {
        const extent = visibleExtent(
            [
                [5, 1, 9, 100],
                [Number.NaN, 0.5, 3, 200],
            ],
            0,
            3,
        );
        expect(extent).toEqual({ min: 0.5, max: 9 });
    });

    it("returns null when nothing in range is finite", () => {
        expect(visibleExtent([[Number.NaN, Number.NaN]], 0, 2)).toBeNull();
    });
});

describe("padExtent", () => {
    it("adds a fraction of the span on both sides", () => {
        expect(padExtent({ min: 10, max: 20 }, 0.1, false)).toEqual({ min: 9, max: 21 });
    });

    it("pads in log space for a logarithmic axis", () => {
        const padded = padExtent({ min: 10, max: 1000 }, 0.1, true);
        expect(padded.min).toBeCloseTo(10 ** 0.8, 9);
        expect(padded.max).toBeCloseTo(10 ** 3.2, 6);
    });

    it("opens up a flat range so it can still be drawn", () => {
        const padded = padExtent({ min: 50, max: 50 }, 0.1, false);
        expect(padded.min).toBeLessThan(50);
        expect(padded.max).toBeGreaterThan(50);
    });
});
