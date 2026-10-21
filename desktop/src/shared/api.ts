export interface AppInfo {
    readonly version: string;
    readonly commit: string;
    readonly dirty: boolean;
    readonly manifestPath: string | null;
}

export interface AssetSummary {
    readonly symbol: string;
    readonly name: string;
    readonly assetClass: string;
    readonly calendar: string;
    readonly firstDate: string;
    readonly lastDate: string;
    readonly rows: number;
}

export interface ZpewApi {
    appInfo(): Promise<AppInfo>;
    listAssets(): Promise<readonly AssetSummary[]>;
}

export const IPC_CHANNELS = {
    appInfo: "zpew:app-info",
    listAssets: "zpew:list-assets",
} as const;
