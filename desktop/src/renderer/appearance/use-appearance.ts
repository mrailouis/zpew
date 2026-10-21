import { useEffect, useState } from "react";

import { chartTheme, type ChartTheme } from "./chart-theme";
import { accentColour } from "./colour";
import { loadPreferences, savePreferences, type Preferences } from "./preferences";

const DARK_QUERY = "(prefers-color-scheme: dark)";

function systemDark(): boolean {
    return window.matchMedia(DARK_QUERY).matches;
}

export function usePreferences(): [Preferences, (patch: Partial<Preferences>) => void] {
    const [preferences, setPreferences] = useState<Preferences>(() =>
        loadPreferences(window.localStorage),
    );
    const update = (patch: Partial<Preferences>) => {
        setPreferences((current) => {
            const next = { ...current, ...patch };
            savePreferences(window.localStorage, next);
            return next;
        });
    };
    return [preferences, update];
}

export function useAppearance(preferences: Preferences): ChartTheme {
    const [dark, setDark] = useState(systemDark);

    useEffect(() => {
        const query = window.matchMedia(DARK_QUERY);
        const listener = () => {
            setDark(query.matches);
        };
        query.addEventListener("change", listener);
        void window.zpew
            .setWindowAppearance({
                themeSource: preferences.theme,
                titleBarColour: query.matches ? "#212121" : "#ffffff",
                titleBarSymbolColour: query.matches ? "#ececec" : "#0d0d0d",
            })
            .then(listener);
        return () => {
            query.removeEventListener("change", listener);
        };
    }, [preferences.theme, dark]);

    useEffect(() => {
        const root = document.documentElement;
        root.dataset.theme = dark ? "dark" : "light";
        root.dataset.density = preferences.density;
        root.style.setProperty("--accent", accentColour(preferences.accent));
    }, [dark, preferences.density, preferences.accent]);

    return chartTheme(preferences, dark);
}
