import { describe, expect, it } from "vitest";

import { formatCommit, formatCount } from "./format";

describe("formatCommit", () => {
    it("shortens a full hash to seven characters", () => {
        expect(formatCommit("541db4b0123456789", false)).toBe("541db4b");
    });

    it("marks builds from uncommitted changes", () => {
        expect(formatCommit("541db4b0123456789", true)).toBe("541db4b (uncommitted changes)");
    });

    it("keeps short or unknown commits unchanged", () => {
        expect(formatCommit("unknown", false)).toBe("unknown");
    });
});

describe("formatCount", () => {
    it("uses the singular for one", () => {
        expect(formatCount(1, "asset")).toBe("1 asset");
    });

    it("uses the plural and thousands separators otherwise", () => {
        expect(formatCount(8474, "bar")).toBe("8,474 bars");
        expect(formatCount(0, "asset")).toBe("0 assets");
    });
});
