interface TextToggleProps {
    readonly label: string;
    readonly checked: boolean;
    readonly onChange: (checked: boolean) => void;
}

export function TextToggle({ label, checked, onChange }: TextToggleProps) {
    return (
        <button
            type="button"
            aria-pressed={checked}
            className={checked ? "text-toggle active" : "text-toggle"}
            onClick={() => {
                onChange(!checked);
            }}
        >
            {label}
        </button>
    );
}
