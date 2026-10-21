import { describe, expect, it } from "vitest";

import { logTicks, niceStep, niceTicks } from "./nice-ticks";

describe("niceStep", () => {
    it("rounds the rough step up to 1, 2 or 5 times a power of ten", () => {
        expect(niceStep(10, 5)).toBe(2);
        expect(niceStep(9.5, 5)).toBe(2);
        expect(niceStep(1, 5)).toBeCloseTo(0.2, 15);
        expect(niceStep(700, 5)).toBe(200);
        expect(niceStep(45, 5)).toBe(10);
    });

    it("rejects a non-positive range", () => {
        expect(() => niceStep(0, 5)).toThrow("positive range");
    });
});

describe("niceTicks", () => {
    it("covers the range with evenly spaced nice values", () => {
        expect(niceTicks(0, 10, 5)).toEqual([0, 2, 4, 6, 8, 10]);
        expect(niceTicks(0.13, 0.87, 4)).toEqual([0.2, 0.4, 0.6, 0.8]);
        expect(niceTicks(-3, 3, 6)).toEqual([-3, -2, -1, 0, 1, 2, 3]);
    });

    it("avoids floating-point noise in tick values", () => {
        expect(niceTicks(0.1, 0.35, 5)).toEqual([0.1, 0.15, 0.2, 0.25, 0.3, 0.35]);
    });
});

describe("logTicks", () => {
    it("uses powers of ten across many decades", () => {
        expect(logTicks(1, 1000, 3)).toEqual([1, 10, 100, 1000]);
    });

    it("adds intermediate multiples within a single decade", () => {
        expect(logTicks(10, 100, 5)).toEqual([10, 20, 30, 50, 70, 100]);
    });

    it("falls back to linear ticks for very narrow ranges", () => {
        expect(logTicks(101, 104, 3)).toEqual([101, 102, 103, 104]);
    });
});
