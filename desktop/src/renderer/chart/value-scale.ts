export interface ValueScale {
    readonly min: number;
    readonly max: number;
    readonly logarithmic: boolean;
    toPixel(value: number): number;
    toValue(pixel: number): number;
}

export function createValueScale(
    min: number,
    max: number,
    top: number,
    bottom: number,
    logarithmic: boolean,
): ValueScale {
    if (!(max > min)) {
        throw new Error(`value scale needs max > min, got ${String(min)} and ${String(max)}`);
    }
    if (logarithmic && !(min > 0)) {
        throw new Error(`a logarithmic scale needs a positive minimum, got ${String(min)}`);
    }
    const forward = logarithmic ? Math.log10 : (v: number) => v;
    const inverse = logarithmic ? (v: number) => 10 ** v : (v: number) => v;
    const low = forward(min);
    const span = forward(max) - low;
    const height = bottom - top;
    return {
        min,
        max,
        logarithmic,
        toPixel: (value) => bottom - ((forward(value) - low) / span) * height,
        toValue: (pixel) => inverse(low + ((bottom - pixel) / height) * span),
    };
}
