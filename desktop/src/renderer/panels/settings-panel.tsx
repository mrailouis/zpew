import type { AnalysisSettings, ChartSettings, WindowMode } from "../../shared/api";
import { NumberField } from "./number-field";

const CONFIDENCE_LEVELS = [0.95, 0.975, 0.99] as const;

interface SettingsPanelProps {
    readonly chart: ChartSettings;
    readonly analysis: AnalysisSettings;
    readonly logPrice: boolean;
    readonly varModel: string | null;
    readonly modelNames: readonly string[];
    readonly onChartChange: (settings: ChartSettings) => void;
    readonly onAnalysisChange: (settings: AnalysisSettings) => void;
    readonly onLogPriceChange: (logPrice: boolean) => void;
    readonly onVarModelChange: (model: string) => void;
}

export function SettingsPanel(props: SettingsPanelProps) {
    const { chart, analysis } = props;
    const setChart = (patch: Partial<ChartSettings>) => {
        props.onChartChange({ ...chart, ...patch });
    };
    const setAnalysis = (patch: Partial<AnalysisSettings>) => {
        props.onAnalysisChange({ ...analysis, ...patch });
    };

    return (
        <>
            <h2>chart</h2>
            <NumberField
                label="fast moving average"
                value={chart.fastAverage}
                min={1}
                max={1000}
                step={1}
                integer
                onChange={(fastAverage) => {
                    setChart({ fastAverage });
                }}
            />
            <NumberField
                label="slow moving average"
                value={chart.slowAverage}
                min={1}
                max={1000}
                step={1}
                integer
                onChange={(slowAverage) => {
                    setChart({ slowAverage });
                }}
            />
            <NumberField
                label="realised volatility window (days)"
                value={chart.volatilityWindow}
                min={2}
                max={252}
                step={1}
                integer
                onChange={(volatilityWindow) => {
                    setChart({ volatilityWindow });
                }}
            />
            <label className="check">
                <input
                    type="checkbox"
                    checked={props.logPrice}
                    onChange={(event) => {
                        props.onLogPriceChange(event.target.checked);
                    }}
                />
                log price scale
            </label>

            <h2>forecast evaluation</h2>
            <NumberField
                label="estimation window (returns)"
                value={analysis.window}
                min={100}
                max={5000}
                step={50}
                integer
                onChange={(window) => {
                    setAnalysis({ window });
                }}
            />
            <NumberField
                label="refit every (days)"
                value={analysis.refitEvery}
                min={1}
                max={250}
                step={1}
                integer
                onChange={(refitEvery) => {
                    setAnalysis({ refitEvery });
                }}
            />
            <label className="field">
                <span>estimation window mode</span>
                <select
                    value={analysis.mode}
                    onChange={(event) => {
                        setAnalysis({ mode: event.target.value as WindowMode });
                    }}
                >
                    <option value="sliding">sliding</option>
                    <option value="expanding">expanding</option>
                </select>
            </label>
            <NumberField
                label="baseline realised window"
                value={analysis.baselineWindow}
                min={2}
                max={252}
                step={1}
                integer
                onChange={(baselineWindow) => {
                    setAnalysis({ baselineWindow });
                }}
            />
            <NumberField
                label="EWMA lambda"
                value={analysis.ewmaLambda}
                min={0.5}
                max={0.999}
                step={0.01}
                integer={false}
                onChange={(ewmaLambda) => {
                    setAnalysis({ ewmaLambda });
                }}
            />
            <label className="check">
                <input
                    type="checkbox"
                    checked={analysis.includeGarch}
                    onChange={(event) => {
                        setAnalysis({ includeGarch: event.target.checked });
                    }}
                />
                include GARCH(1,1)
            </label>
            <label className="field">
                <span>VaR confidence</span>
                <select
                    value={analysis.confidence}
                    onChange={(event) => {
                        setAnalysis({ confidence: Number(event.target.value) });
                    }}
                >
                    {CONFIDENCE_LEVELS.map((level) => (
                        <option key={level} value={level}>
                            {`${String(level * 100)}%`}
                        </option>
                    ))}
                </select>
            </label>
            <label className="field">
                <span>VaR band model</span>
                <select
                    value={props.varModel ?? ""}
                    disabled={props.modelNames.length === 0}
                    onChange={(event) => {
                        props.onVarModelChange(event.target.value);
                    }}
                >
                    {props.modelNames.map((name) => (
                        <option key={name} value={name}>
                            {name}
                        </option>
                    ))}
                </select>
            </label>
        </>
    );
}
