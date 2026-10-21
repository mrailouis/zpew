import { SlidingIndicator } from "./sliding-indicator";
import { useIndicator } from "./use-indicator";

interface SegmentedProps<T extends string> {
    readonly label: string;
    readonly options: readonly T[];
    readonly value: T | null;
    readonly onChange: (value: T) => void;
    readonly format?: (value: T) => string;
    readonly variant?: "switch" | "text";
}

export function Segmented<T extends string>({
    label,
    options,
    value,
    onChange,
    format,
    variant = "switch",
}: SegmentedProps<T>) {
    const index = value === null ? -1 : options.indexOf(value);
    const { containerRef, indicator, animated } = useIndicator(index, "horizontal", options.length);

    const classes = [variant === "text" ? "text-tabs" : "segmented", animated ? "animated" : ""];
    return (
        <div ref={containerRef} className={classes.join(" ")} role="radiogroup" aria-label={label}>
            <SlidingIndicator indicator={indicator} axis="horizontal" />
            {options.map((option) => (
                <button
                    key={option}
                    type="button"
                    role="radio"
                    aria-checked={option === value}
                    className={option === value ? "active" : ""}
                    onClick={() => {
                        onChange(option);
                    }}
                >
                    {format ? format(option) : option}
                </button>
            ))}
        </div>
    );
}
