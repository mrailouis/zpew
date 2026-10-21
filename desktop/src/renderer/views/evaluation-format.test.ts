import { describe, expect, it } from "vitest";

import {
    formatBreaches,
    formatExponent,
    formatImprovement,
    formatPValue,
    splitScientific,
} from "./evaluation-format";

describe("splitScientific", () => {
    it("splits a value into a three-significant-figure mantissa and an exponent", () => {
        expect(splitScientific(2.322e-7)).toEqual({ mantissa: "2.32", exponent: -7 });
        expect(splitScientific(1.5876)).toEqual({ mantissa: "1.59", exponent: 0 });
        expect(splitScientific(-4.5e3)).toEqual({ mantissa: "−4.50", exponent: 3 });
    });

    it("writes values from 0.001 up to 1,000 plainly to three significant figures", () => {
        expect(splitScientific(0.4936)).toEqual({ mantissa: "0.494", exponent: 0 });
        expect(splitScientific(0.001234)).toEqual({ mantissa: "0.00123", exponent: 0 });
        expect(splitScientific(512.3)).toEqual({ mantissa: "512", exponent: 0 });
        expect(splitScientific(-0.25)).toEqual({ mantissa: "−0.250", exponent: 0 });
        expect(splitScientific(0.000999)).toEqual({ mantissa: "9.99", exponent: -4 });
    });

    it("handles zero and non-finite values", () => {
        expect(splitScientific(0)).toEqual({ mantissa: "0", exponent: 0 });
        expect(splitScientific(Number.NaN)).toBeNull();
    });
});

describe("formatExponent", () => {
    it("uses a true minus for negative exponents", () => {
        expect(formatExponent(-7)).toBe("−7");
        expect(formatExponent(3)).toBe("3");
    });
});

describe("formatImprovement", () => {
    it("signs percentages with a plus or a true minus", () => {
        expect(formatImprovement(3.254)).toBe("+3.25%");
        expect(formatImprovement(-1.2)).toBe("−1.20%");
        expect(formatImprovement(0)).toBe("+0.00%");
        expect(formatImprovement(Number.NaN)).toBe("–");
    });
});

describe("formatPValue", () => {
    it("shows three decimals and floors tiny values", () => {
        expect(formatPValue(0.0512)).toBe("0.051");
        expect(formatPValue(0.000105)).toBe("< 0.001");
        expect(formatPValue(0.001)).toBe("0.001");
        expect(formatPValue(Number.NaN)).toBe("–");
    });
});

describe("formatBreaches", () => {
    it("groups thousands", () => {
        expect(formatBreaches(449, 7474)).toBe("449 / 7,474");
    });
});
