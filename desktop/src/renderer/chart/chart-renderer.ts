import { formatCompact, formatIsoDate, formatPercent, formatPrice } from "./chart-format";
import type { ChartModel, OverlayLine } from "./chart-model";
import { aggregateCandles, barsPerColumn } from "./decimate";
import {
    clipTo,
    drawBreaches,
    drawCandles,
    drawLine,
    drawValueGrid,
    drawVolumeBars,
    type BarToX,
} from "./draw-primitives";
import { padExtent, visibleExtent } from "./extent";
import { computeLayout, paneAt, type ChartLayout, type Rect } from "./layout";
import { logTicks, niceTicks } from "./nice-ticks";
import { THEME } from "./theme";
import { timeTicks } from "./time-ticks";
import { createValueScale, type ValueScale } from "./value-scale";
import { barToPixel, pixelToBar, visibleRange, type Viewport } from "./viewport";

export interface Crosshair {
    readonly x: number;
    readonly y: number;
}

export interface RenderState {
    readonly model: ChartModel;
    readonly viewport: Viewport;
    readonly logPrice: boolean;
    readonly crosshair: Crosshair | null;
}

interface PaneScales {
    readonly price: ValueScale | null;
    readonly volume: ValueScale | null;
    readonly volatility: ValueScale | null;
}

const TIME_LABEL_SPACING = 90;
const PIXELS_PER_TICK = 48;
const INSET = 6;

function paneScale(
    rect: Rect,
    series: readonly ArrayLike<number>[],
    start: number,
    end: number,
    logarithmic: boolean,
    floorAtZero: boolean,
): ValueScale | null {
    const extent = visibleExtent(series, start, end);
    if (extent === null || rect.height <= INSET * 2) {
        return null;
    }
    const useLog = logarithmic && extent.min > 0;
    const padded = padExtent(extent, 0.06, useLog);
    const min = floorAtZero ? Math.max(0, padded.min) : padded.min;
    return createValueScale(
        min,
        padded.max,
        rect.top + INSET,
        rect.top + rect.height - INSET,
        useLog,
    );
}

function ticksFor(scale: ValueScale, rect: Rect): number[] {
    const target = Math.max(2, Math.floor(rect.height / PIXELS_PER_TICK));
    return scale.logarithmic
        ? logTicks(scale.min, scale.max, target)
        : niceTicks(scale.min, scale.max, target);
}

function drawTimeGrid(
    ctx: CanvasRenderingContext2D,
    layout: ChartLayout,
    model: ChartModel,
    start: number,
    end: number,
    pixelsPerBar: number,
    x: BarToX,
): void {
    const ticks = timeTicks(model.times, start, end, pixelsPerBar, TIME_LABEL_SPACING);
    ctx.strokeStyle = THEME.grid;
    ctx.fillStyle = THEME.axisText;
    ctx.font = THEME.font;
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    for (const tick of ticks) {
        const px = Math.round(x(tick.index)) + 0.5;
        ctx.beginPath();
        ctx.moveTo(px, 0);
        ctx.lineTo(px, layout.timeAxis.top);
        ctx.stroke();
        ctx.fillText(tick.label, px, layout.timeAxis.top + layout.timeAxis.height / 2);
    }
}

function drawLegend(
    ctx: CanvasRenderingContext2D,
    rect: Rect,
    title: string,
    lines: readonly OverlayLine[],
    index: number,
    format: (value: number) => string,
): void {
    ctx.font = THEME.font;
    ctx.textBaseline = "top";
    ctx.textAlign = "left";
    let x = rect.left + 8;
    const y = rect.top + 6;
    ctx.fillStyle = THEME.text;
    ctx.fillText(title, x, y);
    x += ctx.measureText(title).width + 14;
    for (const line of lines) {
        const text = `${line.label} ${format(line.values[index] ?? Number.NaN)}`;
        ctx.fillStyle = line.colour;
        ctx.fillText(text, x, y);
        x += ctx.measureText(text).width + 14;
    }
}

function drawCrosshair(
    ctx: CanvasRenderingContext2D,
    layout: ChartLayout,
    scales: PaneScales,
    crosshair: Crosshair,
    px: number,
    date: string,
): void {
    ctx.strokeStyle = THEME.crosshair;
    ctx.setLineDash([3, 3]);
    ctx.beginPath();
    ctx.moveTo(Math.round(px) + 0.5, 0);
    ctx.lineTo(Math.round(px) + 0.5, layout.timeAxis.top);
    const pane = paneAt(layout, crosshair.y);
    if (pane !== null) {
        ctx.moveTo(layout.plotLeft, Math.round(crosshair.y) + 0.5);
        ctx.lineTo(layout.plotLeft + layout.plotWidth, Math.round(crosshair.y) + 0.5);
    }
    ctx.stroke();
    ctx.setLineDash([]);
    ctx.font = THEME.font;
    ctx.textBaseline = "middle";
    const scale = pane === null ? null : scales[pane];
    if (scale !== null) {
        const value = scale.toValue(crosshair.y);
        const label =
            pane === "volume"
                ? formatCompact(value)
                : pane === "volatility"
                  ? formatPercent(value)
                  : formatPrice(value);
        ctx.fillStyle = THEME.crosshair;
        ctx.fillRect(layout.plotWidth, crosshair.y - 9, layout.timeAxis.width, 18);
        ctx.fillStyle = THEME.text;
        ctx.textAlign = "left";
        ctx.fillText(label, layout.plotWidth + 8, crosshair.y);
    }
    ctx.fillStyle = THEME.crosshair;
    const labelWidth = ctx.measureText(date).width + 12;
    ctx.fillRect(
        px - labelWidth / 2,
        layout.timeAxis.top + 2,
        labelWidth,
        layout.timeAxis.height - 4,
    );
    ctx.fillStyle = THEME.text;
    ctx.textAlign = "center";
    ctx.fillText(date, px, layout.timeAxis.top + layout.timeAxis.height / 2);
}

export function renderChart(
    ctx: CanvasRenderingContext2D,
    width: number,
    height: number,
    state: RenderState,
): void {
    const { model, viewport } = state;
    const layout = computeLayout(width, height);
    ctx.fillStyle = THEME.background;
    ctx.fillRect(0, 0, width, height);
    const barCount = model.times.length;
    if (barCount === 0 || layout.plotWidth <= 0) {
        return;
    }
    const { start, end } = visibleRange(viewport, barCount);
    const pixelsPerBar = layout.plotWidth / viewport.count;
    const x: BarToX = (index) => barToPixel(index, viewport, layout.plotLeft, layout.plotWidth);
    const group = barsPerColumn(pixelsPerBar);
    const candles = aggregateCandles(model, start, end, group);

    const scales: PaneScales = {
        price: paneScale(
            layout.price,
            [model.high, model.low, ...model.priceLines.map((l) => l.values)],
            start,
            end,
            state.logPrice,
            true,
        ),
        volume: paneScale(layout.volume, [model.volume], start, end, false, true),
        volatility: paneScale(
            layout.volatility,
            model.volatilityLines.map((l) => l.values),
            start,
            end,
            false,
            true,
        ),
    };

    drawTimeGrid(ctx, layout, model, start, end, pixelsPerBar, x);

    if (scales.price !== null) {
        drawValueGrid(
            ctx,
            layout.price,
            scales.price,
            ticksFor(scales.price, layout.price),
            formatPrice,
        );
        ctx.save();
        clipTo(ctx, layout.price);
        drawCandles(ctx, candles, x, pixelsPerBar, scales.price);
        for (const line of model.priceLines) {
            drawLine(ctx, line.values, start, end, x, scales.price, line.colour, line.dashed);
        }
        if (model.breaches !== null) {
            drawBreaches(
                ctx,
                model.breaches.indices,
                model.breaches.prices,
                start,
                end,
                x,
                scales.price,
                pixelsPerBar,
            );
        }
        ctx.restore();
    }
    if (scales.volume !== null) {
        drawValueGrid(
            ctx,
            layout.volume,
            scales.volume,
            ticksFor(scales.volume, layout.volume),
            formatCompact,
        );
        ctx.save();
        clipTo(ctx, layout.volume);
        drawVolumeBars(ctx, candles, model.volume, x, pixelsPerBar, scales.volume);
        ctx.restore();
    }
    if (scales.volatility !== null) {
        drawValueGrid(
            ctx,
            layout.volatility,
            scales.volatility,
            ticksFor(scales.volatility, layout.volatility),
            (v) => formatPercent(v, 0),
        );
        ctx.save();
        clipTo(ctx, layout.volatility);
        for (const line of model.volatilityLines) {
            drawLine(ctx, line.values, start, end, x, scales.volatility, line.colour, line.dashed);
        }
        ctx.restore();
    }

    const hovered =
        state.crosshair === null
            ? end - 1
            : Math.min(
                  end - 1,
                  Math.max(
                      start,
                      pixelToBar(state.crosshair.x, viewport, layout.plotLeft, layout.plotWidth),
                  ),
              );
    const date = formatIsoDate(model.times[hovered] ?? 0);
    const ohlc = `O ${formatPrice(model.open[hovered] ?? Number.NaN)}  H ${formatPrice(model.high[hovered] ?? Number.NaN)}  L ${formatPrice(model.low[hovered] ?? Number.NaN)}  C ${formatPrice(model.close[hovered] ?? Number.NaN)}`;
    drawLegend(
        ctx,
        layout.price,
        `${model.symbol}  ${date}  ${ohlc}`,
        model.priceLines,
        hovered,
        formatPrice,
    );
    drawLegend(
        ctx,
        layout.volume,
        `volume ${formatCompact(model.volume[hovered] ?? Number.NaN)}`,
        [],
        hovered,
        formatCompact,
    );
    drawLegend(
        ctx,
        layout.volatility,
        "annualised volatility",
        model.volatilityLines,
        hovered,
        (v) => formatPercent(v),
    );
    if (state.crosshair !== null && state.crosshair.x < layout.plotWidth) {
        drawCrosshair(ctx, layout, scales, state.crosshair, x(hovered), date);
    }
}
