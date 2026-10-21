import { describe, expect, it } from "vitest";

import type { AssetSummary } from "../../shared/api";
import { assetClassLabel, filterAssets, groupAssets, toggleGroup } from "./asset-groups";

const asset = (symbol: string, name: string, assetClass: string): AssetSummary => ({
    symbol,
    name,
    assetClass,
    calendar: "weekdays",
    source: "tiingo",
    firstDate: "2000-01-03",
    lastDate: "2026-09-30",
    rows: 100,
});

const assets = [
    asset("SPY", "SPDR S&P 500 ETF Trust", "us_equity_broad"),
    asset("TLT", "iShares 20+ Year Treasury Bond ETF", "rates"),
    asset("BTCUSD", "Bitcoin / US Dollar", "crypto"),
];

describe("filterAssets", () => {
    it("returns every asset for an empty filter", () => {
        expect(filterAssets(assets, "  ")).toHaveLength(3);
    });

    it("matches symbol, name or asset class without regard to case", () => {
        expect(filterAssets(assets, "spy").map((a) => a.symbol)).toEqual(["SPY"]);
        expect(filterAssets(assets, "treasury").map((a) => a.symbol)).toEqual(["TLT"]);
        expect(filterAssets(assets, "CRYPTO").map((a) => a.symbol)).toEqual(["BTCUSD"]);
    });
});

describe("assetClassLabel", () => {
    it("title-cases known asset classes and unknown ones", () => {
        expect(assetClassLabel("rates")).toBe("Rates");
        expect(assetClassLabel("us_equity_broad")).toBe("US Equity");
        expect(assetClassLabel("us_equity_style")).toBe("US Equity Styles");
        expect(assetClassLabel("frontier_markets")).toBe("Frontier Markets");
        expect(assetClassLabel("odd__name_")).toBe("Odd Name");
    });
});

describe("groupAssets", () => {
    it("groups assets by class in order of first appearance, keeping their order", () => {
        const mixed = [
            asset("SPY", "S&P 500", "us_equity_broad"),
            asset("TLT", "Treasuries", "rates"),
            asset("QQQ", "Nasdaq 100", "us_equity_broad"),
        ];
        expect(groupAssets(mixed).map((g) => [g.label, g.assets.map((a) => a.symbol)])).toEqual([
            ["US Equity", ["SPY", "QQQ"]],
            ["Rates", ["TLT"]],
        ]);
    });

    it("returns no groups for no assets", () => {
        expect(groupAssets([])).toEqual([]);
    });
});

describe("toggleGroup", () => {
    it("collapses an open group and reopens a collapsed one", () => {
        expect(toggleGroup([], "rates")).toEqual(["rates"]);
        expect(toggleGroup(["rates", "crypto"], "rates")).toEqual(["crypto"]);
    });
});
