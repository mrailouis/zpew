import { describe, expect, it } from "vitest";

import { chartTheme, withAlpha } from "./chart-theme";
import { accentColour } from "./colour";
import { DEFAULT_PREFERENCES } from "./preferences";

describe("withAlpha", () => {
    it("converts a hex colour to rgba with the given alpha", () => {
        expect(withAlpha("#4c7dff", 0.5)).toBe("rgba(76, 125, 255, 0.5)");
    });
});

describe("chartTheme", () => {
    it("uses the chosen accent and candle palette", () => {
        const theme = chartTheme(
            { ...DEFAULT_PREFERENCES, accent: 173, palette: "colourblind" },
            true,
        );
        expect(theme.accent).toBe(accentColour(173));
        expect(theme.rising).toBe("#3b82f6");
        expect(theme.falling).toBe("#f59e0b");
    });

    it("switches every surface colour between dark and light", () => {
        const dark = chartTheme(DEFAULT_PREFERENCES, true);
        const light = chartTheme(DEFAULT_PREFERENCES, false);
        expect(dark.background).not.toBe(light.background);
        expect(dark.text).not.toBe(light.text);
        expect(dark.rising).not.toBe(light.rising);
    });

    it("leaves rising candle bodies hollow only in the hollow style", () => {
        expect(chartTheme({ ...DEFAULT_PREFERENCES, chartStyle: "hollow" }, true).risingFill).toBe(
            "transparent",
        );
        expect(chartTheme(DEFAULT_PREFERENCES, true).risingFill).toBe(
            chartTheme(DEFAULT_PREFERENCES, true).rising,
        );
    });

    it("follows the crosshair, grid and density preferences", () => {
        const theme = chartTheme(
            { ...DEFAULT_PREFERENCES, crosshair: "solid", showGrid: false, density: "compact" },
            true,
        );
        expect(theme.crosshairDash).toEqual([]);
        expect(theme.showGrid).toBe(false);
        expect(theme.font.startsWith("10.5px")).toBe(true);
    });
});
