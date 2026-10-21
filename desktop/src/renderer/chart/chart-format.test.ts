import { describe, expect, it } from "vitest";

import {
    formatCompact,
    formatIsoDate,
    formatPercent,
    formatPrice,
    formatChange,
    formatLongDate,
} from "./chart-format";

describe("chart formatting", () => {
    it("formats prices with precision suited to their size", () => {
        expect(formatPrice(1234.5)).toBe("1,234.50");
        expect(formatPrice(12.3456)).toBe("12.35");
        expect(formatPrice(0.123456)).toBe("0.1235");
        expect(formatPrice(0.00123456)).toBe("0.001235");
        expect(formatPrice(Number.NaN)).toBe("–");
        expect(formatPrice(0)).toBe("0.00");
    });

    it("abbreviates large volumes", () => {
        expect(formatCompact(1_500_000)).toBe("1.50M");
        expect(formatCompact(2_340)).toBe("2.34K");
        expect(formatCompact(512)).toBe("512");
    });

    it("formats percentages and UTC dates", () => {
        expect(formatPercent(12.3456)).toBe("12.35%");
        expect(formatPercent(Number.NaN)).toBe("–");
        expect(formatIsoDate(1704412800)).toBe("2024-01-05");
    });
});

describe("formatLongDate", () => {
    it("writes the UTC day, short month and year", () => {
        expect(formatLongDate(Date.UTC(2026, 3, 22) / 1000)).toBe("22 Apr 2026");
        expect(formatLongDate(Date.UTC(1993, 0, 29) / 1000)).toBe("29 Jan 1993");
    });
});

describe("formatChange", () => {
    it("signs a fractional change as a percentage with a true minus", () => {
        expect(formatChange(0.0042)).toBe("+0.42%");
        expect(formatChange(-0.0123)).toBe("−1.23%");
        expect(formatChange(0)).toBe("+0.00%");
        expect(formatChange(Number.NaN)).toBe("–");
    });
});
