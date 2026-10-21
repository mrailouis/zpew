import type { BrowserWindowConstructorOptions } from "electron";

import type { Platform, ThemeSource, WindowAppearance } from "../shared/api";

export const TITLE_BAR_HEIGHT = 44;
const INITIAL_WIDTH = 1600;
const INITIAL_HEIGHT = 1000;
const MINIMUM_WIDTH = 1080;
const MINIMUM_HEIGHT = 680;

export const WINDOW_BACKGROUNDS = { dark: "#212121", light: "#ffffff" } as const;

export function windowOptions(
    platform: Platform,
    preload: string,
    dark: boolean,
): BrowserWindowConstructorOptions {
    const common: BrowserWindowConstructorOptions = {
        width: INITIAL_WIDTH,
        height: INITIAL_HEIGHT,
        minWidth: MINIMUM_WIDTH,
        minHeight: MINIMUM_HEIGHT,
        title: "zpew",
        show: false,
        backgroundColor: dark ? WINDOW_BACKGROUNDS.dark : WINDOW_BACKGROUNDS.light,
        webPreferences: {
            preload,
            contextIsolation: true,
            sandbox: true,
            nodeIntegration: false,
            webSecurity: true,
            spellcheck: false,
        },
    };
    if (platform === "darwin") {
        return {
            ...common,
            titleBarStyle: "hiddenInset",
            trafficLightPosition: { x: 16, y: 15 },
        };
    }
    return {
        ...common,
        titleBarStyle: "hidden",
        titleBarOverlay: {
            color: dark ? WINDOW_BACKGROUNDS.dark : WINDOW_BACKGROUNDS.light,
            symbolColor: dark ? "#ececec" : "#0d0d0d",
            height: TITLE_BAR_HEIGHT,
        },
    };
}

const THEME_SOURCES: readonly ThemeSource[] = ["system", "dark", "light"];
const COLOUR = /^#[0-9a-fA-F]{6}([0-9a-fA-F]{2})?$/;

export function parseWindowAppearance(value: unknown): WindowAppearance {
    if (typeof value !== "object" || value === null) {
        throw new Error("window appearance must be an object");
    }
    const record = value as Record<string, unknown>;
    const themeSource = record.themeSource;
    const colour = record.titleBarColour;
    const symbol = record.titleBarSymbolColour;
    if (typeof themeSource !== "string" || !THEME_SOURCES.includes(themeSource as ThemeSource)) {
        throw new Error("themeSource must be system, dark or light");
    }
    if (typeof colour !== "string" || !COLOUR.test(colour)) {
        throw new Error("titleBarColour must be a hex colour");
    }
    if (typeof symbol !== "string" || !COLOUR.test(symbol)) {
        throw new Error("titleBarSymbolColour must be a hex colour");
    }
    return {
        themeSource: themeSource as ThemeSource,
        titleBarColour: colour,
        titleBarSymbolColour: symbol,
    };
}
