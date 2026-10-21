import type { ChartTheme } from "../appearance/chart-theme";
import type { ChartStyle } from "../appearance/preferences";
import { formatCompact, formatLongDate, formatPercent, formatPrice } from "./chart-format";
import type { ChartModel } from "./chart-model";
import {
    drawCrosshair,
    drawLastPrice,
    drawWatermark,
    lineColour,
    type Crosshair,
    type PaneScales,
} from "./chart-overlays";
import { aggregateCandles, barsPerColumn } from "./decimate";
import {
    clipTo,
    drawArea,
    drawBreaches,
    drawLine,
    drawPriceBars,
    drawValueAxis,
    drawVolumeBars,
    type BarToX,
} from "./draw-primitives";
import { padExtent, visibleExtent } from "./extent";
import {
    AXIS_WIDTH,
    computeLayout,
    headerArea,
    PANE_NAMES,
    plotArea,
    type ChartLayout,
    type PaneVisibility,
    type Rect,
} from "./layout";
import { drawLegend, priceLegend, volatilityLegend, volumeLegend } from "./chart-legend";
import { logTicks, niceTicks } from "./nice-ticks";
import { timeTicks } from "./time-ticks";
import { createValueScale, type ValueScale } from "./value-scale";
import { barToPixel, pixelToBar, visibleRange, type Viewport } from "./viewport";

export type { Crosshair } from "./chart-overlays";

export interface RenderState {
    readonly model: ChartModel;
    readonly viewport: Viewport;
    readonly theme: ChartTheme;
    readonly style: ChartStyle;
    readonly logPrice: boolean;
    readonly panes: PaneVisibility;
    readonly watermark: boolean;
    readonly crosshair: Crosshair | null;
}

const TIME_LABEL_SPACING = 96;
const PIXELS_PER_TICK = 52;
const TOP_INSET = 10;
const BOTTOM_INSET = 8;

function paneScale(
    rect: Rect,
    series: readonly ArrayLike<number>[],
    start: number,
    end: number,
    logarithmic: boolean,
): ValueScale | null {
    const extent = visibleExtent(series, start, end);
    if (extent === null || rect.height <= TOP_INSET + BOTTOM_INSET) {
        return null;
    }
    const useLog = logarithmic && extent.min > 0;
    const padded = padExtent(extent, 0.05, useLog);
    return createValueScale(
        Math.max(0, padded.min),
        padded.max,
        rect.top + TOP_INSET,
        rect.top + rect.height - BOTTOM_INSET,
        useLog,
    );
}

function ticksFor(scale: ValueScale, rect: Rect): number[] {
    const target = Math.max(2, Math.floor(rect.height / PIXELS_PER_TICK));
    return scale.logarithmic
        ? logTicks(scale.min, scale.max, target)
        : niceTicks(scale.min, scale.max, target);
}

function drawTimeAxis(
    ctx: CanvasRenderingContext2D,
    layout: ChartLayout,
    state: RenderState,
    start: number,
    end: number,
    pixelsPerBar: number,
    x: BarToX,
): void {
    const ticks = timeTicks(state.model.times, start, end, pixelsPerBar, TIME_LABEL_SPACING);
    ctx.font = state.theme.font;
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    for (const tick of ticks) {
        const px = Math.round(x(tick.index)) + 0.5;
        if (state.theme.showGrid) {
            ctx.strokeStyle = state.theme.grid;
            ctx.beginPath();
            for (const name of PANE_NAMES) {
                const plot = plotArea(layout[name]);
                if (plot.height > 0) {
                    ctx.moveTo(px, plot.top);
                    ctx.lineTo(px, plot.top + plot.height);
                }
            }
            ctx.stroke();
        }
        const half = ctx.measureText(tick.label).width / 2;
        if (px - half < layout.plotLeft || px + half > layout.plotLeft + layout.plotWidth) {
            continue;
        }
        ctx.fillStyle = state.theme.axisText;
        ctx.fillText(tick.label, px, layout.timeAxis.top + layout.timeAxis.height / 2);
    }
}

function drawSeparators(
    ctx: CanvasRenderingContext2D,
    layout: ChartLayout,
    theme: ChartTheme,
): void {
    ctx.strokeStyle = theme.separator;
    ctx.lineWidth = 1;
    ctx.beginPath();
    for (const rect of [layout.price, layout.volume, layout.volatility]) {
        if (rect.height > 0) {
            const y = Math.round(rect.top + rect.height) + 0.5;
            ctx.moveTo(0, y);
            ctx.lineTo(layout.plotWidth + AXIS_WIDTH, y);
        }
    }
    ctx.moveTo(Math.round(layout.plotWidth) + 0.5, 0);
    ctx.lineTo(Math.round(layout.plotWidth) + 0.5, layout.timeAxis.top);
    ctx.stroke();
}

export function renderChart(
    ctx: CanvasRenderingContext2D,
    width: number,
    height: number,
    state: RenderState,
): void {
    const { model, viewport, theme } = state;
    const layout = computeLayout(width, height, state.panes);
    ctx.fillStyle = theme.background;
    ctx.fillRect(0, 0, width, height);
    const barCount = model.times.length;
    if (barCount === 0 || layout.plotWidth <= 0) {
        return;
    }
    const { start, end } = visibleRange(viewport, barCount);
    const pixelsPerBar = layout.plotWidth / viewport.count;
    const x: BarToX = (index) => barToPixel(index, viewport, layout.plotLeft, layout.plotWidth);
    const candles = aggregateCandles(model, start, end, barsPerColumn(pixelsPerBar));
    const lineStyle = state.style === "line" || state.style === "area";

    const plots = {
        price: plotArea(layout.price),
        volume: plotArea(layout.volume),
        volatility: plotArea(layout.volatility),
    };
    const scales: PaneScales = {
        price: paneScale(
            plots.price,
            lineStyle
                ? [model.close, ...model.priceLines.map((l) => l.values)]
                : [model.high, model.low, ...model.priceLines.map((l) => l.values)],
            start,
            end,
            state.logPrice,
        ),
        volume: state.panes.volume
            ? paneScale(plots.volume, [model.volume], start, end, false)
            : null,
        volatility: state.panes.volatility
            ? paneScale(
                  plots.volatility,
                  model.volatilityLines.map((l) => l.values),
                  start,
                  end,
                  false,
              )
            : null,
    };

    if (state.watermark) {
        drawWatermark(ctx, plots.price, model.symbol, theme);
    }
    drawTimeAxis(ctx, layout, state, start, end, pixelsPerBar, x);
    drawSeparators(ctx, layout, theme);

    if (scales.price !== null) {
        const scale = scales.price;
        drawValueAxis(ctx, plots.price, scale, ticksFor(scale, plots.price), formatPrice, theme);
        ctx.save();
        clipTo(ctx, plots.price);
        if (state.style === "area") {
            drawArea(ctx, model.close, start, end, x, scale, plots.price, theme);
        } else if (state.style === "line") {
            drawLine(ctx, model.close, start, end, x, scale, theme.accent, 1.75, []);
        } else {
            drawPriceBars(ctx, candles, x, pixelsPerBar, scale, state.style, theme);
        }
        for (const line of model.priceLines) {
            drawLine(
                ctx,
                line.values,
                start,
                end,
                x,
                scale,
                lineColour(line, theme),
                1.5,
                line.dashed ? [6, 4] : [],
            );
        }
        if (model.breaches !== null) {
            drawBreaches(
                ctx,
                model.breaches.indices,
                model.breaches.prices,
                start,
                end,
                x,
                scale,
                pixelsPerBar,
                theme,
            );
        }
        ctx.restore();
        drawLastPrice(
            ctx,
            plots.price,
            scale,
            model.close[barCount - 1] ?? Number.NaN,
            model.close[barCount - 2] ?? Number.NaN,
            theme,
        );
    }
    if (scales.volume !== null) {
        const scale = scales.volume;
        drawValueAxis(
            ctx,
            plots.volume,
            scale,
            ticksFor(scale, plots.volume),
            formatCompact,
            theme,
        );
        ctx.save();
        clipTo(ctx, plots.volume);
        drawVolumeBars(ctx, candles, model.volume, x, pixelsPerBar, scale, theme);
        ctx.restore();
    }
    if (scales.volatility !== null) {
        const scale = scales.volatility;
        drawValueAxis(
            ctx,
            plots.volatility,
            scale,
            ticksFor(scale, plots.volatility),
            (v) => formatPercent(v, 0),
            theme,
        );
        ctx.save();
        clipTo(ctx, plots.volatility);
        for (const line of model.volatilityLines) {
            drawLine(
                ctx,
                line.values,
                start,
                end,
                x,
                scale,
                lineColour(line, theme),
                line.role === "realised" ? 1.25 : 1.5,
                [],
            );
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
    const date = formatLongDate(model.times[hovered] ?? 0);
    drawLegend(ctx, headerArea(layout.price), priceLegend(model, hovered, theme), theme);
    drawLegend(ctx, headerArea(layout.volume), volumeLegend(model, hovered, theme), theme);
    drawLegend(ctx, headerArea(layout.volatility), volatilityLegend(model, hovered, theme), theme);
    if (state.crosshair !== null && state.crosshair.x < layout.plotWidth) {
        drawCrosshair(ctx, layout, scales, state.crosshair, x(hovered), date, theme);
    }
}
