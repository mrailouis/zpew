import { useEffect, useState } from "react";

import type { AppInfo, AssetSummary } from "../shared/api";
import { formatCommit, formatCount } from "./format";

type LoadState =
    | { readonly kind: "loading" }
    | { readonly kind: "ready"; readonly info: AppInfo; readonly assets: readonly AssetSummary[] }
    | { readonly kind: "failed"; readonly message: string };

function errorMessage(error: unknown): string {
    return error instanceof Error ? error.message : String(error);
}

export function App() {
    const [state, setState] = useState<LoadState>({ kind: "loading" });

    useEffect(() => {
        let active = true;
        Promise.all([window.zpew.appInfo(), window.zpew.listAssets()])
            .then(([info, assets]) => {
                if (active) {
                    setState({ kind: "ready", info, assets });
                }
            })
            .catch((error: unknown) => {
                if (active) {
                    setState({ kind: "failed", message: errorMessage(error) });
                }
            });
        return () => {
            active = false;
        };
    }, []);

    return (
        <div className="app">
            <aside className="sidebar">
                <div>zpew</div>
                {state.kind === "ready" && (
                    <>
                        <div className="muted">
                            {state.info.version} ·{" "}
                            {formatCommit(state.info.commit, state.info.dirty)}
                        </div>
                        <div className="muted">{formatCount(state.assets.length, "asset")}</div>
                        {state.assets.map((asset) => (
                            <div className="asset" key={asset.symbol}>
                                <span>{asset.symbol}</span>
                                <span className="muted">{asset.name}</span>
                            </div>
                        ))}
                    </>
                )}
            </aside>
            <main className="main">
                {state.kind === "loading" && <div className="muted">loading…</div>}
                {state.kind === "failed" && <div className="error">{state.message}</div>}
            </main>
        </div>
    );
}
