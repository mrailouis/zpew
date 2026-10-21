import { describe, expect, it } from "vitest";

import { accentColour, hslToHex, normaliseHue } from "./colour";

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
