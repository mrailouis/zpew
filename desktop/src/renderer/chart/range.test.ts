import { describe, expect, it } from "vitest";

import { rangeStart, viewportForRange } from "./range";

const DAY = 86400;
const end = Date.UTC(2026, 8, 30) / 1000;
const daily = Float64Array.from({ length: 4000 }, (_, i) => end - (3999 - i) * DAY);

describe("rangeStart", () => {
    it("measures fixed spans back from the last bar", () => {
        expect(rangeStart(end, "1M")).toBe(end - 31 * DAY);
        expect(rangeStart(end, "1Y")).toBe(end - 365 * DAY);
    });

    it("starts year-to-date on 1 January of the last bar's year", () => {
        expect(rangeStart(end, "YTD")).toBe(Date.UTC(2026, 0, 1) / 1000);
    });

    it("has no start for the full history", () => {
        expect(rangeStart(end, "MAX")).toBeNull();
    });
});

describe("viewportForRange", () => {
    it("shows the bars inside the range plus a small right margin", () => {
        const viewport = viewportForRange(daily, "3M");
        expect(daily[viewport.first]).toBe(end - 92 * DAY);
        expect(viewport.first + viewport.count).toBe(daily.length + 3);
    });

    it("fits the full history for MAX and for ranges longer than the data", () => {
        expect(viewportForRange(daily, "MAX")).toEqual({ first: 0, count: 4003 });
        const short = daily.slice(-100);
        expect(viewportForRange(short, "5Y")).toEqual({ first: 0, count: 103 });
    });
});
