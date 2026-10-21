import { describe, expect, it } from "vitest";

import { AXIS_WIDTH, computeLayout, paneAt, TIME_AXIS_HEIGHT } from "./layout";

describe("computeLayout", () => {
    it("stacks price, volume and volatility above the time axis without overlap", () => {
        const layout = computeLayout(1000, 800);
        expect(layout.plotWidth).toBe(1000 - AXIS_WIDTH);
        expect(layout.price.top).toBe(0);
        expect(layout.volume.top).toBeGreaterThanOrEqual(layout.price.top + layout.price.height);
        expect(layout.volatility.top).toBeGreaterThanOrEqual(
            layout.volume.top + layout.volume.height,
        );
        expect(layout.timeAxis.top + TIME_AXIS_HEIGHT).toBe(800);
        expect(layout.price.height).toBeGreaterThan(layout.volatility.height);
        expect(layout.volatility.height).toBeGreaterThan(layout.volume.height);
    });

    it("finds which pane a vertical position falls in", () => {
        const layout = computeLayout(1000, 800);
        expect(paneAt(layout, 10)).toBe("price");
        expect(paneAt(layout, layout.volume.top + 1)).toBe("volume");
        expect(paneAt(layout, layout.volatility.top + 1)).toBe("volatility");
        expect(paneAt(layout, 799)).toBeNull();
    });

    it("never produces negative sizes for a tiny canvas", () => {
        const layout = computeLayout(10, 10);
        for (const rect of [layout.price, layout.volume, layout.volatility]) {
            expect(rect.width).toBeGreaterThanOrEqual(0);
            expect(rect.height).toBeGreaterThanOrEqual(0);
        }
    });
});
