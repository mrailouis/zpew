export interface Rect {
    readonly left: number;
    readonly top: number;
    readonly width: number;
    readonly height: number;
}

export type PaneName = "price" | "volume" | "volatility";

export interface PaneVisibility {
    readonly volume: boolean;
    readonly volatility: boolean;
}

export interface ChartLayout {
    readonly price: Rect;
    readonly volume: Rect;
    readonly volatility: Rect;
    readonly timeAxis: Rect;
    readonly plotLeft: number;
    readonly plotWidth: number;
}

export const AXIS_WIDTH = 78;
export const TIME_AXIS_HEIGHT = 26;
const PANE_GAP = 10;
const SHARES: Readonly<Record<PaneName, number>> = { price: 0.6, volume: 0.15, volatility: 0.25 };

export function computeLayout(width: number, height: number, panes: PaneVisibility): ChartLayout {
    const plotWidth = Math.max(0, width - AXIS_WIDTH);
    const visible: PaneName[] = ["price"];
    if (panes.volume) {
        visible.push("volume");
    }
    if (panes.volatility) {
        visible.push("volatility");
    }
    const totalShare = visible.reduce((sum, name) => sum + SHARES[name], 0);
    const available = Math.max(0, height - TIME_AXIS_HEIGHT - PANE_GAP * (visible.length - 1));
    const rects: Record<PaneName, Rect> = {
        price: { left: 0, top: 0, width: plotWidth, height: 0 },
        volume: { left: 0, top: 0, width: plotWidth, height: 0 },
        volatility: { left: 0, top: 0, width: plotWidth, height: 0 },
    };
    let top = 0;
    visible.forEach((name, i) => {
        const last = i === visible.length - 1;
        const paneHeight = last
            ? Math.max(0, available - top + PANE_GAP * i)
            : Math.floor((available * SHARES[name]) / totalShare);
        rects[name] = { left: 0, top, width: plotWidth, height: paneHeight };
        top += paneHeight + PANE_GAP;
    });
    const axisTop = Math.max(0, height - TIME_AXIS_HEIGHT);
    return {
        price: rects.price,
        volume: rects.volume,
        volatility: rects.volatility,
        timeAxis: { left: 0, top: axisTop, width: plotWidth, height: TIME_AXIS_HEIGHT },
        plotLeft: 0,
        plotWidth,
    };
}

export const PANE_HEADER_HEIGHT = 26;
export const PANE_NAMES: readonly PaneName[] = ["price", "volume", "volatility"];

export function headerArea(rect: Rect): Rect {
    return { ...rect, height: Math.min(PANE_HEADER_HEIGHT, rect.height) };
}

export function plotArea(rect: Rect): Rect {
    const header = Math.min(PANE_HEADER_HEIGHT, rect.height);
    return { ...rect, top: rect.top + header, height: rect.height - header };
}

export function paneAt(layout: ChartLayout, y: number): PaneName | null {
    for (const name of PANE_NAMES) {
        const plot = plotArea(layout[name]);
        if (plot.height > 0 && y >= plot.top && y < plot.top + plot.height) {
            return name;
        }
    }
    return null;
}
