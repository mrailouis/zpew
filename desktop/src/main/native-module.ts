import path from "node:path";

import type {
    AnalysisData,
    AnalysisSettings,
    AssetSummary,
    ChartData,
    ChartSettings,
} from "../shared/api";

export interface NativeModule {
    version(): string;
    buildInfo(): { commit: string; dirty: boolean };
    findManifest(start: string): string | null;
    listAssets(manifestPath: string): Promise<AssetSummary[]>;
    loadChart(manifestPath: string, symbol: string, settings: ChartSettings): Promise<ChartData>;
    analyse(
        manifestPath: string,
        symbol: string,
        settings: AnalysisSettings,
    ): Promise<AnalysisData>;
}

const REQUIRED_FUNCTIONS = [
    "version",
    "buildInfo",
    "findManifest",
    "listAssets",
    "loadChart",
    "analyse",
] as const;

function isNativeModule(value: unknown): value is NativeModule {
    if (typeof value !== "object" || value === null) {
        return false;
    }
    const record = value as Record<string, unknown>;
    return REQUIRED_FUNCTIONS.every((name) => typeof record[name] === "function");
}

export function addonPath(appRoot: string): string {
    return path.join(appRoot, "build", "native", "Release", "zpew_native.node");
}

export function loadNativeModule(file: string): NativeModule {
    const holder: { exports: unknown } = { exports: {} };
    process.dlopen(holder, file);
    if (!isNativeModule(holder.exports)) {
        throw new Error("zpew_native.node does not expose the expected functions");
    }
    return holder.exports;
}
