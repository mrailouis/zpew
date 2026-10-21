const NICE_MULTIPLIERS = [1, 2, 5, 10] as const;

export function niceStep(range: number, targetCount: number): number {
    if (!(range > 0) || !(targetCount >= 1)) {
        throw new Error(`nice step needs a positive range and target, got ${String(range)}`);
    }
    const rough = range / targetCount;
    const magnitude = 10 ** Math.floor(Math.log10(rough));
    const multiplier = NICE_MULTIPLIERS.find((m) => m * magnitude >= rough) ?? 10;
    return multiplier * magnitude;
}

function roundToStep(value: number, step: number): number {
    const decimals = Math.max(0, -Math.floor(Math.log10(step)) + 1);
    return Number(value.toFixed(decimals));
}

export function niceTicks(min: number, max: number, targetCount: number): number[] {
    const step = niceStep(max - min, targetCount);
    const ticks: number[] = [];
    for (let i = Math.ceil(min / step); i * step <= max + step * 1e-9; i += 1) {
        ticks.push(roundToStep(i * step, step));
    }
    return ticks;
}

export function logTicks(min: number, max: number, targetCount: number): number[] {
    if (!(min > 0) || !(max > min)) {
        throw new Error(`log ticks need 0 < min < max, got ${String(min)} and ${String(max)}`);
    }
    const firstDecade = Math.floor(Math.log10(min));
    const lastDecade = Math.ceil(Math.log10(max));
    const decades = lastDecade - firstDecade;
    const mantissas =
        decades >= targetCount ? [1] : decades * 3 >= targetCount ? [1, 2, 5] : [1, 2, 3, 5, 7];
    const ticks: number[] = [];
    for (let decade = firstDecade; decade <= lastDecade; decade += 1) {
        for (const mantissa of mantissas) {
            const value = Number((mantissa * 10 ** decade).toPrecision(12));
            if (value >= min && value <= max) {
                ticks.push(value);
            }
        }
    }
    return ticks.length >= 2 ? ticks : niceTicks(min, max, targetCount);
}
