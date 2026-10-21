import { describe, expect, it } from "vitest";

import { formatCompact, formatIsoDate, formatPercent, formatPrice } from "./chart-format";

describe("chart formatting", () => {
    it("formats prices with precision suited to their size", () => {
        expect(formatPrice(1234.5)).toBe("1,234.50");
        expect(formatPrice(12.3456)).toBe("12.35");
        expect(formatPrice(0.123456)).toBe("0.1235");
        expect(formatPrice(0.00123456)).toBe("0.001235");
        expect(formatPrice(Number.NaN)).toBe("–");
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
