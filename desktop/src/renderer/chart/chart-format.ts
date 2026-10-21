export function formatPrice(value: number): string {
    if (!Number.isFinite(value)) {
        return "–";
    }
    const magnitude = Math.abs(value);
    const decimals = magnitude === 0 || magnitude >= 1 ? 2 : magnitude >= 0.01 ? 4 : 6;
    return value.toLocaleString("en-GB", {
        minimumFractionDigits: decimals,
        maximumFractionDigits: decimals,
    });
}

export function formatCompact(value: number): string {
    if (!Number.isFinite(value)) {
        return "–";
    }
    const units = [
        [1e12, "T"],
        [1e9, "B"],
        [1e6, "M"],
        [1e3, "K"],
    ] as const;
    for (const [size, suffix] of units) {
        if (Math.abs(value) >= size) {
            return `${(value / size).toFixed(2)}${suffix}`;
        }
    }
    return value.toFixed(0);
}

export function formatPercent(value: number, decimals = 2): string {
    return Number.isFinite(value) ? `${value.toFixed(decimals)}%` : "–";
}

export function formatIsoDate(secondsSinceEpoch: number): string {
    return new Date(secondsSinceEpoch * 1000).toISOString().slice(0, 10);
}
