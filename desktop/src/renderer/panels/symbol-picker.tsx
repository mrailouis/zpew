import { useMemo, useState } from "react";

import type { AssetSummary } from "../../shared/api";

interface SymbolPickerProps {
    readonly assets: readonly AssetSummary[];
    readonly selected: string | null;
    readonly onSelect: (symbol: string) => void;
}

export function filterAssets(assets: readonly AssetSummary[], filter: string): AssetSummary[] {
    const needle = filter.trim().toLowerCase();
    if (needle === "") {
        return [...assets];
    }
    return assets.filter((asset) =>
        [asset.symbol, asset.name, asset.assetClass].some((field) =>
            field.toLowerCase().includes(needle),
        ),
    );
}

export function SymbolPicker({ assets, selected, onSelect }: SymbolPickerProps) {
    const [filter, setFilter] = useState("");
    const visible = useMemo(() => filterAssets(assets, filter), [assets, filter]);

    return (
        <>
            <h2>symbols</h2>
            <input
                className="filter"
                placeholder="filter symbols"
                value={filter}
                onChange={(event) => {
                    setFilter(event.target.value);
                }}
            />
            <div className="symbols">
                {visible.map((asset, i) => (
                    <div key={asset.symbol}>
                        {asset.assetClass !== visible[i - 1]?.assetClass && (
                            <div className="asset-class">{asset.assetClass}</div>
                        )}
                        <button
                            type="button"
                            className={asset.symbol === selected ? "asset selected" : "asset"}
                            onClick={() => {
                                onSelect(asset.symbol);
                            }}
                        >
                            <span>{asset.symbol}</span>
                            <span className="muted">{asset.name}</span>
                        </button>
                    </div>
                ))}
            </div>
        </>
    );
}
