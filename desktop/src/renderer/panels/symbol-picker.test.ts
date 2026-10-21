import { describe, expect, it } from "vitest";

import type { AssetSummary } from "../../shared/api";
import { filterAssets } from "./symbol-picker";

const asset = (symbol: string, name: string, assetClass: string): AssetSummary => ({
    symbol,
    name,
    assetClass,
    calendar: "weekdays",
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
