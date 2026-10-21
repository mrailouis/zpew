import type { AppInfo, ChartSettings } from "../../shared/api";
import {
    CANDLE_PALETTES,
    CROSSHAIR_STYLES,
    DENSITIES,
    THEME_SOURCES,
    type CandlePalette,
    type Preferences,
} from "../appearance/preferences";
import { HueSlider } from "../controls/hue-slider";
import { NumberField } from "../controls/number-field";
import { Segmented } from "../controls/segmented";
import { Switch } from "../controls/switch";
import { formatCommit } from "../format";

const PALETTE_LABELS: Readonly<Record<CandlePalette, string>> = {
    classic: "Green and red",
    colourblind: "Blue and orange",
    mono: "Monochrome",
};

const capitalise = (value: string) => value.charAt(0).toUpperCase() + value.slice(1);

interface SettingsViewProps {
    readonly info: AppInfo | null;
    readonly preferences: Preferences;
    readonly chart: ChartSettings;
    readonly onPreferences: (patch: Partial<Preferences>) => void;
    readonly onChart: (settings: ChartSettings) => void;
}

function Row({ label, children }: { readonly label: string; readonly children: React.ReactNode }) {
    return (
        <div className="row">
            <span>{label}</span>
            {children}
        </div>
    );
}

export function SettingsView({
    info,
    preferences: p,
    chart,
    onPreferences,
    onChart,
}: SettingsViewProps) {
    return (
        <section className="page">
            <div className="page-column">
                <h1>Settings</h1>

                <h2>Appearance</h2>
                <div className="rows">
                    <Row label="Theme">
                        <Segmented
                            label="Theme"
                            options={THEME_SOURCES}
                            value={p.theme}
                            format={capitalise}
                            onChange={(theme) => {
                                onPreferences({ theme });
                            }}
                        />
                    </Row>
                    <Row label="Accent colour">
                        <HueSlider
                            label="Accent hue"
                            hue={p.accent}
                            onChange={(accent) => {
                                onPreferences({ accent });
                            }}
                        />
                    </Row>
                    <Row label="Density">
                        <Segmented
                            label="Density"
                            options={DENSITIES}
                            value={p.density}
                            format={capitalise}
                            onChange={(density) => {
                                onPreferences({ density });
                            }}
                        />
                    </Row>
                </div>

                <h2>Chart</h2>
                <div className="rows">
                    <Row label="Candle colours">
                        <Segmented
                            label="Candle colours"
                            options={CANDLE_PALETTES}
                            value={p.palette}
                            format={(palette) => PALETTE_LABELS[palette]}
                            onChange={(palette) => {
                                onPreferences({ palette });
                            }}
                        />
                    </Row>
                    <Row label="Crosshair">
                        <Segmented
                            label="Crosshair"
                            options={CROSSHAIR_STYLES}
                            value={p.crosshair}
                            format={capitalise}
                            onChange={(crosshair) => {
                                onPreferences({ crosshair });
                            }}
                        />
                    </Row>
                    <Switch
                        label="Grid lines"
                        checked={p.showGrid}
                        onChange={(showGrid) => {
                            onPreferences({ showGrid });
                        }}
                    />
                    <Switch
                        label="Symbol watermark"
                        checked={p.showWatermark}
                        onChange={(showWatermark) => {
                            onPreferences({ showWatermark });
                        }}
                    />
                    <NumberField
                        label="Fast moving average (days)"
                        value={chart.fastAverage}
                        min={2}
                        max={500}
                        step={1}
                        integer
                        onChange={(fastAverage) => {
                            onChart({ ...chart, fastAverage });
                        }}
                    />
                    <NumberField
                        label="Slow moving average (days)"
                        value={chart.slowAverage}
                        min={2}
                        max={1000}
                        step={1}
                        integer
                        onChange={(slowAverage) => {
                            onChart({ ...chart, slowAverage });
                        }}
                    />
                    <NumberField
                        label="Realised volatility window (days)"
                        value={chart.volatilityWindow}
                        min={2}
                        max={252}
                        step={1}
                        integer
                        onChange={(volatilityWindow) => {
                            onChart({ ...chart, volatilityWindow });
                        }}
                    />
                </div>

                <h2>About</h2>
                <div className="rows">
                    <Row label="Version">
                        <span className="muted">{info?.version ?? ""}</span>
                    </Row>
                    <Row label="Build">
                        <span className="muted">
                            {info ? formatCommit(info.commit, info.dirty) : ""}
                        </span>
                    </Row>
                    <Row label="Data">
                        <span className="muted path" title={info?.manifestPath ?? undefined}>
                            {info?.manifestPath
                                ? `\u200e${info.manifestPath}\u200e`
                                : "No manifest found"}
                        </span>
                    </Row>
                </div>
            </div>
        </section>
    );
}
