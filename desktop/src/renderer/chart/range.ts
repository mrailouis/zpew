import { lowerBound } from "./series-index";
import { clampViewport, fitViewport, type Viewport } from "./viewport";

export const RANGE_PRESETS = ["1M", "3M", "6M", "YTD", "1Y", "5Y", "MAX"] as const;
export type RangePreset = (typeof RANGE_PRESETS)[number];

const DAY = 86400;
const SPANS: Readonly<Record<Exclude<RangePreset, "YTD" | "MAX">, number>> = {
    "1M": 31 * DAY,
    "3M": 92 * DAY,
    "6M": 183 * DAY,
    "1Y": 365 * DAY,
    "5Y": 1826 * DAY,
};
const RIGHT_MARGIN_BARS = 3;

export function rangeStart(lastTime: number, preset: RangePreset): number | null {
    if (preset === "MAX") {
        return null;
    }
    if (preset === "YTD") {
        const year = new Date(lastTime * 1000).getUTCFullYear();
        return Date.UTC(year, 0, 1) / 1000;
    }
    return lastTime - SPANS[preset];
}

export function viewportForRange(times: ArrayLike<number>, preset: RangePreset): Viewport {
    const count = times.length;
    const start = count === 0 ? null : rangeStart(times[count - 1] ?? 0, preset);
    if (start === null) {
        return fitViewport(count);
    }
    const first = lowerBound(times, start);
    return clampViewport({ first, count: count - first + RIGHT_MARGIN_BARS }, count);
}
