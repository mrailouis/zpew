import type { ThemeSource } from "../../shared/api";
import { normaliseHue } from "./colour";

export const CHART_STYLES = ["candles", "hollow", "bars", "line", "area"] as const;
export const CANDLE_PALETTES = ["classic", "colourblind", "mono"] as const;
const LEGACY_ACCENTS = [
    { name: "Blue", hue: 211 },
    { name: "Violet", hue: 258 },
    { name: "Teal", hue: 173 },
    { name: "Amber", hue: 38 },
    { name: "Rose", hue: 350 },
] as const;
export const DENSITIES = ["compact", "comfortable"] as const;
export const CROSSHAIR_STYLES = ["dashed", "solid"] as const;
export const THEME_SOURCES = ["system", "dark", "light"] as const;

export type ChartStyle = (typeof CHART_STYLES)[number];
export type CandlePalette = (typeof CANDLE_PALETTES)[number];
export type Density = (typeof DENSITIES)[number];
export type CrosshairStyle = (typeof CROSSHAIR_STYLES)[number];

export interface Preferences {
    readonly theme: ThemeSource;
    readonly accent: number;
    readonly palette: CandlePalette;
    readonly chartStyle: ChartStyle;
    readonly density: Density;
    readonly crosshair: CrosshairStyle;
    readonly logPrice: boolean;
    readonly showGrid: boolean;
    readonly showVolume: boolean;
    readonly showVolatility: boolean;
    readonly showWatermark: boolean;
    readonly showWatchlist: boolean;
    readonly showInspector: boolean;
    readonly sidebarWidth: number;
    readonly collapsedGroups: readonly string[];
}

export const DEFAULT_PREFERENCES: Preferences = {
    theme: "system",
    accent: 211,
    palette: "classic",
    chartStyle: "candles",
    density: "comfortable",
    crosshair: "dashed",
    logPrice: false,
    showGrid: true,
    showVolume: true,
    showVolatility: true,
    showWatermark: false,
    showWatchlist: true,
    showInspector: true,
    sidebarWidth: 260,
    collapsedGroups: [],
};

export const SIDEBAR_MIN_WIDTH = 200;
export const SIDEBAR_MAX_WIDTH = 440;

export function clampSidebarWidth(width: number): number {
    if (!Number.isFinite(width)) {
        return DEFAULT_PREFERENCES.sidebarWidth;
    }
    return Math.round(Math.min(SIDEBAR_MAX_WIDTH, Math.max(SIDEBAR_MIN_WIDTH, width)));
}

const STORAGE_KEY = "zpew.preferences.v1";

function pick<T extends string>(options: readonly T[], value: unknown, fallback: T): T {
    return typeof value === "string" && (options as readonly string[]).includes(value)
        ? (value as T)
        : fallback;
}

function flag(value: unknown, fallback: boolean): boolean {
    return typeof value === "boolean" ? value : fallback;
}

function parseAccent(value: unknown): number {
    if (typeof value === "number" && Number.isFinite(value)) {
        return normaliseHue(value);
    }
    const preset = LEGACY_ACCENTS.find(
        (candidate) => typeof value === "string" && candidate.name.toLowerCase() === value,
    );
    return preset?.hue ?? DEFAULT_PREFERENCES.accent;
}

function strings(value: unknown): readonly string[] {
    return Array.isArray(value)
        ? value.filter((item): item is string => typeof item === "string")
        : [];
}

export function parsePreferences(value: unknown): Preferences {
    const record =
        typeof value === "object" && value !== null ? (value as Record<string, unknown>) : {};
    const d = DEFAULT_PREFERENCES;
    return {
        theme: pick(THEME_SOURCES, record.theme, d.theme),
        accent: parseAccent(record.accent),
        palette: pick(CANDLE_PALETTES, record.palette, d.palette),
        chartStyle: pick(CHART_STYLES, record.chartStyle, d.chartStyle),
        density: pick(DENSITIES, record.density, d.density),
        crosshair: pick(CROSSHAIR_STYLES, record.crosshair, d.crosshair),
        logPrice: flag(record.logPrice, d.logPrice),
        showGrid: flag(record.showGrid, d.showGrid),
        showVolume: flag(record.showVolume, d.showVolume),
        showVolatility: flag(record.showVolatility, d.showVolatility),
        showWatermark: flag(record.showWatermark, d.showWatermark),
        showWatchlist: flag(record.showWatchlist, d.showWatchlist),
        showInspector: flag(record.showInspector, d.showInspector),
        sidebarWidth:
            typeof record.sidebarWidth === "number"
                ? clampSidebarWidth(record.sidebarWidth)
                : d.sidebarWidth,
        collapsedGroups: strings(record.collapsedGroups),
    };
}

export function loadPreferences(storage: Pick<Storage, "getItem">): Preferences {
    const raw = storage.getItem(STORAGE_KEY);
    if (raw === null) {
        return DEFAULT_PREFERENCES;
    }
    try {
        return parsePreferences(JSON.parse(raw));
    } catch {
        return DEFAULT_PREFERENCES;
    }
}

export function savePreferences(storage: Pick<Storage, "setItem">, preferences: Preferences): void {
    storage.setItem(STORAGE_KEY, JSON.stringify(preferences));
}
