export interface Extent {
    readonly min: number;
    readonly max: number;
}

export function visibleExtent(
    series: readonly ArrayLike<number>[],
    start: number,
    end: number,
): Extent | null {
    let min = Number.POSITIVE_INFINITY;
    let max = Number.NEGATIVE_INFINITY;
    for (const values of series) {
        for (let i = start; i < end; i += 1) {
            const value = values[i];
            if (value !== undefined && Number.isFinite(value)) {
                min = Math.min(min, value);
                max = Math.max(max, value);
            }
        }
    }
    return min <= max ? { min, max } : null;
}

export function padExtent(extent: Extent, fraction: number, logarithmic: boolean): Extent {
    if (logarithmic) {
        const low = Math.log10(extent.min);
        const high = Math.log10(extent.max);
        const pad = Math.max((high - low) * fraction, 0.005);
        return { min: 10 ** (low - pad), max: 10 ** (high + pad) };
    }
    const span = extent.max - extent.min;
    const pad = span > 0 ? span * fraction : Math.max(Math.abs(extent.max) * 0.01, 1e-12);
    return { min: extent.min - pad, max: extent.max + pad };
}
