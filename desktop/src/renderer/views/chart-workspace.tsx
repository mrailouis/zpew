import type { AssetSummary } from "../../shared/api";
import type { ChartTheme } from "../appearance/chart-theme";
import { CHART_STYLES, type ChartStyle, type Preferences } from "../appearance/preferences";
import { formatPrice } from "../chart/chart-format";
import type { ChartModel } from "../chart/chart-model";
import { ChartView, type RangeRequest } from "../chart/chart-view";
import { RANGE_PRESETS, type RangePreset } from "../chart/range";
import { Segmented } from "../controls/segmented";
import { Select } from "../controls/select";
import { TextToggle } from "../controls/text-toggle";

const STYLE_LABELS: Readonly<Record<ChartStyle, string>> = {
    candles: "Candles",
    hollow: "Hollow candles",
    bars: "Bars",
    line: "Line",
    area: "Area",
};

interface ChartWorkspaceProps {
    readonly asset: AssetSummary | undefined;
    readonly model: ChartModel | null;
    readonly theme: ChartTheme;
    readonly preferences: Preferences;
    readonly range: RangeRequest;
    readonly error: string | null;
    readonly onRange: (preset: RangePreset) => void;
    readonly onPreferences: (patch: Partial<Preferences>) => void;
}

function Heading({
    asset,
    model,
}: {
    readonly asset: AssetSummary;
    readonly model: ChartModel | null;
}) {
    const count = model?.close.length ?? 0;
    const close = model?.close[count - 1] ?? Number.NaN;
    const change = close / (model?.close[count - 2] ?? Number.NaN) - 1;
    return (
        <div className="heading">
            <div className="heading-main">
                <h1>{asset.symbol}</h1>
                {Number.isFinite(close) && (
                    <div className="heading-quote">
                        <span className={change >= 0 ? "up" : "down"}>
                            {Number.isFinite(change)
                                ? `${change >= 0 ? "+" : ""}${(change * 100).toFixed(2)}%`
                                : ""}
                        </span>
                        <span>{formatPrice(close)}</span>
                    </div>
                )}
            </div>
            <span className="muted heading-name">{asset.name}</span>
        </div>
    );
}

export function ChartWorkspace(props: ChartWorkspaceProps) {
    const { preferences: p, onPreferences } = props;
    return (
        <section className="workspace">
            {props.asset !== undefined && <Heading asset={props.asset} model={props.model} />}
            <div className="toolbar">
                <Segmented
                    variant="text"
                    label="Range"
                    options={RANGE_PRESETS}
                    value={props.range.preset}
                    format={(preset) => (preset === "MAX" ? "All" : preset)}
                    onChange={props.onRange}
                />
                <div className="toolbar-spacer" />
                <TextToggle
                    label="Log"
                    checked={p.logPrice}
                    onChange={(logPrice) => {
                        onPreferences({ logPrice });
                    }}
                />
                <TextToggle
                    label="Volume"
                    checked={p.showVolume}
                    onChange={(showVolume) => {
                        onPreferences({ showVolume });
                    }}
                />
                <TextToggle
                    label="Volatility"
                    checked={p.showVolatility}
                    onChange={(showVolatility) => {
                        onPreferences({ showVolatility });
                    }}
                />
                <Select
                    label="Chart style"
                    options={CHART_STYLES}
                    value={p.chartStyle}
                    format={(style) => STYLE_LABELS[style]}
                    onChange={(chartStyle) => {
                        onPreferences({ chartStyle });
                    }}
                />
            </div>
            {props.error !== null && <p className="note error">{props.error}</p>}
            <div className="chart-area">
                {props.model !== null && (
                    <ChartView
                        model={props.model}
                        theme={props.theme}
                        style={p.chartStyle}
                        logPrice={p.logPrice}
                        panes={{ volume: p.showVolume, volatility: p.showVolatility }}
                        watermark={p.showWatermark}
                        range={props.range}
                    />
                )}
            </div>
        </section>
    );
}
