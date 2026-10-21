import { describe, expect, it } from "vitest";

import { chartTheme, withAlpha } from "../appearance/chart-theme";
import { DEFAULT_PREFERENCES } from "../appearance/preferences";
import { coverageColours } from "./coverage-colours";

describe("coverageColours", () => {
    it("fills trading days with the accent and highlights missing sessions like breaches", () => {
        const theme = chartTheme(DEFAULT_PREFERENCES, true);
        expect(coverageColours(theme)).toEqual({
            bar: withAlpha(theme.accent, 0.6),
            missing: theme.breach,
            closed: theme.separator,
        });
    });

    it("follows the theme between dark and light", () => {
        const dark = coverageColours(chartTheme(DEFAULT_PREFERENCES, true));
        const light = coverageColours(chartTheme(DEFAULT_PREFERENCES, false));
        expect(dark.closed).not.toBe(light.closed);
        expect(dark.missing).not.toBe(light.missing);
    });
});
