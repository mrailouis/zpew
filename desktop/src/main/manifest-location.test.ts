import path from "node:path";

import { describe, expect, it } from "vitest";

import { manifestFromArguments } from "./manifest-location";

describe("manifestFromArguments", () => {
    it("returns the absolute path that follows --manifest", () => {
        expect(manifestFromArguments(["zpew", "--manifest", "data/manifest.json"])).toBe(
            path.resolve("data/manifest.json"),
        );
    });

    it("returns null when the option is missing or has no value", () => {
        expect(manifestFromArguments(["zpew"])).toBeNull();
        expect(manifestFromArguments(["zpew", "--manifest"])).toBeNull();
        expect(manifestFromArguments(["zpew", "--manifest", ""])).toBeNull();
    });
});
