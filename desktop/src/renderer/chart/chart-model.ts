import type { AnalysisData, ChartData } from "../../shared/api";
import { modelLabel } from "../format";
import { alignToBars, lowerBound } from "./series-index";

export type LineRole = "fast" | "slow" | "var" | "realised" | "forecast";

export interface OverlayLine {
    readonly label: string;
    readonly role: LineRole;
    readonly order: number;
    readonly values: Float64Array;
    readonly dashed: boolean;
}

export interface BreachMarkers {
    readonly label: string;
    readonly indices: readonly number[];
    readonly prices: readonly number[];
}

export interface ChartModel {
    readonly symbol: string;
    readonly times: Float64Array;
    readonly open: Float64Array;
    readonly high: Float64Array;
    readonly low: Float64Array;
    readonly close: Float64Array;
    readonly volume: Float64Array;
    readonly priceLines: readonly OverlayLine[];
    readonly breaches: BreachMarkers | null;
    readonly volatilityLines: readonly OverlayLine[];
}

export interface ChartModelOptions {
    readonly fastAverage: number;
    readonly slowAverage: number;
    readonly volatilityWindow: number;
    readonly varModel: string | null;
}

function breachMarkers(
    times: Float64Array,
    breachTimes: Float64Array,
    breachPrices: Float64Array,
    label: string,
): BreachMarkers {
    const indices: number[] = [];
    const prices: number[] = [];
    breachTimes.forEach((time, i) => {
        const index = lowerBound(times, time);
        if (times[index] === time) {
            indices.push(index);
            prices.push(breachPrices[i] ?? Number.NaN);
        }
    });
    return { label, indices, prices };
}

export function buildChartModel(
    chart: ChartData,
    analysis: AnalysisData | null,
    options: ChartModelOptions,
): ChartModel {
    const priceLines: OverlayLine[] = [
        {
            label: `SMA ${String(options.fastAverage)}`,
            role: "fast",
            order: 0,
            values: chart.fastAverage,
            dashed: false,
        },
        {
            label: `SMA ${String(options.slowAverage)}`,
            role: "slow",
            order: 0,
            values: chart.slowAverage,
            dashed: false,
        },
    ];
    const volatilityLines: OverlayLine[] = [
        {
            label: `${String(options.volatilityWindow)}d realised`,
            role: "realised",
            order: 0,
            values: chart.volatilityPercent,
            dashed: false,
        },
    ];
    let breaches: BreachMarkers | null = null;

    if (analysis !== null && analysis.symbol === chart.symbol) {
        analysis.models.forEach((model, order) => {
            if (model.failure !== null) {
                return;
            }
            volatilityLines.push({
                label: modelLabel(model.name),
                role: "forecast",
                order,
                values: alignToBars(chart.times, analysis.times, model.volatilityPercent),
                dashed: false,
            });
            if (model.name === options.varModel) {
                const percent = String(Math.round(analysis.confidence * 1000) / 10);
                priceLines.push({
                    label: `VaR ${percent}% ${modelLabel(model.name)}`,
                    role: "var",
                    order: 0,
                    values: alignToBars(chart.times, analysis.times, model.varPrice),
                    dashed: true,
                });
                breaches = breachMarkers(
                    chart.times,
                    model.breachTimes,
                    model.breachPrices,
                    `${String(model.breachTimes.length)} breaches`,
                );
            }
        });
    }

    return {
        symbol: chart.symbol,
        times: chart.times,
        open: chart.open,
        high: chart.high,
        low: chart.low,
        close: chart.close,
        volume: chart.volume,
        priceLines,
        breaches,
        volatilityLines,
    };
}
