import { describe, expect, it } from "vitest";

import { alignToBars, lowerBound } from "./series-index";

describe("lowerBound", () => {
    it("finds the first index whose value is not less than the target", () => {
        const sorted = [10, 20, 20, 30];
        expect(lowerBound(sorted, 5)).toBe(0);
        expect(lowerBound(sorted, 20)).toBe(1);
        expect(lowerBound(sorted, 25)).toBe(3);
        expect(lowerBound(sorted, 40)).toBe(4);
    });
});

describe("alignToBars", () => {
    it("places each value on the bar with the same time and fills the rest with NaN", () => {
        const aligned = alignToBars([100, 200, 300, 400], [300, 400], [0.5, 0.7]);
        expect(Number.isNaN(aligned[0])).toBe(true);
        expect(Number.isNaN(aligned[1])).toBe(true);
        expect(aligned[2]).toBe(0.5);
        expect(aligned[3]).toBe(0.7);
    });

    it("ignores values whose time is not a bar", () => {
        const aligned = alignToBars([100, 200], [150], [9]);
        expect(Array.from(aligned).every(Number.isNaN)).toBe(true);
    });
});
