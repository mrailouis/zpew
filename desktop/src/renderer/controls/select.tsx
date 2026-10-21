import { useEffect, useId, useRef, useState } from "react";

import { ChevronIcon } from "../shell/icons";
import { keyToIndex } from "./list-navigation";

interface SelectProps<T extends string> {
    readonly label: string;
    readonly options: readonly T[];
    readonly value: T;
    readonly onChange: (value: T) => void;
    readonly format: (value: T) => string;
}

function CheckIcon() {
    return (
        <svg
            width="14"
            height="14"
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            strokeWidth={2}
            strokeLinecap="round"
            strokeLinejoin="round"
            aria-hidden="true"
        >
            <path d="m5 12.5 4.5 4.5L19 7.5" />
        </svg>
    );
}

export function Select<T extends string>({
    label,
    options,
    value,
    onChange,
    format,
}: SelectProps<T>) {
    const [open, setOpen] = useState(false);
    const [active, setActive] = useState(-1);
    const rootRef = useRef<HTMLDivElement | null>(null);
    const listRef = useRef<HTMLUListElement | null>(null);
    const triggerRef = useRef<HTMLButtonElement | null>(null);
    const listId = useId();

    useEffect(() => {
        if (!open) {
            return;
        }
        const onPointerDown = (event: PointerEvent) => {
            if (!(event.target instanceof Node) || !rootRef.current?.contains(event.target)) {
                setOpen(false);
            }
        };
        document.addEventListener("pointerdown", onPointerDown);
        listRef.current?.focus();
        return () => {
            document.removeEventListener("pointerdown", onPointerDown);
        };
    }, [open]);

    const show = () => {
        setActive(options.indexOf(value));
        setOpen(true);
    };

    const close = () => {
        setOpen(false);
        triggerRef.current?.focus();
    };

    const choose = (option: T) => {
        onChange(option);
        close();
    };

    const onListKeyDown = (event: React.KeyboardEvent<HTMLUListElement>) => {
        const next = keyToIndex(event.key, active, options.length);
        if (next !== null) {
            event.preventDefault();
            setActive(next);
            return;
        }
        if (event.key === "Enter" || event.key === " ") {
            event.preventDefault();
            const option = options[active];
            if (option !== undefined) {
                choose(option);
            }
        } else if (event.key === "Escape") {
            event.preventDefault();
            close();
        } else if (event.key === "Tab") {
            setOpen(false);
        }
    };

    return (
        <div ref={rootRef} className={open ? "dropdown open" : "dropdown"}>
            <button
                ref={triggerRef}
                type="button"
                className="dropdown-trigger"
                aria-label={label}
                aria-haspopup="listbox"
                aria-expanded={open}
                aria-controls={listId}
                onClick={() => {
                    if (open) {
                        close();
                    } else {
                        show();
                    }
                }}
                onKeyDown={(event) => {
                    if (event.key === "ArrowDown" || event.key === "ArrowUp") {
                        event.preventDefault();
                        show();
                    }
                }}
            >
                <span>{format(value)}</span>
                <ChevronIcon size={14} />
            </button>
            <ul
                ref={listRef}
                id={listId}
                className="dropdown-menu"
                role="listbox"
                aria-label={label}
                aria-activedescendant={active >= 0 ? `${listId}-${String(active)}` : undefined}
                tabIndex={-1}
                inert={!open}
                onKeyDown={onListKeyDown}
            >
                {options.map((option, index) => (
                    <li
                        key={option}
                        id={`${listId}-${String(index)}`}
                        role="option"
                        aria-selected={option === value}
                        className={index === active ? "dropdown-option active" : "dropdown-option"}
                        onPointerEnter={() => {
                            setActive(index);
                        }}
                        onClick={() => {
                            choose(option);
                        }}
                    >
                        <span>{format(option)}</span>
                        {option === value && <CheckIcon />}
                    </li>
                ))}
            </ul>
        </div>
    );
}
