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
import { useAppearance, usePreferences } from "./appearance/use-appearance";
import { buildChartModel } from "./chart/chart-model";
import type { RangeRequest } from "./chart/chart-view";
import { Inspector } from "./panels/inspector";
import { Sidebar } from "./panels/sidebar";
import { TopBar } from "./shell/top-bar";
import type { View } from "./shell/views";
import { ChartWorkspace } from "./views/chart-workspace";
import { DataView } from "./views/data-view";
import { ModelsView } from "./views/models-view";
import { SettingsView } from "./views/settings-view";

const ANALYSIS_DEBOUNCE_MS = 300;

function errorMessage(error: unknown): string {
    return error instanceof Error
        ? error.message.replace(/^Error invoking remote method '[^']+': (Error: )?/, "")
        : String(error);
}

export function App() {
    const [preferences, updatePreferences] = usePreferences();
    const theme = useAppearance(preferences);
    const [info, setInfo] = useState<AppInfo | null>(null);
    const [assets, setAssets] = useState<readonly AssetSummary[]>([]);
    const [startupError, setStartupError] = useState<string | null>(null);
    const [selected, setSelected] = useState<string | null>(null);
    const [view, setView] = useState<View>("chart");
    const [range, setRange] = useState<RangeRequest>({ preset: "1Y", serial: 0 });
    const [chartSettings, setChartSettings] = useState<ChartSettings>(DEFAULT_CHART_SETTINGS);
    const [analysisSettings, setAnalysisSettings] =
        useState<AnalysisSettings>(DEFAULT_ANALYSIS_SETTINGS);
    const [varModel, setVarModel] = useState<string | null>("garch");
    const [chart, setChart] = useState<ChartData | null>(null);
    const [chartError, setChartError] = useState<string | null>(null);
    const [analysis, setAnalysis] = useState<AnalysisData | null>(null);
    const [analysisError, setAnalysisError] = useState<string | null>(null);
    const [analysing, setAnalysing] = useState(false);
    const chartRequest = useRef(0);
    const analysisRequest = useRef(0);

    useEffect(() => {
        Promise.all([window.zpew.appInfo(), window.zpew.listAssets()])
            .then(([appInfo, list]) => {
                document.documentElement.dataset.platform = appInfo.platform;
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
    const inspectorOpen = preferences.showInspector && view !== "settings";

    const status = analysing
        ? currentAnalysis === null
            ? "Evaluating forecasts…"
            : "Updating forecasts…"
        : analysisError !== null
          ? `Forecast evaluation unavailable: ${analysisError}`
          : null;

    return (
        <div
            className={[
                "shell",
                preferences.showWatchlist ? "" : "sidebar-closed",
                inspectorOpen ? "" : "inspector-closed",
            ].join(" ")}
        >
            <Sidebar
                open={preferences.showWatchlist}
                assets={assets}
                selected={selected}
                settingsOpen={view === "settings"}
                preferences={preferences}
                onPreferences={updatePreferences}
                onSelect={(symbol) => {
                    setSelected(symbol);
                    setRange((current) => ({ ...current, serial: current.serial + 1 }));
                    if (view === "settings") {
                        setView("chart");
                    }
                }}
                onSettings={() => {
                    setView("settings");
                }}
            />
            <div className="main">
                <TopBar
                    view={view}
                    showSidebar={preferences.showWatchlist}
                    showInspector={inspectorOpen}
                    onView={setView}
                    onToggleSidebar={() => {
                        updatePreferences({ showWatchlist: true });
                    }}
                    onToggleInspector={() => {
                        updatePreferences({ showInspector: true });
                        if (view === "settings") {
                            setView("chart");
                        }
                    }}
                />
                <main className="content">
                    {startupError !== null && <p className="note error page">{startupError}</p>}
                    {view === "chart" && (
                        <ChartWorkspace
                            asset={asset}
                            model={model}
                            theme={theme}
                            preferences={preferences}
                            range={range}
                            error={chartError}
                            onRange={(preset) => {
                                setRange((current) => ({ preset, serial: current.serial + 1 }));
                            }}
                            onPreferences={updatePreferences}
                        />
                    )}
                    {view === "models" && <ModelsView analysis={currentAnalysis} status={status} />}
                    {view === "data" && <DataView asset={asset} chart={chart} theme={theme} />}
                    {view === "settings" && (
                        <SettingsView
                            info={info}
                            preferences={preferences}
                            chart={chartSettings}
                            onPreferences={updatePreferences}
                            onChart={setChartSettings}
                        />
                    )}
                </main>
            </div>
            <Inspector
                open={inspectorOpen}
                settings={analysisSettings}
                analysis={currentAnalysis}
                status={status}
                error={analysisError !== null}
                varModel={bandModel}
                onChange={setAnalysisSettings}
                onVarModelChange={setVarModel}
                onHide={() => {
                    updatePreferences({ showInspector: false });
                }}
            />
        </div>
    );
}
