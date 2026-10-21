import { useMemo } from "react";

import type { AssetSummary, ChartData } from "../../shared/api";
import type { ChartTheme } from "../appearance/chart-theme";
import { formatLongDate } from "../chart/chart-format";
import { buildCoverage } from "../coverage/coverage";
import { coverageColours } from "../coverage/coverage-colours";
import { CoverageGrid, CoverageLegend } from "../coverage/coverage-grid";
import { assetClassLabel } from "../panels/asset-groups";
import { sourceNote } from "./source-note";

interface DataViewProps {
    readonly asset: AssetSummary | undefined;
    readonly chart: ChartData | null;
    readonly theme: ChartTheme;
}

const CALENDAR_LABELS: Readonly<Record<string, string>> = {
    weekdays: "Weekdays",
    daily: "Every day",
};

const DAYS_PER_YEAR = 365.25;

function isoToSeconds(iso: string): number {
    return Date.parse(`${iso}T00:00:00Z`) / 1000;
}

function Stat({
    label,
    value,
    detail,
}: {
    readonly label: string;
    readonly value: string;
    readonly detail?: string;
}) {
    return (
        <div className="stat">
            <span className="stat-label">{label}</span>
            <span className="stat-value">{value}</span>
            {detail !== undefined && <span className="stat-detail">{detail}</span>}
        </div>
    );
}

export function DataView({ asset, chart, theme }: DataViewProps) {
    const current =
        chart !== null && asset !== undefined && chart.symbol === asset.symbol ? chart : null;
    const years = useMemo(
        () =>
            current === null ? [] : buildCoverage(current.times, current.gaps, current.calendar),
        [current],
    );
    const colours = useMemo(() => coverageColours(theme), [theme]);
    if (asset === undefined) {
        return <section className="page" />;
    }
    const gaps = current?.gaps ?? [];
    const missing = gaps.reduce((sum, gap) => sum + gap.missingSessions, 0);
    const first = isoToSeconds(asset.firstDate);
    const last = isoToSeconds(asset.lastDate);
    const span = (last - first) / 86400 / DAYS_PER_YEAR;

    return (
        <section className="page">
            <div className="heading">
                <div className="heading-main">
                    <h1>{asset.symbol}</h1>
                </div>
                <span className="muted heading-name">{asset.name}</span>
            </div>

            <div className="table-frame stats">
                <Stat label="Asset class" value={assetClassLabel(asset.assetClass)} />
                <Stat label="Calendar" value={CALENDAR_LABELS[asset.calendar] ?? asset.calendar} />
                <Stat
                    label="Coverage"
                    value={`${span.toFixed(1)} years`}
                    detail={`${formatLongDate(first)} to ${formatLongDate(last)}`}
                />
                <Stat label="Bars" value={asset.rows.toLocaleString("en-GB")} />
                <Stat
                    label="Missing sessions"
                    value={missing.toLocaleString("en-GB")}
                    detail={`in ${gaps.length.toLocaleString("en-GB")} ${gaps.length === 1 ? "gap" : "gaps"}`}
                />
            </div>
            <p className="note">
                {sourceNote(asset.source)} Gaps are runs of missing sessions longer than the trading
                calendar allows.
            </p>

            <div className="section-title">
                <h2>Coverage</h2>
                <CoverageLegend colours={colours} />
            </div>
            <div className="table-frame coverage-frame">
                <CoverageGrid years={years} colours={colours} />
            </div>

            <h2>Gaps</h2>
            {gaps.length === 0 ? (
                <p className="muted">No gaps beyond what the trading calendar allows.</p>
            ) : (
                <div className="table-frame">
                    <table className="evaluation-table gap-table">
                        <thead>
                            <tr>
                                <th>Last bar before</th>
                                <th>First bar after</th>
                                <th>Missing sessions</th>
                            </tr>
                        </thead>
                        <tbody>
                            {gaps.map((gap) => (
                                <tr key={gap.lastBefore}>
                                    <th scope="row">{formatLongDate(gap.lastBefore)}</th>
                                    <td>{formatLongDate(gap.firstAfter)}</td>
                                    <td className="down">
                                        {gap.missingSessions.toLocaleString("en-GB")}
                                    </td>
                                </tr>
                            ))}
                        </tbody>
                    </table>
                </div>
            )}
        </section>
    );
}
