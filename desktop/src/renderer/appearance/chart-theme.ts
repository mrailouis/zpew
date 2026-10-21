import { accentColour } from "./colour";
import type { CandlePalette, Preferences } from "./preferences";

export interface ChartTheme {
    readonly background: string;
    readonly grid: string;
    readonly separator: string;
    readonly axisText: string;
    readonly text: string;
    readonly mutedText: string;
    readonly crosshair: string;
    readonly crosshairDash: readonly number[];
    readonly labelBackground: string;
    readonly labelText: string;
    readonly rising: string;
    readonly falling: string;
    readonly risingFill: string;
    readonly fallingFill: string;
    readonly volumeRising: string;
    readonly volumeFalling: string;
    readonly accent: string;
    readonly areaTop: string;
    readonly areaBottom: string;
    readonly varBand: string;
    readonly breach: string;
    readonly watermark: string;
    readonly fastAverage: string;
    readonly slowAverage: string;
    readonly realised: string;
    readonly forecasts: readonly string[];
    readonly font: string;
    readonly boldFont: string;
    readonly watermarkFont: string;
    readonly showGrid: boolean;
}

const CANDLE_COLOURS: Readonly<
    Record<CandlePalette, { dark: [string, string]; light: [string, string] }>
> = {
    classic: { dark: ["#26a69a", "#ef5350"], light: ["#089981", "#f23645"] },
    colourblind: { dark: ["#3b82f6", "#f59e0b"], light: ["#2563eb", "#d97706"] },
    mono: { dark: ["#e6e8ee", "#6b7280"], light: ["#1f2328", "#9ca3af"] },
};

const UI_FONT =
    '-apple-system, BlinkMacSystemFont, "Segoe UI Variable Text", "Segoe UI", system-ui, sans-serif';

export function withAlpha(hex: string, alpha: number): string {
    const value = hex.replace("#", "");
    const r = Number.parseInt(value.slice(0, 2), 16);
    const g = Number.parseInt(value.slice(2, 4), 16);
    const b = Number.parseInt(value.slice(4, 6), 16);
    return `rgba(${String(r)}, ${String(g)}, ${String(b)}, ${String(alpha)})`;
}

export function chartTheme(preferences: Preferences, dark: boolean): ChartTheme {
    const [rising, falling] = CANDLE_COLOURS[preferences.palette][dark ? "dark" : "light"];
    const accent = accentColour(preferences.accent);
    const fontSize = preferences.density === "compact" ? 10.5 : 11.5;
    return {
        background: dark ? "#212121" : "#ffffff",
        grid: dark ? "rgba(255, 255, 255, 0.04)" : "rgba(0, 0, 0, 0.05)",
        separator: dark ? "rgba(255, 255, 255, 0.07)" : "rgba(0, 0, 0, 0.08)",
        axisText: dark ? "#8f8f8f" : "#8f8f8f",
        text: dark ? "#ececec" : "#0d0d0d",
        mutedText: dark ? "#afafaf" : "#5d5d5d",
        crosshair: dark ? "rgba(236, 236, 236, 0.3)" : "rgba(13, 13, 13, 0.3)",
        crosshairDash: preferences.crosshair === "dashed" ? [4, 4] : [],
        labelBackground: dark ? "#424242" : "#2f2f2f",
        labelText: "#ffffff",
        rising,
        falling,
        risingFill: preferences.chartStyle === "hollow" ? "transparent" : rising,
        fallingFill: falling,
        volumeRising: withAlpha(rising, dark ? 0.35 : 0.3),
        volumeFalling: withAlpha(falling, dark ? 0.35 : 0.3),
        accent,
        areaTop: withAlpha(accent, dark ? 0.25 : 0.18),
        areaBottom: withAlpha(accent, 0),
        varBand: dark ? "rgba(244, 114, 182, 0.85)" : "rgba(219, 39, 119, 0.85)",
        breach: dark ? "#fb7185" : "#e11d48",
        watermark: dark ? "rgba(255, 255, 255, 0.03)" : "rgba(0, 0, 0, 0.035)",
        fastAverage: "#f5a524",
        slowAverage: dark ? "#7aa2ff" : "#3b6cf6",
        realised: dark ? "#c084fc" : "#9333ea",
        forecasts: dark
            ? ["#22d3ee", "#facc15", "#4ade80", "#fb923c"]
            : ["#0891b2", "#ca8a04", "#16a34a", "#ea580c"],
        font: `${String(fontSize)}px ${UI_FONT}`,
        boldFont: `500 ${String(fontSize)}px ${UI_FONT}`,
        watermarkFont: `600 72px ${UI_FONT}`,
        showGrid: preferences.showGrid,
    };
}
