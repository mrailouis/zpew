import { contextBridge, ipcRenderer } from "electron";

import {
    IPC_CHANNELS,
    type AnalysisData,
    type AppInfo,
    type AssetSummary,
    type ChartData,
    type ZpewApi,
} from "../shared/api";

const api: ZpewApi = {
    appInfo: () => ipcRenderer.invoke(IPC_CHANNELS.appInfo) as Promise<AppInfo>,
    listAssets: () =>
        ipcRenderer.invoke(IPC_CHANNELS.listAssets) as Promise<readonly AssetSummary[]>,
    loadChart: (symbol, settings) =>
        ipcRenderer.invoke(IPC_CHANNELS.loadChart, symbol, settings) as Promise<ChartData>,
    analyse: (symbol, settings) =>
        ipcRenderer.invoke(IPC_CHANNELS.analyse, symbol, settings) as Promise<AnalysisData>,
    setWindowAppearance: (appearance) =>
        ipcRenderer.invoke(IPC_CHANNELS.setWindowAppearance, appearance) as Promise<void>,
};

contextBridge.exposeInMainWorld("zpew", api);
