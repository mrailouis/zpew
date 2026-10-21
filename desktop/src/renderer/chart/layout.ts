export interface Rect {
    readonly left: number;
    readonly top: number;
    readonly width: number;
    readonly height: number;
}

export interface ChartLayout {
    readonly price: Rect;
    readonly volume: Rect;
    readonly volatility: Rect;
    readonly timeAxis: Rect;
    readonly plotLeft: number;
    readonly plotWidth: number;
}

export const AXIS_WIDTH = 76;
export const TIME_AXIS_HEIGHT = 24;
const PANE_GAP = 6;
const PANE_SHARES = [0.6, 0.15, 0.25] as const;

export function computeLayout(width: number, height: number): ChartLayout {
    const plotWidth = Math.max(0, width - AXIS_WIDTH);
    const available = Math.max(0, height - TIME_AXIS_HEIGHT - PANE_GAP * 2);
    const heights = PANE_SHARES.map((share) => Math.floor(available * share));
    const priceHeight = heights[0] ?? 0;
    const volumeHeight = heights[1] ?? 0;
    const volatilityHeight = available - priceHeight - volumeHeight;
    const volumeTop = priceHeight + PANE_GAP;
    const volatilityTop = volumeTop + volumeHeight + PANE_GAP;
    return {
        price: { left: 0, top: 0, width: plotWidth, height: priceHeight },
        volume: { left: 0, top: volumeTop, width: plotWidth, height: volumeHeight },
        volatility: { left: 0, top: volatilityTop, width: plotWidth, height: volatilityHeight },
        timeAxis: {
            left: 0,
            top: volatilityTop + volatilityHeight,
            width: plotWidth,
            height: TIME_AXIS_HEIGHT,
        },
        plotLeft: 0,
        plotWidth,
    };
}

export function paneAt(layout: ChartLayout, y: number): "price" | "volume" | "volatility" | null {
    for (const name of ["price", "volume", "volatility"] as const) {
        const rect = layout[name];
        if (y >= rect.top && y < rect.top + rect.height) {
            return name;
        }
    }
    return null;
}
