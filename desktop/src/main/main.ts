import { app, BrowserWindow, ipcMain, shell } from "electron";
import path from "node:path";

import { IPC_CHANNELS, type AppInfo } from "../shared/api";
import { loadNativeModule, type NativeModule } from "./native-module";

const INITIAL_WIDTH = 1600;
const INITIAL_HEIGHT = 1000;

function locateManifest(native: NativeModule): string | null {
    return (
        native.findManifest(process.cwd()) ?? native.findManifest(path.dirname(app.getPath("exe")))
    );
}

function requireManifest(manifestPath: string | null): string {
    if (manifestPath === null) {
        throw new Error(
            "No data/manifest.json found. Fetch data as described in README.md before starting zpew.",
        );
    }
    return manifestPath;
}

function registerHandlers(native: NativeModule, manifestPath: string | null): void {
    ipcMain.handle(IPC_CHANNELS.appInfo, (): AppInfo => {
        const build = native.buildInfo();
        return {
            version: native.version(),
            commit: build.commit,
            dirty: build.dirty,
            manifestPath,
        };
    });
    ipcMain.handle(IPC_CHANNELS.listAssets, () => native.listAssets(requireManifest(manifestPath)));
}

function createWindow(): void {
    const window = new BrowserWindow({
        width: INITIAL_WIDTH,
        height: INITIAL_HEIGHT,
        title: "zpew",
        backgroundColor: "#0b0d12",
        webPreferences: {
            preload: path.join(__dirname, "..", "preload", "preload.js"),
            contextIsolation: true,
            sandbox: true,
            nodeIntegration: false,
            webSecurity: true,
        },
    });
    window.webContents.setWindowOpenHandler(() => ({ action: "deny" }));
    window.webContents.on("will-navigate", (event) => {
        event.preventDefault();
    });
    void window.loadFile(path.join(__dirname, "..", "renderer", "index.html"));
}

void app.whenReady().then(() => {
    const native = loadNativeModule();
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
