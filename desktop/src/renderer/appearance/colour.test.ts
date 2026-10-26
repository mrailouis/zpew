import { describe, expect, it } from "vitest";

import { accentColour, hslToHex, normaliseHue, readableTextOn, relativeLuminance } from "./colour";

describe("normaliseHue", () => {
    it("wraps any angle into 0 to 359 whole degrees", () => {
        expect(normaliseHue(370)).toBe(10);
        expect(normaliseHue(-30)).toBe(330);
        expect(normaliseHue(359.6)).toBe(0);
    });
});

describe("hslToHex", () => {
    it("matches the standard HSL conversion for primary and mixed colours", () => {
        expect(hslToHex(0, 1, 0.5)).toBe("#ff0000");
        expect(hslToHex(120, 1, 0.25)).toBe("#008000");
        expect(hslToHex(240, 1, 0.5)).toBe("#0000ff");
        expect(hslToHex(60, 1, 0.5)).toBe("#ffff00");
        expect(hslToHex(300, 0.5, 0.5)).toBe("#bf40bf");
    });

    it("gives greys at zero saturation", () => {
        expect(hslToHex(200, 0, 0.5)).toBe("#808080");
        expect(hslToHex(10, 0, 1)).toBe("#ffffff");
    });
});

describe("accentColour", () => {
    it("is a valid hex colour that changes with the hue", () => {
        expect(accentColour(211)).toMatch(/^#[0-9a-f]{6}$/);
        expect(accentColour(211)).not.toBe(accentColour(30));
        expect(accentColour(0)).toBe(accentColour(360));
    });
});

describe("relativeLuminance", () => {
    it("follows the WCAG definition at the extremes and for pure green", () => {
        expect(relativeLuminance("#000000")).toBe(0);
        expect(relativeLuminance("#ffffff")).toBeCloseTo(1, 12);
        expect(relativeLuminance("#00ff00")).toBeCloseTo(0.7152, 12);
    });
});

describe("readableTextOn", () => {
    it("keeps white text on the classic, colour-blind and dark mono candle colours", () => {
        for (const colour of ["#26a69a", "#ef5350", "#3b82f6", "#f59e0b", "#1f2328", "#6b7280"]) {
            expect(readableTextOn(colour)).toBe("#ffffff");
        }
    });

    it("switches to dark text on the light mono candle colour", () => {
        expect(readableTextOn("#e6e8ee")).toBe("#0d0d0d");
    });
});
