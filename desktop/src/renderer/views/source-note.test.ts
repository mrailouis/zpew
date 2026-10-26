import { describe, expect, it } from "vitest";

import { sourceNote } from "./source-note";

describe("sourceNote", () => {
    it("describes known sources", () => {
        expect(sourceNote("binance_vision")).toContain("Binance Vision");
        expect(sourceNote("simulated")).toContain("not market data");
    });

    it("names any other source as recorded in the manifest", () => {
        expect(sourceNote("hand-made")).toBe("Daily bars from hand-made.");
    });
});
