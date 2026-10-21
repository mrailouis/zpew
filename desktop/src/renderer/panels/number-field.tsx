import { useState } from "react";

interface NumberFieldProps {
    readonly label: string;
    readonly value: number;
    readonly min: number;
    readonly max: number;
    readonly step: number;
    readonly integer: boolean;
    readonly onChange: (value: number) => void;
}

export function parseFieldValue(
    raw: string,
    min: number,
    max: number,
    integer: boolean,
): number | null {
    const parsed = Number(raw);
    const valid =
        raw.trim() !== "" && Number.isFinite(parsed) && (!integer || Number.isInteger(parsed));
    return valid ? Math.min(max, Math.max(min, parsed)) : null;
}

export function NumberField({ label, value, min, max, step, integer, onChange }: NumberFieldProps) {
    const [draft, setDraft] = useState<string | null>(null);

    const commit = (raw: string) => {
        setDraft(null);
        const parsed = parseFieldValue(raw, min, max, integer);
        if (parsed !== null && parsed !== value) {
            onChange(parsed);
        }
    };

    return (
        <label className="field">
            <span>{label}</span>
            <input
                type="number"
                value={draft ?? String(value)}
                min={min}
                max={max}
                step={step}
                onChange={(event) => {
                    setDraft(event.target.value);
                }}
                onBlur={(event) => {
                    commit(event.target.value);
                }}
                onKeyDown={(event) => {
                    if (event.key === "Enter") {
                        commit(event.currentTarget.value);
                    }
                }}
            />
        </label>
    );
}
