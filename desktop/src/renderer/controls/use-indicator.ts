import { useLayoutEffect, useRef, useState } from "react";

import { indicatorFor, measureSegment, type Axis, type Indicator } from "./indicator";

export interface IndicatorState {
    readonly containerRef: React.RefObject<HTMLDivElement | null>;
    readonly indicator: Indicator | null;
    readonly animated: boolean;
}

export function useIndicator(index: number, axis: Axis, itemCount: number): IndicatorState {
    const containerRef = useRef<HTMLDivElement | null>(null);
    const [indicator, setIndicator] = useState<Indicator | null>(null);
    const [animated, setAnimated] = useState(false);

    useLayoutEffect(() => {
        const container = containerRef.current;
        if (container === null) {
            return;
        }
        const measure = () => {
            const items = Array.from(
                container.querySelectorAll<HTMLElement>(":scope > button"),
                (item) => measureSegment(item, axis),
            );
            setIndicator((previous) => indicatorFor(items, index, previous));
        };
        measure();
        const observer = new ResizeObserver(measure);
        observer.observe(container);
        const frame = requestAnimationFrame(() => {
            setAnimated(true);
        });
        return () => {
            observer.disconnect();
            cancelAnimationFrame(frame);
        };
    }, [index, axis, itemCount]);

    return { containerRef, indicator, animated };
}
