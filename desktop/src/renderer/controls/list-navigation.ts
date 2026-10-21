export function stepIndex(current: number, delta: number, count: number): number {
    if (count <= 0) {
        return -1;
    }
    if (current < 0) {
        return delta >= 0 ? 0 : count - 1;
    }
    return (((current + delta) % count) + count) % count;
}

export function keyToIndex(key: string, current: number, count: number): number | null {
    switch (key) {
        case "ArrowDown":
            return stepIndex(current, 1, count);
        case "ArrowUp":
            return stepIndex(current, -1, count);
        case "Home":
            return count > 0 ? 0 : -1;
        case "End":
            return count - 1;
        default:
            return null;
    }
}
