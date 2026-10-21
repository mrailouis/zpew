interface IconProps {
    readonly size?: number;
}

function Svg({ size = 18, children }: IconProps & { readonly children: React.ReactNode }) {
    return (
        <svg
            width={size}
            height={size}
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            strokeWidth={1.6}
            strokeLinecap="round"
            strokeLinejoin="round"
            aria-hidden="true"
        >
            {children}
        </svg>
    );
}

export const SettingsIcon = (props: IconProps) => (
    <Svg {...props}>
        <path d="M4 7h10M18 7h2M4 17h2M10 17h10" />
        <circle cx="16" cy="7" r="2" />
        <circle cx="8" cy="17" r="2" />
    </Svg>
);

export const SidebarLeftIcon = (props: IconProps) => (
    <Svg {...props}>
        <rect x="3.5" y="4.5" width="17" height="15" rx="2.5" />
        <path d="M9 4.5v15" />
    </Svg>
);

export const SidebarRightIcon = (props: IconProps) => (
    <Svg {...props}>
        <rect x="3.5" y="4.5" width="17" height="15" rx="2.5" />
        <path d="M15 4.5v15" />
    </Svg>
);

export const SearchIcon = (props: IconProps) => (
    <Svg {...props}>
        <circle cx="11" cy="11" r="6" />
        <path d="m20 20-4.2-4.2" />
    </Svg>
);

export const ChevronIcon = (props: IconProps) => (
    <Svg {...props}>
        <path d="m6 9 6 6 6-6" />
    </Svg>
);
