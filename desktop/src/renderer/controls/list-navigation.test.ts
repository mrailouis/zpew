import { describe, expect, it } from "vitest";

import { keyToIndex, stepIndex } from "./list-navigation";

describe("stepIndex", () => {
    it("moves through the list and wraps at both ends", () => {
        expect(stepIndex(0, 1, 5)).toBe(1);
        expect(stepIndex(4, 1, 5)).toBe(0);
        expect(stepIndex(0, -1, 5)).toBe(4);
    });

    it("starts from the first or last item when nothing is active", () => {
        expect(stepIndex(-1, 1, 5)).toBe(0);
        expect(stepIndex(-1, -1, 5)).toBe(4);
    });

    it("has no active item in an empty list", () => {
        expect(stepIndex(0, 1, 0)).toBe(-1);
    });
});

describe("keyToIndex", () => {
    it("maps arrow, Home and End keys and ignores others", () => {
        expect(keyToIndex("ArrowDown", 2, 5)).toBe(3);
        expect(keyToIndex("ArrowUp", 0, 5)).toBe(4);
        expect(keyToIndex("Home", 3, 5)).toBe(0);
        expect(keyToIndex("End", 0, 5)).toBe(4);
        expect(keyToIndex("a", 0, 5)).toBeNull();
    });
});
