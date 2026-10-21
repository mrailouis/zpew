export interface Viewport {
    readonly first: number;
    readonly count: number;
}

export const MIN_VISIBLE_BARS = 10;
const RIGHT_MARGIN_BARS = 3;

export function clampViewport(viewport: Viewport, barCount: number): Viewport {
    const maxCount = Math.max(MIN_VISIBLE_BARS, barCount + RIGHT_MARGIN_BARS);
    const count = Math.min(Math.max(viewport.count, MIN_VISIBLE_BARS), maxCount);
    const minFirst = -count + MIN_VISIBLE_BARS;
    const maxFirst = barCount - MIN_VISIBLE_BARS;
    const first = Math.min(Math.max(viewport.first, minFirst), Math.max(minFirst, maxFirst));
    return { first, count };
}

export function fitViewport(barCount: number): Viewport {
    return clampViewport({ first: 0, count: barCount + RIGHT_MARGIN_BARS }, barCount);
}

export function zoomViewport(
    viewport: Viewport,
    factor: number,
    anchorFraction: number,
    barCount: number,
): Viewport {
    const anchorBar = viewport.first + anchorFraction * viewport.count;
    const count = viewport.count / factor;
    return clampViewport({ first: anchorBar - anchorFraction * count, count }, barCount);
}

export function panViewport(viewport: Viewport, deltaBars: number, barCount: number): Viewport {
    return clampViewport({ first: viewport.first + deltaBars, count: viewport.count }, barCount);
}

export function visibleRange(viewport: Viewport, barCount: number): { start: number; end: number } {
    const start = Math.max(0, Math.floor(viewport.first));
    const end = Math.min(barCount, Math.ceil(viewport.first + viewport.count));
    return { start, end: Math.max(start, end) };
}

export function barToPixel(index: number, viewport: Viewport, left: number, width: number): number {
    return left + ((index + 0.5 - viewport.first) / viewport.count) * width;
}

export function pixelToBar(pixel: number, viewport: Viewport, left: number, width: number): number {
    return Math.floor(viewport.first + ((pixel - left) / width) * viewport.count);
}
