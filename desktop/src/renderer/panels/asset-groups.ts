import type { AssetSummary } from "../../shared/api";

export interface AssetGroup {
    readonly assetClass: string;
    readonly label: string;
    readonly assets: readonly AssetSummary[];
}

const ASSET_CLASS_LABELS: Readonly<Record<string, string>> = {
    us_equity_broad: "US Equity",
    us_equity_style: "US Equity Styles",
    us_equity_sector: "US Sectors",
    us_equity_industry: "US Industries",
    us_equity_single: "US Stocks",
    intl_developed: "International Developed",
    emerging: "Emerging Markets",
    rates: "Rates",
    credit: "Credit",
    commodity: "Commodities",
    currency: "Currencies",
    crypto: "Crypto",
};

export function assetClassLabel(assetClass: string): string {
    return (
        ASSET_CLASS_LABELS[assetClass] ??
        assetClass
            .split("_")
            .filter((word) => word.length > 0)
            .map((word) => word.charAt(0).toUpperCase() + word.slice(1))
            .join(" ")
    );
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

export function groupAssets(assets: readonly AssetSummary[]): AssetGroup[] {
    const groups: { assetClass: string; label: string; assets: AssetSummary[] }[] = [];
    for (const asset of assets) {
        const group = groups.find((g) => g.assetClass === asset.assetClass);
        if (group === undefined) {
            groups.push({
                assetClass: asset.assetClass,
                label: assetClassLabel(asset.assetClass),
                assets: [asset],
            });
        } else {
            group.assets.push(asset);
        }
    }
    return groups;
}

export function toggleGroup(collapsed: readonly string[], assetClass: string): string[] {
    return collapsed.includes(assetClass)
        ? collapsed.filter((name) => name !== assetClass)
        : [...collapsed, assetClass];
}
