import { describe, expect, it } from "vitest";

import { chartTheme } from "../appearance/chart-theme";
import { DEFAULT_PREFERENCES } from "../appearance/preferences";
import type { ChartModel } from "./chart-model";
import { priceLegend, volatilityLegend, volumeLegend } from "./chart-legend";

const theme = chartTheme(DEFAULT_PREFERENCES, true);
const day = (d: number) => Date.UTC(2026, 3, d) / 1000;

const model: ChartModel = {
    symbol: "SIM",
    times: Float64Array.from([day(21), day(22)]),
    open: Float64Array.from([100, 101]),
    high: Float64Array.from([102, 103]),
    low: Float64Array.from([99, 100]),
    close: Float64Array.from([100, 99]),
    volume: Float64Array.from([1500, 2500000]),
    priceLines: [
        {
            label: "SMA 50",
            role: "fast",
            order: 0,
            values: Float64Array.from([Number.NaN, 98.5]),
            dashed: false,
        },
    ],
    breaches: null,
    volatilityLines: [
        {
            label: "21d realised",
            role: "realised",
            order: 0,
            values: Float64Array.from([12, 12.5]),
            dashed: false,
        },
    ],
};

describe("priceLegend", () => {
    it("groups the date, OHLC with the day's change, and the overlays", () => {
        const groups = priceLegend(model, 1, theme);
        expect(groups.map((g) => g.map((i) => [i.label, i.value]))).toEqual([
            [["", "22 Apr 2026"]],
            [
                ["O", "101.00"],
                ["H", "103.00"],
                ["L", "100.00"],
                ["C", "99.00"],
                ["", "−1.00%"],
            ],
            [["SMA 50", "98.50"]],
        ]);
        expect(groups[1]?.[4]?.valueColour).toBe(theme.falling);
        expect(groups[2]?.[0]?.swatch).toBe(theme.fastAverage);
    });

    it("shows no change for the first bar and a dash for missing overlay values", () => {
        const groups = priceLegend(model, 0, theme);
        expect(groups[1]?.[4]?.value).toBe("–");
        expect(groups[2]?.[0]?.value).toBe("–");
    });
});

describe("volumeLegend and volatilityLegend", () => {
    it("label the hovered bar's volume and volatility lines", () => {
        expect(volumeLegend(model, 1, theme)).toEqual([
            [{ label: "Volume", value: "2.50M", valueColour: theme.text, swatch: null }],
        ]);
        expect(
            volatilityLegend(model, 1, theme)[0]?.map((i) => [i.label, i.value, i.swatch]),
        ).toEqual([["21d realised", "12.50%", theme.realised]]);
    });
});
