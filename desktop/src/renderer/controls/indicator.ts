export type Axis = "horizontal" | "vertical";

export interface Segment {
    readonly offset: number;
    readonly size: number;
}

export interface Indicator {
    readonly offset: number;
    readonly size: number;
    readonly visible: boolean;
}

export function measureSegment(element: HTMLElement, axis: Axis): Segment {
    return axis === "horizontal"
        ? { offset: element.offsetLeft, size: element.offsetWidth }
        : { offset: element.offsetTop, size: element.offsetHeight };
}

export function indicatorFor(
    segments: readonly Segment[],
    index: number,
    previous: Indicator | null,
): Indicator {
    const segment = segments[index];
    if (segment === undefined) {
        return { offset: previous?.offset ?? 0, size: previous?.size ?? 0, visible: false };
    }
    return { offset: segment.offset, size: segment.size, visible: true };
}
