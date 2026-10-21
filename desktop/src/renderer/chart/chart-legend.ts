import type { ChartTheme } from "../appearance/chart-theme";
import {
    formatChange,
    formatCompact,
    formatLongDate,
    formatPercent,
    formatPrice,
} from "./chart-format";
import type { ChartModel, OverlayLine } from "./chart-model";
import { lineColour } from "./chart-overlays";
import type { Rect } from "./layout";

export interface LegendItem {
    readonly label: string;
    readonly value: string;
    readonly valueColour: string;
    readonly swatch: string | null;
}

export type LegendGroup = readonly LegendItem[];

function item(label: string, value: string, theme: ChartTheme): LegendItem {
    return { label, value, valueColour: theme.text, swatch: null };
}

function lineItems(
    lines: readonly OverlayLine[],
    index: number,
    format: (value: number) => string,
    theme: ChartTheme,
): LegendItem[] {
    return lines.map((line) => ({
        label: line.label,
        value: format(line.values[index] ?? Number.NaN),
        valueColour: theme.text,
        swatch: lineColour(line, theme),
    }));
}

export function priceLegend(model: ChartModel, index: number, theme: ChartTheme): LegendGroup[] {
    const value = (series: Float64Array) => formatPrice(series[index] ?? Number.NaN);
    const change = (model.close[index] ?? Number.NaN) / (model.close[index - 1] ?? Number.NaN) - 1;
    const groups: LegendGroup[] = [
        [item("", formatLongDate(model.times[index] ?? 0), theme)],
        [
            item("O", value(model.open), theme),
            item("H", value(model.high), theme),
            item("L", value(model.low), theme),
            item("C", value(model.close), theme),
            {
                label: "",
                value: formatChange(change),
                valueColour: change >= 0 ? theme.rising : theme.falling,
                swatch: null,
            },
        ],
    ];
    if (model.priceLines.length > 0) {
        groups.push(lineItems(model.priceLines, index, formatPrice, theme));
    }
    return groups;
}

export function volumeLegend(model: ChartModel, index: number, theme: ChartTheme): LegendGroup[] {
    return [[item("Volume", formatCompact(model.volume[index] ?? Number.NaN), theme)]];
}

export function volatilityLegend(
    model: ChartModel,
    index: number,
    theme: ChartTheme,
): LegendGroup[] {
    return [lineItems(model.volatilityLines, index, (v) => formatPercent(v), theme)];
}

const ITEM_GAP = 12;
const GROUP_GAP = 22;
const LABEL_GAP = 5;
const SWATCH_WIDTH = 10;

export function drawLegend(
    ctx: CanvasRenderingContext2D,
    rect: Rect,
    groups: readonly LegendGroup[],
    theme: ChartTheme,
): void {
    if (rect.height <= 0) {
        return;
    }
    ctx.save();
    ctx.beginPath();
    ctx.rect(rect.left, rect.top, rect.width, rect.height);
    ctx.clip();
    ctx.textBaseline = "middle";
    ctx.textAlign = "left";
    const y = rect.top + rect.height / 2;
    let x = rect.left + 10;
    groups.forEach((group, g) => {
        if (g > 0) {
            x += GROUP_GAP - ITEM_GAP;
        }
        for (const entry of group) {
            if (entry.swatch !== null) {
                ctx.fillStyle = entry.swatch;
                ctx.beginPath();
                ctx.roundRect(x, y - 1, SWATCH_WIDTH, 2, 1);
                ctx.fill();
                x += SWATCH_WIDTH + 6;
            }
            if (entry.label !== "") {
                ctx.font = theme.font;
                ctx.fillStyle = theme.axisText;
                ctx.fillText(entry.label, x, y);
                x += ctx.measureText(entry.label).width + LABEL_GAP;
            }
            ctx.font = theme.boldFont;
            ctx.fillStyle = entry.valueColour;
            ctx.fillText(entry.value, x, y);
            x += ctx.measureText(entry.value).width + ITEM_GAP;
        }
    });
    ctx.restore();
}
