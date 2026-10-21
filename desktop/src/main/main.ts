import { app, BrowserWindow, ipcMain, nativeTheme, shell } from "electron";
import path from "node:path";

import {
    IPC_CHANNELS,
    type AnalysisSettings,
    type AppInfo,
    type ChartSettings,
    type Platform,
} from "../shared/api";
import { manifestFromArguments } from "./manifest-location";
import { addonPath, loadNativeModule, type NativeModule } from "./native-module";
import { parseWindowAppearance, TITLE_BAR_HEIGHT, windowOptions } from "./window-options";

function currentPlatform(): Platform {
    return process.platform === "darwin" || process.platform === "win32"
        ? process.platform
        : "linux";
}

function locateManifest(native: NativeModule): string | null {
    const explicit = manifestFromArguments(process.argv) ?? process.env.ZPEW_MANIFEST ?? null;
    if (explicit !== null && explicit.length > 0) {
        return path.resolve(explicit);
    }
    return (
        native.findManifest(process.cwd()) ?? native.findManifest(path.dirname(app.getPath("exe")))
    );
}

function requireManifest(manifestPath: string | null): string {
    if (manifestPath === null) {
        throw new Error(
            "No data/manifest.json found. Fetch data as described in README.md, or start zpew with --manifest <path>.",
        );
    }
    return manifestPath;
}

function requireSymbol(symbol: unknown): string {
    if (typeof symbol !== "string" || symbol.length === 0) {
        throw new Error("symbol must be a non-empty string");
    }
    return symbol;
}

function appRoot(): string {
    return app.isPackaged
        ? app.getAppPath().replace("app.asar", "app.asar.unpacked")
        : app.getAppPath();
}

function registerHandlers(native: NativeModule, manifestPath: string | null): void {
    ipcMain.handle(IPC_CHANNELS.appInfo, (): AppInfo => {
        const build = native.buildInfo();
        return {
            version: native.version(),
            commit: build.commit,
            dirty: build.dirty,
            manifestPath,
            platform: currentPlatform(),
        };
    });
    ipcMain.handle(IPC_CHANNELS.listAssets, () => native.listAssets(requireManifest(manifestPath)));
    ipcMain.handle(IPC_CHANNELS.loadChart, (_event, symbol: unknown, settings: unknown) =>
        native.loadChart(
            requireManifest(manifestPath),
            requireSymbol(symbol),
            settings as ChartSettings,
        ),
    );
    ipcMain.handle(IPC_CHANNELS.analyse, (_event, symbol: unknown, settings: unknown) =>
        native.analyse(
            requireManifest(manifestPath),
            requireSymbol(symbol),
            settings as AnalysisSettings,
        ),
    );
    ipcMain.handle(IPC_CHANNELS.setWindowAppearance, (event, value: unknown) => {
        const appearance = parseWindowAppearance(value);
        nativeTheme.themeSource = appearance.themeSource;
        const window = BrowserWindow.fromWebContents(event.sender);
        if (window === null) {
            return;
        }
        window.setBackgroundColor(appearance.titleBarColour);
        if (currentPlatform() !== "darwin") {
            window.setTitleBarOverlay({
                color: appearance.titleBarColour,
                symbolColor: appearance.titleBarSymbolColour,
                height: TITLE_BAR_HEIGHT,
            });
        }
    });
}

function createWindow(): void {
    const window = new BrowserWindow(
        windowOptions(
            currentPlatform(),
            path.join(__dirname, "..", "preload", "preload.js"),
            nativeTheme.shouldUseDarkColors,
        ),
    );
    window.once("ready-to-show", () => {
        window.show();
    });
    window.webContents.setWindowOpenHandler(() => ({ action: "deny" }));
    window.webContents.on("will-navigate", (event) => {
        event.preventDefault();
    });
    void window.loadFile(path.join(__dirname, "..", "renderer", "index.html"));
}

void app.whenReady().then(() => {
    const native = loadNativeModule(addonPath(appRoot()));
    registerHandlers(native, locateManifest(native));
    createWindow();
    app.on("activate", () => {
        if (BrowserWindow.getAllWindows().length === 0) {
            createWindow();
        }
    });
});

app.on("window-all-closed", () => {
    if (process.platform !== "darwin") {
        app.quit();
    }
});

app.on("web-contents-created", (_event, contents) => {
    contents.setWindowOpenHandler(({ url }) => {
        if (url.startsWith("https://")) {
            void shell.openExternal(url);
        }
        return { action: "deny" };
    });
});
