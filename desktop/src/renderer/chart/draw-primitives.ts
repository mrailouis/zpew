import type { ChartTheme } from "../appearance/chart-theme";
import type { ChartStyle } from "../appearance/preferences";
import type { Candle } from "./decimate";
import type { Rect } from "./layout";
import type { ValueScale } from "./value-scale";

export type BarToX = (index: number) => number;

export function clipTo(ctx: CanvasRenderingContext2D, rect: Rect): void {
    ctx.beginPath();
    ctx.rect(rect.left, rect.top, rect.width, rect.height);
    ctx.clip();
}

export function drawValueAxis(
    ctx: CanvasRenderingContext2D,
    rect: Rect,
    scale: ValueScale,
    ticks: readonly number[],
    format: (value: number) => string,
    theme: ChartTheme,
): void {
    ctx.font = theme.font;
    ctx.textBaseline = "middle";
    ctx.textAlign = "left";
    for (const tick of ticks) {
        const y = Math.round(scale.toPixel(tick)) + 0.5;
        if (y < rect.top + 4 || y > rect.top + rect.height - 4) {
            continue;
        }
        if (theme.showGrid) {
            ctx.strokeStyle = theme.grid;
            ctx.lineWidth = 1;
            ctx.beginPath();
            ctx.moveTo(rect.left, y);
            ctx.lineTo(rect.left + rect.width, y);
            ctx.stroke();
        }
        ctx.fillStyle = theme.axisText;
        ctx.fillText(format(tick), rect.left + rect.width + 10, y);
    }
}

export function drawLine(
    ctx: CanvasRenderingContext2D,
    values: ArrayLike<number>,
    start: number,
    end: number,
    x: BarToX,
    scale: ValueScale,
    colour: string,
    width: number,
    dash: readonly number[],
): void {
    ctx.strokeStyle = colour;
    ctx.lineWidth = width;
    ctx.lineJoin = "round";
    ctx.lineCap = "round";
    ctx.setLineDash(dash);
    ctx.beginPath();
    let drawing = false;
    for (let i = start; i < end; i += 1) {
        const value = values[i];
        const valid =
            value !== undefined && Number.isFinite(value) && (!scale.logarithmic || value > 0);
        if (!valid) {
            drawing = false;
            continue;
        }
        const px = x(i);
        const py = scale.toPixel(value);
        if (drawing) {
            ctx.lineTo(px, py);
        } else {
            ctx.moveTo(px, py);
            drawing = true;
        }
    }
    ctx.stroke();
    ctx.setLineDash([]);
}

export function drawArea(
    ctx: CanvasRenderingContext2D,
    values: ArrayLike<number>,
    start: number,
    end: number,
    x: BarToX,
    scale: ValueScale,
    rect: Rect,
    theme: ChartTheme,
): void {
    const gradient = ctx.createLinearGradient(0, rect.top, 0, rect.top + rect.height);
    gradient.addColorStop(0, theme.areaTop);
    gradient.addColorStop(1, theme.areaBottom);
    ctx.beginPath();
    let firstX: number | null = null;
    let lastX = 0;
    for (let i = start; i < end; i += 1) {
        const value = values[i];
        if (value === undefined || !Number.isFinite(value)) {
            continue;
        }
        lastX = x(i);
        const py = scale.toPixel(value);
        if (firstX === null) {
            firstX = lastX;
            ctx.moveTo(lastX, py);
        } else {
            ctx.lineTo(lastX, py);
        }
    }
    if (firstX === null) {
        return;
    }
    ctx.lineTo(lastX, rect.top + rect.height);
    ctx.lineTo(firstX, rect.top + rect.height);
    ctx.closePath();
    ctx.fillStyle = gradient;
    ctx.fill();
    drawLine(ctx, values, start, end, x, scale, theme.accent, 1.75, []);
}

function groupCentre(candle: Candle, x: BarToX): number {
    return Math.round((x(candle.first) + x(candle.last)) / 2);
}

function bodyWidth(candles: readonly Candle[], pixelsPerBar: number): number {
    const group = candles[0] ? candles[0].last - candles[0].first + 1 : 1;
    return Math.max(1, Math.floor(group * pixelsPerBar * 0.72));
}

export function drawPriceBars(
    ctx: CanvasRenderingContext2D,
    candles: readonly Candle[],
    x: BarToX,
    pixelsPerBar: number,
    scale: ValueScale,
    style: ChartStyle,
    theme: ChartTheme,
): void {
    const width = bodyWidth(candles, pixelsPerBar);
    for (const candle of candles) {
        if (!Number.isFinite(candle.open) || !Number.isFinite(candle.close)) {
            continue;
        }
        const rising = candle.close >= candle.open;
        const colour = rising ? theme.rising : theme.falling;
        const centre = groupCentre(candle, x);
        const high = Math.round(scale.toPixel(candle.high));
        const low = Math.round(scale.toPixel(candle.low));
        const open = Math.round(scale.toPixel(candle.open));
        const close = Math.round(scale.toPixel(candle.close));
        ctx.fillStyle = colour;
        ctx.fillRect(centre, high, 1, Math.max(1, low - high));
        if (style === "bars") {
            const arm = Math.max(1, Math.floor(width / 2));
            ctx.fillRect(centre - arm, open, arm, 1);
            ctx.fillRect(centre + 1, close, arm, 1);
            continue;
        }
        if (width < 3) {
            continue;
        }
        const top = Math.min(open, close);
        const height = Math.max(1, Math.abs(close - open));
        const left = centre - Math.floor(width / 2);
        const fill = rising ? theme.risingFill : theme.fallingFill;
        if (fill === "transparent") {
            ctx.fillStyle = theme.background;
            ctx.fillRect(left, top, width, height);
            ctx.strokeStyle = colour;
            ctx.lineWidth = 1;
            ctx.strokeRect(left + 0.5, top + 0.5, width - 1, Math.max(0, height - 1));
        } else {
            ctx.fillStyle = fill;
            ctx.fillRect(left, top, width, height);
        }
    }
}

export function drawVolumeBars(
    ctx: CanvasRenderingContext2D,
    candles: readonly Candle[],
    volume: ArrayLike<number>,
    x: BarToX,
    pixelsPerBar: number,
    scale: ValueScale,
    theme: ChartTheme,
): void {
    const width = bodyWidth(candles, pixelsPerBar);
    const base = scale.toPixel(scale.min);
    for (const candle of candles) {
        let total = 0;
        for (let i = candle.first; i <= candle.last; i += 1) {
            total += volume[i] ?? 0;
        }
        const top = scale.toPixel(total / (candle.last - candle.first + 1));
        ctx.fillStyle = candle.close >= candle.open ? theme.volumeRising : theme.volumeFalling;
        ctx.fillRect(
            groupCentre(candle, x) - Math.floor(width / 2),
            Math.round(top),
            width,
            Math.max(1, Math.round(base - top)),
        );
    }
}

export function breachMarkerSize(pixelsPerBar: number): number {
    return Math.min(5, Math.max(1.5, pixelsPerBar * 0.6));
}

export function drawBreaches(
    ctx: CanvasRenderingContext2D,
    indices: readonly number[],
    prices: readonly number[],
    start: number,
    end: number,
    x: BarToX,
    scale: ValueScale,
    pixelsPerBar: number,
    theme: ChartTheme,
): void {
    const size = breachMarkerSize(pixelsPerBar);
    ctx.fillStyle = theme.breach;
    indices.forEach((index, i) => {
        const price = prices[i];
        if (index < start || index >= end || price === undefined) {
            return;
        }
        const px = x(index);
        const py = scale.toPixel(price) + size + 3;
        ctx.beginPath();
        if (size < 3) {
            ctx.arc(px, py, size, 0, Math.PI * 2);
        } else {
            ctx.moveTo(px, py + size);
            ctx.lineTo(px - size * 0.85, py - size * 0.45);
            ctx.lineTo(px + size * 0.85, py - size * 0.45);
            ctx.closePath();
        }
        ctx.fill();
    });
}
