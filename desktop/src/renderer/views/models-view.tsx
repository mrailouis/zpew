import type { AnalysisData, Improvement, LossSummary, ModelAnalysis } from "../../shared/api";
import { modelLabel } from "../format";
import {
    formatBreaches,
    formatExponent,
    formatImprovement,
    formatPValue,
    splitScientific,
} from "./evaluation-format";

type Metric = "mse" | "qlike";

const PERCENT_METRIC: Readonly<Record<Metric, "msePercent" | "qlikePercent">> = {
    mse: "msePercent",
    qlike: "qlikePercent",
};

function Loss({ value }: { readonly value: number }) {
    const parts = splitScientific(value);
    if (parts === null) {
        return <span className="cell-sub">–</span>;
    }
    return (
        <span className="cell-sub">
            {parts.exponent === 0 ? (
                parts.mantissa
            ) : (
                <>
                    {parts.mantissa} × 10<sup>{formatExponent(parts.exponent)}</sup>
                </>
            )}
        </span>
    );
}

function AccuracyCell(props: {
    readonly improvement: Improvement | null;
    readonly losses: LossSummary;
    readonly metric: Metric;
}) {
    const { improvement, losses, metric } = props;
    const percent = improvement === null ? null : improvement[PERCENT_METRIC[metric]];
    return (
        <td>
            {percent === null ? (
                <span className="cell-main muted">Baseline</span>
            ) : (
                <span className={percent >= 0 ? "cell-main up" : "cell-main down"}>
                    {formatImprovement(percent)}
                </span>
            )}
            <Loss value={losses[metric]} />
        </td>
    );
}

function ModelCell({ model }: { readonly model: ModelAnalysis }) {
    return (
        <th scope="row">
            <span className="cell-main">{modelLabel(model.name)}</span>
        </th>
    );
}

function FailedRow({ model }: { readonly model: ModelAnalysis }) {
    return (
        <tr>
            <ModelCell model={model} />
            <td colSpan={5} className="failure">
                {model.failure ?? "No result"}
            </td>
        </tr>
    );
}

function Columns() {
    return (
        <colgroup>
            <col className="model-column" />
            <col />
            <col />
            <col />
            <col />
            <col />
        </colgroup>
    );
}

function AccuracyTable({ analysis }: { readonly analysis: AnalysisData }) {
    return (
        <div className="table-frame">
            <table className="evaluation-table">
                <Columns />
                <thead>
                    <tr className="group-row">
                        <th />
                        <th colSpan={2}>Squared returns</th>
                        <th colSpan={2}>Parkinson range</th>
                        <th />
                    </tr>
                    <tr>
                        <th>Model</th>
                        <th>MSE</th>
                        <th>QLIKE</th>
                        <th>MSE</th>
                        <th>QLIKE</th>
                        <th>Refits</th>
                    </tr>
                </thead>
                <tbody>
                    {analysis.models.map((model) =>
                        model.losses === null ? (
                            <FailedRow key={model.name} model={model} />
                        ) : (
                            <tr key={model.name}>
                                <ModelCell model={model} />
                                <AccuracyCell
                                    improvement={model.improvements.squaredReturn}
                                    losses={model.losses.squaredReturn}
                                    metric="mse"
                                />
                                <AccuracyCell
                                    improvement={model.improvements.squaredReturn}
                                    losses={model.losses.squaredReturn}
                                    metric="qlike"
                                />
                                <AccuracyCell
                                    improvement={model.improvements.parkinson}
                                    losses={model.losses.parkinson}
                                    metric="mse"
                                />
                                <AccuracyCell
                                    improvement={model.improvements.parkinson}
                                    losses={model.losses.parkinson}
                                    metric="qlike"
                                />
                                <td>
                                    <span className="cell-main">
                                        {model.refits.toLocaleString("en-GB")}
                                    </span>
                                    <span className="cell-sub">
                                        {model.boundaryRefits > 0
                                            ? `${String(model.boundaryRefits)} at boundary`
                                            : " "}
                                    </span>
                                </td>
                            </tr>
                        ),
                    )}
                </tbody>
            </table>
        </div>
    );
}

function BacktestTable({ analysis }: { readonly analysis: AnalysisData }) {
    return (
        <div className="table-frame">
            <table className="evaluation-table">
                <Columns />
                <thead>
                    <tr>
                        <th>Model</th>
                        <th>Breaches</th>
                        <th>Breach rate</th>
                        <th>Expected</th>
                        <th>Kupiec p</th>
                        <th>Result</th>
                    </tr>
                </thead>
                <tbody>
                    {analysis.models.map((model) => {
                        const b = model.backtest;
                        if (model.failure !== null || b === null) {
                            return <FailedRow key={model.name} model={model} />;
                        }
                        const rejects = b.kupiecPValue < 0.05;
                        return (
                            <tr key={model.name}>
                                <ModelCell model={model} />
                                <td>{formatBreaches(b.breaches, b.observations)}</td>
                                <td>{(b.breachRate * 100).toFixed(2)}%</td>
                                <td className="muted">{(b.expectedRate * 100).toFixed(1)}%</td>
                                <td>{formatPValue(b.kupiecPValue)}</td>
                                <td className={rejects ? "down" : "up"}>
                                    {rejects ? "Rejected" : "Not rejected"}
                                </td>
                            </tr>
                        );
                    })}
                </tbody>
            </table>
        </div>
    );
}

export function ModelsView({
    analysis,
    status,
}: {
    readonly analysis: AnalysisData | null;
    readonly status: string | null;
}) {
    if (analysis === null) {
        return (
            <section className="page">
                <p className="muted">{status ?? "No evaluation yet"}</p>
            </section>
        );
    }
    const percent = String(Math.round(analysis.confidence * 1000) / 10);
    return (
        <section className="page">
            <h1>{analysis.symbol} forecast evaluation</h1>
            <p className="muted">
                {`${analysis.times.length.toLocaleString("en-GB")} out-of-sample days with a ${analysis.mode} window of ${analysis.window.toLocaleString("en-GB")} returns, refitted every ${String(analysis.refitEvery)} days. Computed in ${analysis.seconds.toFixed(1)} s.`}
            </p>

            <h2>Forecast accuracy</h2>
            <p className="note">
                Reduction in loss against {modelLabel(analysis.baseline)}, the baseline. Positive is
                better; the loss itself is shown beneath. QLIKE excludes days with a zero proxy.
            </p>
            <AccuracyTable analysis={analysis} />

            <h2>{percent}% VaR backtest</h2>
            <p className="note">A Kupiec test at the 5% level against the expected breach rate.</p>
            <BacktestTable analysis={analysis} />
        </section>
    );
}
