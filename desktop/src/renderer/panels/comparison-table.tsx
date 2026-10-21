import type { AnalysisData, Improvement, ModelAnalysis } from "../../shared/api";

function percent(improvement: Improvement | null, metric: "msePercent" | "qlikePercent"): string {
    if (improvement === null) {
        return "baseline";
    }
    const value = improvement[metric];
    return `${value >= 0 ? "+" : ""}${value.toFixed(2)}%`;
}

function ModelRow({ model }: { readonly model: ModelAnalysis }) {
    if (model.failure !== null) {
        return (
            <tr>
                <td>{model.name}</td>
                <td colSpan={11} className="muted">
                    {model.failure}
                </td>
            </tr>
        );
    }
    const backtest = model.backtest;
    return (
        <tr>
            <td>{model.name}</td>
            <td>
                {model.refits} ({model.boundaryRefits} at boundary)
            </td>
            <td>{model.losses?.squaredReturn.mse.toPrecision(4) ?? ""}</td>
            <td>{model.losses?.squaredReturn.qlike.toFixed(4) ?? ""}</td>
            <td>{percent(model.improvements.squaredReturn, "msePercent")}</td>
            <td>{percent(model.improvements.squaredReturn, "qlikePercent")}</td>
            <td>{percent(model.improvements.parkinson, "msePercent")}</td>
            <td>{percent(model.improvements.parkinson, "qlikePercent")}</td>
            <td>
                {backtest ? `${String(backtest.breaches)} / ${String(backtest.observations)}` : ""}
            </td>
            <td>{backtest ? `${(backtest.breachRate * 100).toFixed(2)}%` : ""}</td>
            <td>{backtest ? `${(backtest.expectedRate * 100).toFixed(2)}%` : ""}</td>
            <td>{backtest ? backtest.kupiecPValue.toPrecision(3) : ""}</td>
        </tr>
    );
}

export function ComparisonTable({ analysis }: { readonly analysis: AnalysisData }) {
    return (
        <div className="comparison">
            <p className="muted">
                {analysis.symbol}: window {analysis.window} returns, refit every{" "}
                {analysis.refitEvery} days, {analysis.mode} window, baseline {analysis.baseline},
                computed in {analysis.seconds.toFixed(1)} s
            </p>
            <table>
                <thead>
                    <tr>
                        <th>model</th>
                        <th>refits</th>
                        <th>MSE (r²)</th>
                        <th>QLIKE (r²)</th>
                        <th>MSE vs base (r²)</th>
                        <th>QLIKE vs base (r²)</th>
                        <th>MSE vs base (Parkinson)</th>
                        <th>QLIKE vs base (Parkinson)</th>
                        <th>VaR breaches</th>
                        <th>breach rate</th>
                        <th>expected</th>
                        <th>Kupiec p</th>
                    </tr>
                </thead>
                <tbody>
                    {analysis.models.map((model) => (
                        <ModelRow key={model.name} model={model} />
                    ))}
                </tbody>
            </table>
            <p className="muted">
                Improvements are 100 × (1 − model loss / baseline loss); positive means better than
                the baseline. QLIKE excludes days with a zero proxy. A Kupiec p below 0.05 rejects
                the expected breach rate.
            </p>
        </div>
    );
}
