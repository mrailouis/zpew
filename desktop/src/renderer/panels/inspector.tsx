import type { AnalysisData, AnalysisSettings, WindowMode } from "../../shared/api";
import { formatPercent } from "../chart/chart-format";
import { SlidingIndicator } from "../controls/sliding-indicator";
import { useIndicator } from "../controls/use-indicator";
import { modelLabel } from "../format";
import { NumberField } from "../controls/number-field";
import { Segmented } from "../controls/segmented";
import { Switch } from "../controls/switch";
import { SidebarRightIcon } from "../shell/icons";

const CONFIDENCE_LEVELS = ["0.95", "0.975", "0.99"] as const;
const MODES: readonly WindowMode[] = ["sliding", "expanding"];

interface InspectorProps {
    readonly open: boolean;
    readonly onHide: () => void;
    readonly settings: AnalysisSettings;
    readonly analysis: AnalysisData | null;
    readonly status: string | null;
    readonly error: boolean;
    readonly varModel: string | null;
    readonly onChange: (settings: AnalysisSettings) => void;
    readonly onVarModelChange: (model: string) => void;
}

function lastFinite(values: Float64Array): number {
    for (let i = values.length - 1; i >= 0; i -= 1) {
        const value = values[i];
        if (value !== undefined && Number.isFinite(value)) {
            return value;
        }
    }
    return Number.NaN;
}

const capitalise = (value: string) => value.charAt(0).toUpperCase() + value.slice(1);

export function Inspector(props: InspectorProps) {
    const { settings, analysis, varModel } = props;
    const models = analysis?.models.filter((model) => model.failure === null) ?? [];
    const failures = analysis?.models.filter((model) => model.failure !== null) ?? [];
    const selectedIndex = models.findIndex((model) => model.name === varModel);
    const { containerRef, indicator, animated } = useIndicator(
        selectedIndex,
        "vertical",
        models.length,
    );
    const set = (patch: Partial<AnalysisSettings>) => {
        props.onChange({ ...settings, ...patch });
    };

    return (
        <div
            className={props.open ? "inspector-frame" : "inspector-frame closed"}
            inert={!props.open}
        >
            <aside className="inspector">
                <div className="inspector-top">
                    <button
                        type="button"
                        className="icon-button"
                        title="Hide forecasts panel"
                        onClick={props.onHide}
                    >
                        <SidebarRightIcon />
                    </button>
                </div>
                <section className="section">
                    <div className="section-heading">
                        <h3>Forecasts</h3>
                        {props.status !== null && (
                            <span
                                className={props.error ? "heading-status error" : "heading-status"}
                                title={props.status}
                            >
                                {props.status}
                            </span>
                        )}
                    </div>
                    <p className="note">
                        Annualised volatility forecast for the next day. Choose a model to draw its
                        VaR band.
                    </p>
                    <div
                        ref={containerRef}
                        className={animated ? "forecast-list animated" : "forecast-list"}
                    >
                        <SlidingIndicator indicator={indicator} axis="vertical" />
                        {models.map((model) => (
                            <button
                                key={model.name}
                                type="button"
                                className={
                                    model.name === varModel ? "forecast selected" : "forecast"
                                }
                                title="Draw this model's VaR band"
                                onClick={() => {
                                    props.onVarModelChange(model.name);
                                }}
                            >
                                <span>{modelLabel(model.name)}</span>
                                <span className="number">
                                    {formatPercent(lastFinite(model.volatilityPercent), 1)}
                                </span>
                            </button>
                        ))}
                    </div>
                    {failures.map((model) => (
                        <p key={model.name} className="note error">
                            {modelLabel(model.name)} failed: {model.failure}
                        </p>
                    ))}
                </section>

                <section className="section">
                    <h3>Evaluation</h3>
                    <NumberField
                        label="Estimation window"
                        value={settings.window}
                        min={100}
                        max={5000}
                        step={50}
                        integer
                        onChange={(window) => {
                            set({ window });
                        }}
                    />
                    <NumberField
                        label="Refit every (days)"
                        value={settings.refitEvery}
                        min={1}
                        max={250}
                        step={1}
                        integer
                        onChange={(refitEvery) => {
                            set({ refitEvery });
                        }}
                    />
                    <div className="field">
                        <span>Window</span>
                        <Segmented
                            label="Estimation window mode"
                            options={MODES}
                            value={settings.mode}
                            format={capitalise}
                            onChange={(mode) => {
                                set({ mode });
                            }}
                        />
                    </div>
                    <NumberField
                        label="Baseline window"
                        value={settings.baselineWindow}
                        min={2}
                        max={252}
                        step={1}
                        integer
                        onChange={(baselineWindow) => {
                            set({ baselineWindow });
                        }}
                    />
                    <NumberField
                        label="EWMA λ"
                        value={settings.ewmaLambda}
                        min={0.5}
                        max={0.999}
                        step={0.01}
                        integer={false}
                        onChange={(ewmaLambda) => {
                            set({ ewmaLambda });
                        }}
                    />
                    <div className="field">
                        <span>VaR level</span>
                        <Segmented
                            label="VaR confidence"
                            options={CONFIDENCE_LEVELS}
                            value={
                                String(settings.confidence) as (typeof CONFIDENCE_LEVELS)[number]
                            }
                            format={(level) => `${String(Number(level) * 100)}%`}
                            onChange={(level) => {
                                set({ confidence: Number(level) });
                            }}
                        />
                    </div>
                    <Switch
                        label="GARCH(1,1)"
                        checked={settings.includeGarch}
                        onChange={(includeGarch) => {
                            set({ includeGarch });
                        }}
                    />
                </section>
            </aside>
        </div>
    );
}
