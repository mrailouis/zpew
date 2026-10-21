import { useEffect, useMemo, useRef, useState } from "react";

import {
    DEFAULT_ANALYSIS_SETTINGS,
    DEFAULT_CHART_SETTINGS,
    type AnalysisData,
    type AnalysisSettings,
    type AppInfo,
    type AssetSummary,
    type ChartData,
    type ChartSettings,
} from "../shared/api";
import { buildChartModel } from "./chart/chart-model";
import { ChartView } from "./chart/chart-view";
import { formatCommit } from "./format";
import { ComparisonTable } from "./panels/comparison-table";
import { SettingsPanel } from "./panels/settings-panel";
import { SymbolPicker } from "./panels/symbol-picker";

type Tab = "chart" | "comparison";

const ANALYSIS_DEBOUNCE_MS = 300;

function errorMessage(error: unknown): string {
    return error instanceof Error
        ? error.message.replace(/^Error invoking remote method '[^']+': (Error: )?/, "")
        : String(error);
}

export function App() {
    const [info, setInfo] = useState<AppInfo | null>(null);
    const [assets, setAssets] = useState<readonly AssetSummary[]>([]);
    const [startupError, setStartupError] = useState<string | null>(null);
    const [selected, setSelected] = useState<string | null>(null);
    const [chartSettings, setChartSettings] = useState<ChartSettings>(DEFAULT_CHART_SETTINGS);
    const [analysisSettings, setAnalysisSettings] =
        useState<AnalysisSettings>(DEFAULT_ANALYSIS_SETTINGS);
    const [logPrice, setLogPrice] = useState(false);
    const [varModel, setVarModel] = useState<string | null>("garch");
    const [chart, setChart] = useState<ChartData | null>(null);
    const [chartError, setChartError] = useState<string | null>(null);
    const [analysis, setAnalysis] = useState<AnalysisData | null>(null);
    const [analysisError, setAnalysisError] = useState<string | null>(null);
    const [analysing, setAnalysing] = useState(false);
    const [tab, setTab] = useState<Tab>("chart");
    const chartRequest = useRef(0);
    const analysisRequest = useRef(0);

    useEffect(() => {
        Promise.all([window.zpew.appInfo(), window.zpew.listAssets()])
            .then(([appInfo, list]) => {
                setInfo(appInfo);
                setAssets(list);
                setSelected(list[0]?.symbol ?? null);
            })
            .catch((error: unknown) => {
                setStartupError(errorMessage(error));
            });
    }, []);

    useEffect(() => {
        if (selected === null) {
            return;
        }
        const id = ++chartRequest.current;
        window.zpew
            .loadChart(selected, chartSettings)
            .then((data) => {
                if (id === chartRequest.current) {
                    setChart(data);
                    setChartError(null);
                }
            })
            .catch((error: unknown) => {
                if (id === chartRequest.current) {
                    setChartError(errorMessage(error));
                }
            });
    }, [selected, chartSettings]);

    useEffect(() => {
        if (selected === null) {
            return;
        }
        const id = ++analysisRequest.current;
        const timer = setTimeout(() => {
            setAnalysing(true);
            window.zpew
                .analyse(selected, analysisSettings)
                .then((data) => {
                    if (id === analysisRequest.current) {
                        setAnalysis(data);
                        setAnalysisError(null);
                    }
                })
                .catch((error: unknown) => {
                    if (id === analysisRequest.current) {
                        setAnalysisError(errorMessage(error));
                    }
                })
                .finally(() => {
                    if (id === analysisRequest.current) {
                        setAnalysing(false);
                    }
                });
        }, ANALYSIS_DEBOUNCE_MS);
        return () => {
            clearTimeout(timer);
        };
    }, [selected, analysisSettings]);

    const currentAnalysis = analysis !== null && analysis.symbol === selected ? analysis : null;
    const modelNames = useMemo(
        () => currentAnalysis?.models.filter((m) => m.failure === null).map((m) => m.name) ?? [],
        [currentAnalysis],
    );
    const bandModel =
        varModel !== null && modelNames.includes(varModel) ? varModel : (modelNames.at(-1) ?? null);
    const model = useMemo(
        () =>
            chart === null
                ? null
                : buildChartModel(chart, currentAnalysis, {
                      ...chartSettings,
                      varModel: bandModel,
                  }),
        [chart, currentAnalysis, chartSettings, bandModel],
    );
    const asset = assets.find((a) => a.symbol === selected);

    const status = analysing
        ? currentAnalysis === null
            ? "evaluating forecasts…"
            : "updating forecasts…"
        : analysisError !== null
          ? `forecast evaluation unavailable: ${analysisError}`
          : null;

    return (
        <div className="app">
            <aside className="sidebar">
                <div className="brand">
                    zpew{" "}
                    <span className="muted">
                        {info ? `${info.version} · ${formatCommit(info.commit, info.dirty)}` : ""}
                    </span>
                </div>
                <SettingsPanel
                    chart={chartSettings}
                    analysis={analysisSettings}
                    logPrice={logPrice}
                    varModel={bandModel}
                    modelNames={modelNames}
                    onChartChange={setChartSettings}
                    onAnalysisChange={setAnalysisSettings}
                    onLogPriceChange={setLogPrice}
                    onVarModelChange={setVarModel}
                />
                <SymbolPicker assets={assets} selected={selected} onSelect={setSelected} />
            </aside>
            <main className="main">
                {startupError !== null && <div className="error">{startupError}</div>}
                {asset && (
                    <header className="header">
                        <span className="symbol">{asset.symbol}</span>
                        <span>{asset.name}</span>
                        {status !== null && (
                            <span className={analysisError ? "error" : "muted"}>{status}</span>
                        )}
                    </header>
                )}
                <nav className="tabs">
                    <button
                        type="button"
                        className={tab === "chart" ? "active" : ""}
                        onClick={() => {
                            setTab("chart");
                        }}
                    >
                        chart
                    </button>
                    <button
                        type="button"
                        className={tab === "comparison" ? "active" : ""}
                        onClick={() => {
                            setTab("comparison");
                        }}
                    >
                        model comparison
                    </button>
                </nav>
                {chartError !== null && <div className="error">{chartError}</div>}
                {tab === "chart" && model !== null && (
                    <ChartView model={model} logPrice={logPrice} />
                )}
                {tab === "comparison" &&
                    (currentAnalysis !== null ? (
                        <ComparisonTable analysis={currentAnalysis} />
                    ) : (
                        <p className="muted">{status ?? "no forecast evaluation yet"}</p>
                    ))}
            </main>
        </div>
    );
}
