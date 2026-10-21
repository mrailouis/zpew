interface SwitchProps {
    readonly label: string;
    readonly checked: boolean;
    readonly onChange: (checked: boolean) => void;
    readonly hint?: string;
}

export function Switch({ label, checked, onChange, hint }: SwitchProps) {
    return (
        <label className="switch-row">
            <span className="switch-text">
                <span>{label}</span>
                {hint !== undefined && <span className="hint">{hint}</span>}
            </span>
            <span className={checked ? "switch on" : "switch"}>
                <input
                    type="checkbox"
                    checked={checked}
                    onChange={(event) => {
                        onChange(event.target.checked);
                    }}
                />
                <span className="knob" />
            </span>
        </label>
    );
}
