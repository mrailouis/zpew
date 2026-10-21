import { describe, expect, it } from "vitest";

import { createValueScale } from "./value-scale";

describe("createValueScale", () => {
    it("maps a linear domain onto pixels with larger values higher up", () => {
        const scale = createValueScale(0, 100, 0, 200, false);
        expect(scale.toPixel(0)).toBe(200);
        expect(scale.toPixel(100)).toBe(0);
        expect(scale.toPixel(25)).toBe(150);
        expect(scale.toValue(150)).toBe(25);
    });

    it("maps a logarithmic domain so each decade gets equal height", () => {
        const scale = createValueScale(1, 100, 0, 200, true);
        expect(scale.toPixel(10)).toBeCloseTo(100, 12);
        expect(scale.toValue(100)).toBeCloseTo(10, 12);
    });

    it("round-trips values through pixels", () => {
        const scale = createValueScale(3, 750, 20, 620, true);
        for (const value of [3, 17.5, 123.4, 750]) {
            expect(scale.toValue(scale.toPixel(value))).toBeCloseTo(value, 9);
        }
    });

    it("rejects empty and non-positive logarithmic domains", () => {
        expect(() => createValueScale(5, 5, 0, 100, false)).toThrow("max > min");
        expect(() => createValueScale(0, 10, 0, 100, true)).toThrow("positive minimum");
    });
});
