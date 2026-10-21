import { describe, expect, it } from "vitest";

import { parseWindowAppearance, windowOptions } from "./window-options";

describe("windowOptions", () => {
    it("insets the traffic lights on macOS", () => {
        const options = windowOptions("darwin", "/preload.js", true);
        expect(options.titleBarStyle).toBe("hiddenInset");
        expect(options.titleBarOverlay).toBeUndefined();
    });

    it("overlays native window controls in the theme's colours on Windows", () => {
        const options = windowOptions("win32", "/preload.js", false);
        expect(options.titleBarStyle).toBe("hidden");
        expect(options.titleBarOverlay).toEqual({
            color: "#ffffff",
            symbolColor: "#0d0d0d",
            height: 44,
        });
    });

    it("paints the window background in the theme colour before the page loads", () => {
        expect(windowOptions("darwin", "/preload.js", true).backgroundColor).toBe("#212121");
        expect(windowOptions("win32", "/preload.js", false).backgroundColor).toBe("#ffffff");
    });

    it("keeps the renderer sandboxed on every platform", () => {
        for (const platform of ["darwin", "win32", "linux"] as const) {
            const prefs = windowOptions(platform, "/preload.js", true).webPreferences;
            expect(prefs).toMatchObject({
                contextIsolation: true,
                sandbox: true,
                nodeIntegration: false,
            });
        }
    });
});

describe("parseWindowAppearance", () => {
    it("accepts a valid appearance", () => {
        expect(
            parseWindowAppearance({
                themeSource: "dark",
                titleBarColour: "#00000000",
                titleBarSymbolColour: "#e6e8ee",
            }),
        ).toEqual({
            themeSource: "dark",
            titleBarColour: "#00000000",
            titleBarSymbolColour: "#e6e8ee",
        });
    });

    it("rejects unknown themes and malformed colours", () => {
        expect(() =>
            parseWindowAppearance({
                themeSource: "neon",
                titleBarColour: "#000000",
                titleBarSymbolColour: "#ffffff",
            }),
        ).toThrow("themeSource");
        expect(() =>
            parseWindowAppearance({
                themeSource: "dark",
                titleBarColour: "red",
                titleBarSymbolColour: "#ffffff",
            }),
        ).toThrow("titleBarColour");
        expect(() => parseWindowAppearance(null)).toThrow("object");
    });
});
