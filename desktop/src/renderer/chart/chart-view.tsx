import { useCallback, useEffect, useRef } from "react";

import type { ChartTheme } from "../appearance/chart-theme";
import type { ChartStyle } from "../appearance/preferences";
import type { ChartModel } from "./chart-model";
import { renderChart, type Crosshair } from "./chart-renderer";
import { computeLayout, type PaneVisibility } from "./layout";
import { viewportForRange, type RangePreset } from "./range";
import { fitViewport, panViewport, zoomViewport, type Viewport } from "./viewport";

export interface RangeRequest {
    readonly preset: RangePreset;
    readonly serial: number;
}

interface ChartViewProps {
    readonly model: ChartModel;
    readonly theme: ChartTheme;
    readonly style: ChartStyle;
    readonly logPrice: boolean;
    readonly panes: PaneVisibility;
    readonly watermark: boolean;
    readonly range: RangeRequest;
}

const WHEEL_ZOOM_SPEED = 0.0025;
const KEY_ZOOM_FACTOR = 1.25;
const KEY_PAN_FRACTION = 0.1;

export function ChartView(props: ChartViewProps) {
    const { model, range } = props;
    const canvasRef = useRef<HTMLCanvasElement | null>(null);
    const viewportRef = useRef<Viewport>(viewportForRange(model.times, range.preset));
    const crosshairRef = useRef<Crosshair | null>(null);
    const dragRef = useRef<{ x: number; viewport: Viewport } | null>(null);
    const frameRef = useRef<number | null>(null);
    const stateRef = useRef(props);
    const rangeRef = useRef(range.serial);
    const symbolRef = useRef(model.symbol);

    const draw = useCallback(() => {
        frameRef.current = null;
        const canvas = canvasRef.current;
        const context = canvas?.getContext("2d");
        if (!canvas || !context) {
            return;
        }
        const ratio = window.devicePixelRatio;
        const width = canvas.clientWidth;
        const height = canvas.clientHeight;
        if (
            canvas.width !== Math.round(width * ratio) ||
            canvas.height !== Math.round(height * ratio)
        ) {
            canvas.width = Math.round(width * ratio);
            canvas.height = Math.round(height * ratio);
        }
        context.setTransform(ratio, 0, 0, ratio, 0, 0);
        const current = stateRef.current;
        renderChart(context, width, height, {
            model: current.model,
            theme: current.theme,
            style: current.style,
            logPrice: current.logPrice,
            panes: current.panes,
            watermark: current.watermark,
            viewport: viewportRef.current,
            crosshair: crosshairRef.current,
        });
    }, []);

    const requestDraw = useCallback(() => {
        frameRef.current ??= requestAnimationFrame(draw);
    }, [draw]);

    useEffect(() => {
        if (symbolRef.current !== model.symbol || rangeRef.current !== range.serial) {
            symbolRef.current = model.symbol;
            rangeRef.current = range.serial;
            viewportRef.current = viewportForRange(model.times, range.preset);
        }
        stateRef.current = props;
        requestDraw();
    }, [props, model, range, requestDraw]);

    useEffect(() => {
        const canvas = canvasRef.current;
        if (!canvas) {
            return;
        }
        const observer = new ResizeObserver(requestDraw);
        observer.observe(canvas);
        const ratioQuery = matchMedia(`(resolution: ${String(window.devicePixelRatio)}dppx)`);
        ratioQuery.addEventListener("change", requestDraw);
        return () => {
            observer.disconnect();
            ratioQuery.removeEventListener("change", requestDraw);
            if (frameRef.current !== null) {
                cancelAnimationFrame(frameRef.current);
            }
        };
    }, [requestDraw]);

    const plotWidth = () =>
        computeLayout(canvasRef.current?.clientWidth ?? 0, 0, stateRef.current.panes).plotWidth;
    const barCount = () => stateRef.current.model.times.length;

    const setViewport = (viewport: Viewport) => {
        viewportRef.current = viewport;
        requestDraw();
    };

    const onWheel = (event: React.WheelEvent<HTMLCanvasElement>) => {
        const width = plotWidth();
        if (width <= 0) {
            return;
        }
        if (Math.abs(event.deltaX) > Math.abs(event.deltaY) && !event.ctrlKey) {
            setViewport(
                panViewport(
                    viewportRef.current,
                    (event.deltaX / width) * viewportRef.current.count,
                    barCount(),
                ),
            );
            return;
        }
        const anchor = Math.min(1, Math.max(0, event.nativeEvent.offsetX / width));
        const factor = Math.exp(-event.deltaY * WHEEL_ZOOM_SPEED * (event.ctrlKey ? 4 : 1));
        setViewport(zoomViewport(viewportRef.current, factor, anchor, barCount()));
    };

    const onPointerDown = (event: React.PointerEvent<HTMLCanvasElement>) => {
        event.currentTarget.setPointerCapture(event.pointerId);
        dragRef.current = { x: event.nativeEvent.offsetX, viewport: viewportRef.current };
    };

    const onPointerMove = (event: React.PointerEvent<HTMLCanvasElement>) => {
        crosshairRef.current = { x: event.nativeEvent.offsetX, y: event.nativeEvent.offsetY };
        const drag = dragRef.current;
        const width = plotWidth();
        if (drag !== null && width > 0) {
            const deltaBars = ((drag.x - event.nativeEvent.offsetX) / width) * drag.viewport.count;
            viewportRef.current = panViewport(drag.viewport, deltaBars, barCount());
        }
        requestDraw();
    };

    const onPointerUp = (event: React.PointerEvent<HTMLCanvasElement>) => {
        event.currentTarget.releasePointerCapture(event.pointerId);
        dragRef.current = null;
    };

    const onPointerLeave = () => {
        crosshairRef.current = null;
        requestDraw();
    };

    const onKeyDown = (event: React.KeyboardEvent<HTMLCanvasElement>) => {
        const viewport = viewportRef.current;
        const pan = viewport.count * KEY_PAN_FRACTION;
        const actions: Record<string, () => Viewport> = {
            ArrowLeft: () => panViewport(viewport, -pan, barCount()),
            ArrowRight: () => panViewport(viewport, pan, barCount()),
            "+": () => zoomViewport(viewport, KEY_ZOOM_FACTOR, 1, barCount()),
            "=": () => zoomViewport(viewport, KEY_ZOOM_FACTOR, 1, barCount()),
            "-": () => zoomViewport(viewport, 1 / KEY_ZOOM_FACTOR, 1, barCount()),
            Home: () => fitViewport(barCount()),
        };
        const action = actions[event.key];
        if (action !== undefined) {
            event.preventDefault();
            setViewport(action());
        }
    };

    return (
        <canvas
            ref={canvasRef}
            className="chart-canvas"
            tabIndex={0}
            aria-label={`${model.symbol} price, volume and volatility chart`}
            onWheel={onWheel}
            onPointerDown={onPointerDown}
            onPointerMove={onPointerMove}
            onPointerUp={onPointerUp}
            onPointerLeave={onPointerLeave}
            onDoubleClick={() => {
                setViewport(fitViewport(barCount()));
            }}
            onKeyDown={onKeyDown}
        />
    );
}
