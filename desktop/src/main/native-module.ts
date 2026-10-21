import { app } from "electron";
import path from "node:path";

import type { AssetSummary } from "../shared/api";

export interface NativeModule {
    version(): string;
    buildInfo(): { commit: string; dirty: boolean };
    findManifest(start: string): string | null;
    listAssets(manifestPath: string): Promise<AssetSummary[]>;
}

function isNativeModule(value: unknown): value is NativeModule {
    if (typeof value !== "object" || value === null) {
        return false;
    }
    return ["version", "buildInfo", "findManifest", "listAssets"].every(
        (name) => typeof (value as Record<string, unknown>)[name] === "function",
    );
}

function addonPath(): string {
    const root = app.isPackaged
        ? app.getAppPath().replace(`app.asar`, `app.asar.unpacked`)
        : app.getAppPath();
    return path.join(root, "build", "native", "Release", "zpew_native.node");
}

export function loadNativeModule(): NativeModule {
    const holder: { exports: unknown } = { exports: {} };
    process.dlopen(holder, addonPath());
    if (!isNativeModule(holder.exports)) {
        throw new Error("zpew_native.node does not expose the expected functions");
    }
    return holder.exports;
}
