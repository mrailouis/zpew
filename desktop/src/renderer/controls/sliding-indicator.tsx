import type { Axis, Indicator } from "./indicator";

interface SlidingIndicatorProps {
    readonly indicator: Indicator | null;
    readonly axis: Axis;
}

export function SlidingIndicator({ indicator, axis }: SlidingIndicatorProps) {
    if (indicator === null) {
        return null;
    }
    const style =
        axis === "horizontal"
            ? { width: indicator.size, transform: `translateX(${String(indicator.offset)}px)` }
            : { height: indicator.size, transform: `translateY(${String(indicator.offset)}px)` };
    return (
        <span
            className="segment-indicator"
            aria-hidden="true"
            style={{ ...style, opacity: indicator.visible ? 1 : 0 }}
        />
    );
}
