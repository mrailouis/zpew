export function lowerBound(sorted: ArrayLike<number>, value: number): number {
    let low = 0;
    let high = sorted.length;
    while (low < high) {
        const middle = (low + high) >>> 1;
        if ((sorted[middle] ?? Number.POSITIVE_INFINITY) < value) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }
    return low;
}

export function alignToBars(
    barTimes: ArrayLike<number>,
    seriesTimes: ArrayLike<number>,
    values: ArrayLike<number>,
): Float64Array {
    const aligned = new Float64Array(barTimes.length).fill(Number.NaN);
    for (let i = 0; i < seriesTimes.length; i += 1) {
        const time = seriesTimes[i] ?? Number.NaN;
        const index = lowerBound(barTimes, time);
        if (index < barTimes.length && barTimes[index] === time) {
            aligned[index] = values[i] ?? Number.NaN;
        }
    }
    return aligned;
}
