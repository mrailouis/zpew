import { describe, expect, it } from "vitest";

import {
    clampSidebarWidth,
    DEFAULT_PREFERENCES,
    loadPreferences,
    parsePreferences,
    savePreferences,
} from "./preferences";

function memoryStorage(initial: Record<string, string> = {}) {
    const data = new Map(Object.entries(initial));
    return {
        getItem: (key: string) => data.get(key) ?? null,
        setItem: (key: string, value: string) => {
            data.set(key, value);
        },
    };
}

describe("parsePreferences", () => {
    it("keeps valid values and replaces invalid ones with defaults", () => {
        const parsed = parsePreferences({
            theme: "light",
            accent: "neon",
            chartStyle: "area",
            showGrid: "yes",
            showVolume: false,
        });
        expect(parsed.theme).toBe("light");
        expect(parsed.accent).toBe(DEFAULT_PREFERENCES.accent);
        expect(parsed.chartStyle).toBe("area");
        expect(parsed.showGrid).toBe(DEFAULT_PREFERENCES.showGrid);
        expect(parsed.showVolume).toBe(false);
    });

    it("returns the defaults for anything that is not an object", () => {
        expect(parsePreferences(null)).toEqual(DEFAULT_PREFERENCES);
        expect(parsePreferences("dark")).toEqual(DEFAULT_PREFERENCES);
    });
});

describe("loadPreferences and savePreferences", () => {
    it("round-trips preferences through storage", () => {
        const storage = memoryStorage();
        const preferences = {
            ...DEFAULT_PREFERENCES,
            accent: 173,
            density: "compact" as const,
        };
        savePreferences(storage, preferences);
        expect(loadPreferences(storage)).toEqual(preferences);
    });

    it("falls back to the defaults for missing or corrupt storage", () => {
        expect(loadPreferences(memoryStorage())).toEqual(DEFAULT_PREFERENCES);
        expect(loadPreferences(memoryStorage({ "zpew.preferences.v1": "{not json" }))).toEqual(
            DEFAULT_PREFERENCES,
        );
    });
});

describe("sidebar preferences", () => {
    it("clamps the sidebar width to its limits and rounds it", () => {
        expect(clampSidebarWidth(100)).toBe(200);
        expect(clampSidebarWidth(900)).toBe(440);
        expect(clampSidebarWidth(301.6)).toBe(302);
        expect(clampSidebarWidth(Number.NaN)).toBe(DEFAULT_PREFERENCES.sidebarWidth);
    });

    it("parses the width and collapsed groups, dropping invalid entries", () => {
        const parsed = parsePreferences({
            sidebarWidth: 1000,
            collapsedGroups: ["rates", 3, "crypto"],
        });
        expect(parsed.sidebarWidth).toBe(440);
        expect(parsed.collapsedGroups).toEqual(["rates", "crypto"]);
        expect(parsePreferences({ sidebarWidth: "wide", collapsedGroups: "rates" })).toMatchObject({
            sidebarWidth: DEFAULT_PREFERENCES.sidebarWidth,
            collapsedGroups: [],
        });
    });
});

describe("accent preference", () => {
    it("keeps any hue, wrapping it into range", () => {
        expect(parsePreferences({ accent: 95 }).accent).toBe(95);
        expect(parsePreferences({ accent: 400 }).accent).toBe(40);
    });

    it("migrates the old named accents to their hues", () => {
        expect(parsePreferences({ accent: "teal" }).accent).toBe(173);
        expect(parsePreferences({ accent: "rose" }).accent).toBe(350);
        expect(parsePreferences({ accent: "neon" }).accent).toBe(DEFAULT_PREFERENCES.accent);
    });
});
