import { describe, expect, it } from "vitest";

import type { AnalysisData, ChartData, ModelAnalysis } from "../../shared/api";
import { buildChartModel } from "./chart-model";

const times = Float64Array.from([100, 200, 300, 400, 500]);
const series = (values: number[]) => Float64Array.from(values);

const chart: ChartData = {
    symbol: "SIM",
    name: "Simulated",
    calendar: "weekdays",
    times,
    open: series([1, 2, 3, 4, 5]),
    high: series([2, 3, 4, 5, 6]),
    low: series([0.5, 1, 2, 3, 4]),
    close: series([1.5, 2.5, 3.5, 4.5, 5.5]),
    volume: series([10, 20, 30, 40, 50]),
    fastAverage: series([Number.NaN, 2, 3, 4, 5]),
    slowAverage: series([Number.NaN, Number.NaN, 2.5, 3.5, 4.5]),
    volatilityPercent: series([Number.NaN, 10, 11, 12, 13]),
    gaps: [],
};

function model(name: string, failure: string | null = null): ModelAnalysis {
    return {
        name,
        failure,
        refits: 1,
        boundaryRefits: 0,
        volatilityPercent: series([20, 21]),
        varPrice: series([3.1, 4.1]),
        breachTimes: series([500]),
        breachPrices: series([5.5]),
        losses: null,
        improvements: { squaredReturn: null, parkinson: null },
        backtest: null,
    };
}

const analysis: AnalysisData = {
    symbol: "SIM",
    baseline: "rv21",
    window: 3,
    refitEvery: 1,
    mode: "sliding",
    confidence: 0.95,
    seconds: 0,
    times: series([400, 500]),
    models: [
        model("rv21"),
        model("ewma"),
        model("garch", "needs a window of at least 500 returns"),
    ],
};

const options = { fastAverage: 50, slowAverage: 200, volatilityWindow: 21, varModel: "ewma" };

describe("buildChartModel", () => {
    it("shows only moving averages and realised volatility without an analysis", () => {
        const result = buildChartModel(chart, null, options);
        expect(result.priceLines.map((l) => l.label)).toEqual(["SMA 50", "SMA 200"]);
        expect(result.volatilityLines.map((l) => l.label)).toEqual(["21-day realised"]);
        expect(result.breaches).toBeNull();
    });

    it("aligns forecasts to bars and skips models that failed", () => {
        const result = buildChartModel(chart, analysis, options);
        expect(result.volatilityLines.map((l) => l.label)).toEqual([
            "21-day realised",
            "forecast rv21",
            "forecast ewma",
        ]);
        const ewma = result.volatilityLines[2]?.values ?? new Float64Array();
        expect(Array.from(ewma.slice(0, 3)).every(Number.isNaN)).toBe(true);
        expect(Array.from(ewma.slice(3))).toEqual([20, 21]);
    });

    it("draws the chosen model's VaR band and marks its breaches on their bars", () => {
        const result = buildChartModel(chart, analysis, options);
        const band = result.priceLines[2];
        expect(band?.label).toBe("95% VaR (ewma)");
        expect(band?.dashed).toBe(true);
        expect(Array.from(band?.values.slice(3) ?? [])).toEqual([3.1, 4.1]);
        expect(result.breaches).toEqual({ label: "breaches (1)", indices: [4], prices: [5.5] });
    });

    it("ignores an analysis that belongs to a different symbol", () => {
        const result = buildChartModel(chart, { ...analysis, symbol: "OTHER" }, options);
        expect(result.volatilityLines).toHaveLength(1);
        expect(result.breaches).toBeNull();
    });
});
