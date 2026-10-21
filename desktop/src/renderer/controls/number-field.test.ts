import { describe, expect, it } from "vitest";

import { parseFieldValue } from "./number-field";

describe("parseFieldValue", () => {
    it("accepts values inside the range unchanged", () => {
        expect(parseFieldValue("21", 2, 252, true)).toBe(21);
        expect(parseFieldValue("0.94", 0.5, 0.999, false)).toBe(0.94);
    });

    it("clamps values outside the range", () => {
        expect(parseFieldValue("1", 2, 252, true)).toBe(2);
        expect(parseFieldValue("1.2", 0.5, 0.999, false)).toBe(0.999);
    });

    it("rejects empty, non-numeric and fractional integer input", () => {
        expect(parseFieldValue("", 2, 252, true)).toBeNull();
        expect(parseFieldValue("abc", 2, 252, true)).toBeNull();
        expect(parseFieldValue("2.5", 2, 252, true)).toBeNull();
    });
});
