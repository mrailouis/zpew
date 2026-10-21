export type Platform = "darwin" | "win32" | "linux";

export type ThemeSource = "system" | "dark" | "light";

export interface AppInfo {
    readonly version: string;
    readonly commit: string;
    readonly dirty: boolean;
    readonly manifestPath: string | null;
    readonly platform: Platform;
}

export interface WindowAppearance {
    readonly themeSource: ThemeSource;
    readonly titleBarColour: string;
    readonly titleBarSymbolColour: string;
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

export interface ChartSettings {
    readonly fastAverage: number;
    readonly slowAverage: number;
    readonly volatilityWindow: number;
}

export type WindowMode = "sliding" | "expanding";

export interface AnalysisSettings {
    readonly window: number;
    readonly refitEvery: number;
    readonly mode: WindowMode;
    readonly baselineWindow: number;
    readonly ewmaLambda: number;
    readonly includeGarch: boolean;
    readonly confidence: number;
}

export interface ChartGap {
    readonly lastBefore: number;
    readonly firstAfter: number;
    readonly missingSessions: number;
}

export interface ChartData {
    readonly symbol: string;
    readonly name: string;
    readonly calendar: string;
    readonly times: Float64Array;
    readonly open: Float64Array;
    readonly high: Float64Array;
    readonly low: Float64Array;
    readonly close: Float64Array;
    readonly volume: Float64Array;
    readonly fastAverage: Float64Array;
    readonly slowAverage: Float64Array;
    readonly volatilityPercent: Float64Array;
    readonly gaps: readonly ChartGap[];
}

export interface LossSummary {
    readonly observations: number;
    readonly mse: number;
    readonly qlike: number;
    readonly qlikeExcluded: number;
}

export interface Improvement {
    readonly msePercent: number;
    readonly qlikePercent: number;
}

export interface Backtest {
    readonly confidence: number;
    readonly observations: number;
    readonly breaches: number;
    readonly breachRate: number;
    readonly expectedRate: number;
    readonly kupiecLikelihoodRatio: number;
    readonly kupiecPValue: number;
}

export interface ModelAnalysis {
    readonly name: string;
    readonly failure: string | null;
    readonly refits: number;
    readonly boundaryRefits: number;
    readonly volatilityPercent: Float64Array;
    readonly varPrice: Float64Array;
    readonly breachTimes: Float64Array;
    readonly breachPrices: Float64Array;
    readonly losses: {
        readonly squaredReturn: LossSummary;
        readonly parkinson: LossSummary;
    } | null;
    readonly improvements: {
        readonly squaredReturn: Improvement | null;
        readonly parkinson: Improvement | null;
    };
    readonly backtest: Backtest | null;
}

export interface AnalysisData {
    readonly symbol: string;
    readonly baseline: string;
    readonly window: number;
    readonly refitEvery: number;
    readonly mode: WindowMode;
    readonly confidence: number;
    readonly seconds: number;
    readonly times: Float64Array;
    readonly models: readonly ModelAnalysis[];
}

export interface ZpewApi {
    appInfo(): Promise<AppInfo>;
    listAssets(): Promise<readonly AssetSummary[]>;
    loadChart(symbol: string, settings: ChartSettings): Promise<ChartData>;
    analyse(symbol: string, settings: AnalysisSettings): Promise<AnalysisData>;
    setWindowAppearance(appearance: WindowAppearance): Promise<void>;
}

export const IPC_CHANNELS = {
    appInfo: "zpew:app-info",
    listAssets: "zpew:list-assets",
    loadChart: "zpew:load-chart",
    analyse: "zpew:analyse",
    setWindowAppearance: "zpew:set-window-appearance",
} as const;

export const DEFAULT_CHART_SETTINGS: ChartSettings = {
    fastAverage: 50,
    slowAverage: 200,
    volatilityWindow: 21,
};

export const DEFAULT_ANALYSIS_SETTINGS: AnalysisSettings = {
    window: 1000,
    refitEvery: 20,
    mode: "sliding",
    baselineWindow: 21,
    ewmaLambda: 0.94,
    includeGarch: true,
    confidence: 0.95,
};
