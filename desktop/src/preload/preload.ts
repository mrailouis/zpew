import { contextBridge, ipcRenderer } from "electron";

import { IPC_CHANNELS, type AppInfo, type AssetSummary, type ZpewApi } from "../shared/api";

const api: ZpewApi = {
    appInfo: () => ipcRenderer.invoke(IPC_CHANNELS.appInfo) as Promise<AppInfo>,
    listAssets: () =>
        ipcRenderer.invoke(IPC_CHANNELS.listAssets) as Promise<readonly AssetSummary[]>,
};

contextBridge.exposeInMainWorld("zpew", api);
