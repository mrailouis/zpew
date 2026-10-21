import { useEffect, useRef, useState } from "react";

import { accentColour } from "../appearance/colour";

interface HueSliderProps {
    readonly label: string;
    readonly hue: number;
    readonly onChange: (hue: number) => void;
}

export function HueSlider({ label, hue, onChange }: HueSliderProps) {
    const [draft, setDraft] = useState<number | null>(null);
    const pending = useRef<number | null>(null);
    const frame = useRef<number | null>(null);

    useEffect(
        () => () => {
            if (frame.current !== null) {
                cancelAnimationFrame(frame.current);
            }
        },
        [],
    );

    const flush = () => {
        frame.current = null;
        if (pending.current !== null) {
            onChange(pending.current);
            pending.current = null;
        }
    };

    const settle = () => {
        if (frame.current !== null) {
            cancelAnimationFrame(frame.current);
        }
        flush();
        setDraft(null);
    };

    const value = draft ?? hue;
    return (
        <input
            type="range"
            className="hue-slider"
            aria-label={label}
            min={0}
            max={359}
            step="any"
            value={value}
            style={{ "--thumb": accentColour(value) } as React.CSSProperties}
            onChange={(event) => {
                const next = Number(event.target.value);
                setDraft(next);
                pending.current = next;
                frame.current ??= requestAnimationFrame(flush);
            }}
            onPointerUp={settle}
            onKeyUp={settle}
            onBlur={settle}
        />
    );
}
