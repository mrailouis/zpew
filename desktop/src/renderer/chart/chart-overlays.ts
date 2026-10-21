import type { ChartTheme } from "../appearance/chart-theme";
import { formatCompact, formatPercent, formatPrice } from "./chart-format";
import type { LineRole, OverlayLine } from "./chart-model";
import { PANE_NAMES, paneAt, plotArea, type ChartLayout, type PaneName, type Rect } from "./layout";
import type { ValueScale } from "./value-scale";

export interface Crosshair {
    readonly x: number;
    readonly y: number;
}

export type PaneScales = Readonly<Record<PaneName, ValueScale | null>>;

export function lineColour(line: OverlayLine, theme: ChartTheme): string {
    const roles: Readonly<Record<Exclude<LineRole, "forecast">, string>> = {
        fast: theme.fastAverage,
        slow: theme.slowAverage,
        var: theme.varBand,
        realised: theme.realised,
    };
    return line.role === "forecast"
        ? (theme.forecasts[line.order % theme.forecasts.length] ?? theme.realised)
        : roles[line.role];
}

export function drawPill(
    ctx: CanvasRenderingContext2D,
    text: string,
    x: number,
    y: number,
    background: string,
    foreground: string,
    theme: ChartTheme,
    align: "left" | "centre",
): void {
    ctx.font = theme.boldFont;
    const width = ctx.measureText(text).width + 12;
    const left = align === "centre" ? x - width / 2 : x;
    ctx.fillStyle = background;
    ctx.beginPath();
    ctx.roundRect(left, y - 9, width, 18, 4);
    ctx.fill();
    ctx.fillStyle = foreground;
    ctx.textAlign = "left";
    ctx.textBaseline = "middle";
    ctx.fillText(text, left + 6, y + 0.5);
}

export function drawWatermark(
    ctx: CanvasRenderingContext2D,
    rect: Rect,
    symbol: string,
    theme: ChartTheme,
): void {
    ctx.font = theme.watermarkFont;
    ctx.fillStyle = theme.watermark;
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    ctx.fillText(symbol, rect.left + rect.width / 2, rect.top + rect.height / 2);
}

export function drawLastPrice(
    ctx: CanvasRenderingContext2D,
    rect: Rect,
    scale: ValueScale,
    close: number,
    previous: number,
    theme: ChartTheme,
): void {
    if (!Number.isFinite(close)) {
        return;
    }
    const y = Math.round(scale.toPixel(close)) + 0.5;
    if (y < rect.top || y > rect.top + rect.height) {
        return;
    }
    const colour = close >= previous ? theme.rising : theme.falling;
    ctx.strokeStyle = colour;
    ctx.globalAlpha = 0.5;
    ctx.setLineDash([1, 3]);
    ctx.beginPath();
    ctx.moveTo(rect.left, y);
    ctx.lineTo(rect.left + rect.width, y);
    ctx.stroke();
    ctx.setLineDash([]);
    ctx.globalAlpha = 1;
    drawPill(
        ctx,
        formatPrice(close),
        rect.left + rect.width + 4,
        y,
        colour,
        theme.labelText,
        theme,
        "left",
    );
}

export function drawCrosshair(
    ctx: CanvasRenderingContext2D,
    layout: ChartLayout,
    scales: PaneScales,
    crosshair: Crosshair,
    px: number,
    date: string,
    theme: ChartTheme,
): void {
    ctx.strokeStyle = theme.crosshair;
    ctx.lineWidth = 1;
    ctx.setLineDash(theme.crosshairDash);
    ctx.beginPath();
    for (const name of PANE_NAMES) {
        const plot = plotArea(layout[name]);
        if (plot.height > 0) {
            ctx.moveTo(Math.round(px) + 0.5, plot.top);
            ctx.lineTo(Math.round(px) + 0.5, plot.top + plot.height);
        }
    }
    const pane = paneAt(layout, crosshair.y);
    if (pane !== null) {
        ctx.moveTo(layout.plotLeft, Math.round(crosshair.y) + 0.5);
        ctx.lineTo(layout.plotLeft + layout.plotWidth, Math.round(crosshair.y) + 0.5);
    }
    ctx.stroke();
    ctx.setLineDash([]);
    const scale = pane === null ? null : scales[pane];
    if (pane !== null && scale !== null) {
        const value = scale.toValue(crosshair.y);
        const label =
            pane === "volume"
                ? formatCompact(value)
                : pane === "volatility"
                  ? formatPercent(value)
                  : formatPrice(value);
        drawPill(
            ctx,
            label,
            layout.plotWidth + 4,
            crosshair.y,
            theme.labelBackground,
            theme.labelText,
            theme,
            "left",
        );
    }
    drawPill(
        ctx,
        date,
        px,
        layout.timeAxis.top + layout.timeAxis.height / 2,
        theme.labelBackground,
        theme.labelText,
        theme,
        "centre",
    );
}
