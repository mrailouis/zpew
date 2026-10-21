import { describe, expect, it } from "vitest";

import {
    AXIS_WIDTH,
    computeLayout,
    headerArea,
    PANE_HEADER_HEIGHT,
    paneAt,
    plotArea,
    TIME_AXIS_HEIGHT,
} from "./layout";

const ALL = { volume: true, volatility: true };

describe("computeLayout", () => {
    it("stacks price, volume and volatility above the time axis without overlap", () => {
        const layout = computeLayout(1000, 800, ALL);
        expect(layout.plotWidth).toBe(1000 - AXIS_WIDTH);
        expect(layout.price.top).toBe(0);
        expect(layout.volume.top).toBeGreaterThanOrEqual(layout.price.top + layout.price.height);
        expect(layout.volatility.top).toBeGreaterThanOrEqual(
            layout.volume.top + layout.volume.height,
        );
        expect(layout.volatility.top + layout.volatility.height).toBe(800 - TIME_AXIS_HEIGHT);
        expect(layout.price.height).toBeGreaterThan(layout.volatility.height);
        expect(layout.volatility.height).toBeGreaterThan(layout.volume.height);
    });

    it("gives hidden panes no height and shares their space with the rest", () => {
        const full = computeLayout(1000, 800, ALL);
        const priceOnly = computeLayout(1000, 800, { volume: false, volatility: false });
        expect(priceOnly.volume.height).toBe(0);
        expect(priceOnly.volatility.height).toBe(0);
        expect(priceOnly.price.height).toBe(800 - TIME_AXIS_HEIGHT);
        const noVolume = computeLayout(1000, 800, { volume: false, volatility: true });
        expect(noVolume.price.height).toBeGreaterThan(full.price.height);
        expect(noVolume.volatility.top + noVolume.volatility.height).toBe(800 - TIME_AXIS_HEIGHT);
    });

    it("finds which visible pane a vertical position falls in", () => {
        const layout = computeLayout(1000, 800, ALL);
        expect(paneAt(layout, PANE_HEADER_HEIGHT + 1)).toBe("price");
        expect(paneAt(layout, 10)).toBeNull();
        expect(paneAt(layout, layout.volume.top + 1)).toBeNull();
        expect(paneAt(layout, layout.volume.top + PANE_HEADER_HEIGHT + 1)).toBe("volume");
        expect(paneAt(layout, layout.volatility.top + PANE_HEADER_HEIGHT + 1)).toBe("volatility");
        expect(paneAt(layout, 799)).toBeNull();
        expect(
            paneAt(computeLayout(1000, 800, { volume: false, volatility: false }), 790),
        ).toBeNull();
    });

    it("never produces negative sizes for a tiny canvas", () => {
        const layout = computeLayout(10, 10, ALL);
        for (const rect of [layout.price, layout.volume, layout.volatility]) {
            expect(rect.width).toBeGreaterThanOrEqual(0);
            expect(rect.height).toBeGreaterThanOrEqual(0);
        }
    });
});

describe("headerArea and plotArea", () => {
    it("splits a pane into a header strip and the plot below it", () => {
        const pane = { left: 0, top: 100, width: 500, height: 300 };
        expect(headerArea(pane)).toEqual({
            left: 0,
            top: 100,
            width: 500,
            height: PANE_HEADER_HEIGHT,
        });
        expect(plotArea(pane)).toEqual({
            left: 0,
            top: 100 + PANE_HEADER_HEIGHT,
            width: 500,
            height: 300 - PANE_HEADER_HEIGHT,
        });
    });

    it("gives a pane shorter than the header no plot area", () => {
        const pane = { left: 0, top: 0, width: 500, height: 10 };
        expect(headerArea(pane).height).toBe(10);
        expect(plotArea(pane).height).toBe(0);
    });
});
