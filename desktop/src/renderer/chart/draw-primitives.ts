import type { Candle } from "./decimate";
import type { Rect } from "./layout";
import { THEME } from "./theme";
import type { ValueScale } from "./value-scale";

export type BarToX = (index: number) => number;

export function clipTo(ctx: CanvasRenderingContext2D, rect: Rect): void {
    ctx.beginPath();
    ctx.rect(rect.left, rect.top, rect.width, rect.height);
    ctx.clip();
}

export function drawValueGrid(
    ctx: CanvasRenderingContext2D,
    rect: Rect,
    scale: ValueScale,
    ticks: readonly number[],
    format: (value: number) => string,
): void {
    ctx.strokeStyle = THEME.grid;
    ctx.lineWidth = 1;
    ctx.fillStyle = THEME.axisText;
    ctx.font = THEME.font;
    ctx.textBaseline = "middle";
    ctx.textAlign = "left";
    for (const tick of ticks) {
        const y = Math.round(scale.toPixel(tick)) + 0.5;
        if (y < rect.top || y > rect.top + rect.height) {
            continue;
        }
        ctx.beginPath();
        ctx.moveTo(rect.left, y);
        ctx.lineTo(rect.left + rect.width, y);
        ctx.stroke();
        ctx.fillText(format(tick), rect.left + rect.width + 8, y);
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
    dashed: boolean,
): void {
    ctx.strokeStyle = colour;
    ctx.lineWidth = 1.5;
    ctx.setLineDash(dashed ? [5, 4] : []);
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

export function drawCandles(
    ctx: CanvasRenderingContext2D,
    candles: readonly Candle[],
    x: BarToX,
    pixelsPerBar: number,
    scale: ValueScale,
): void {
    const groupWidth = (candles[0] ? candles[0].last - candles[0].first + 1 : 1) * pixelsPerBar;
    const bodyWidth = Math.max(1, Math.floor(groupWidth * 0.7));
    for (const candle of candles) {
        if (!Number.isFinite(candle.open) || !Number.isFinite(candle.close)) {
            continue;
        }
        const centre = Math.round((x(candle.first) + x(candle.last)) / 2);
        ctx.fillStyle = candle.close >= candle.open ? THEME.rising : THEME.falling;
        const high = scale.toPixel(candle.high);
        const low = scale.toPixel(candle.low);
        ctx.fillRect(centre, Math.round(high), 1, Math.max(1, Math.round(low - high)));
        if (bodyWidth >= 3) {
            const top = Math.round(scale.toPixel(Math.max(candle.open, candle.close)));
            const bottom = Math.round(scale.toPixel(Math.min(candle.open, candle.close)));
            ctx.fillRect(
                centre - Math.floor(bodyWidth / 2),
                top,
                bodyWidth,
                Math.max(1, bottom - top),
            );
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
): void {
    const groupWidth = (candles[0] ? candles[0].last - candles[0].first + 1 : 1) * pixelsPerBar;
    const width = Math.max(1, Math.floor(groupWidth * 0.7));
    const base = scale.toPixel(scale.min);
    for (const candle of candles) {
        let total = 0;
        for (let i = candle.first; i <= candle.last; i += 1) {
            total += volume[i] ?? 0;
        }
        const top = scale.toPixel(total / (candle.last - candle.first + 1));
        const centre = Math.round((x(candle.first) + x(candle.last)) / 2);
        ctx.fillStyle = candle.close >= candle.open ? THEME.volumeRising : THEME.volumeFalling;
        ctx.fillRect(
            centre - Math.floor(width / 2),
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
): void {
    const size = breachMarkerSize(pixelsPerBar);
    ctx.fillStyle = THEME.breach;
    indices.forEach((index, i) => {
        const price = prices[i];
        if (index < start || index >= end || price === undefined) {
            return;
        }
        const px = x(index);
        const py = scale.toPixel(price) + size + 2;
        ctx.beginPath();
        if (size < 3) {
            ctx.arc(px, py, size, 0, Math.PI * 2);
        } else {
            ctx.moveTo(px, py + size);
            ctx.lineTo(px - size * 0.8, py - size * 0.4);
            ctx.lineTo(px + size * 0.8, py - size * 0.4);
            ctx.closePath();
        }
        ctx.fill();
    });
}
