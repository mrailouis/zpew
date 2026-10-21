import path from "node:path";

import { beforeAll, describe, expect, it } from "vitest";

import { DEFAULT_CHART_SETTINGS, type AnalysisSettings } from "../shared/api";
import { addonPath, loadNativeModule, type NativeModule } from "./native-module";

const desktopRoot = path.resolve(__dirname, "..", "..");
const fixtureManifest = path.resolve(
    desktopRoot,
    "..",
    "tests",
    "cli",
    "fixtures",
    "eval",
    "manifest.json",
);
const FIXTURE_BARS = 160;

const fixtureAnalysis: AnalysisSettings = {
    window: 60,
    refitEvery: 20,
    mode: "sliding",
    baselineWindow: 21,
    ewmaLambda: 0.94,
    includeGarch: true,
    confidence: 0.95,
};

let native: NativeModule;

beforeAll(() => {
    native = loadNativeModule(addonPath(desktopRoot));
});

describe("native module", () => {
    it("reports the core version and build", () => {
        expect(native.version()).toMatch(/^\d+\.\d+\.\d+$/);
        expect(native.buildInfo().commit.length).toBeGreaterThan(0);
    });

    it("finds a manifest by walking up from a nested directory", () => {
        expect(native.findManifest(path.join(desktopRoot, "src", "main"))).toMatch(
            /data[/\\]manifest\.json$/,
        );
    });

    it("lists the assets in a manifest", async () => {
        const assets = await native.listAssets(fixtureManifest);
        expect(assets).toEqual([
            expect.objectContaining({
                symbol: "SIM",
                calendar: "weekdays",
                source: "hand-made",
                rows: FIXTURE_BARS,
            }),
        ]);
    });

    it("loads chart series as aligned typed arrays", async () => {
        const chart = await native.loadChart(fixtureManifest, "SIM", {
            fastAverage: 5,
            slowAverage: 20,
            volatilityWindow: 21,
        });
        expect(chart.times).toBeInstanceOf(Float64Array);
        for (const series of [
            chart.open,
            chart.close,
            chart.fastAverage,
            chart.volatilityPercent,
        ]) {
            expect(series.length).toBe(FIXTURE_BARS);
        }
        expect(Number.isNaN(chart.fastAverage[3])).toBe(true);
        expect(chart.fastAverage[4]).toBeCloseTo(
            ((chart.close[0] ?? 0) +
                (chart.close[1] ?? 0) +
                (chart.close[2] ?? 0) +
                (chart.close[3] ?? 0) +
                (chart.close[4] ?? 0)) /
                5,
            12,
        );
        expect(Number.isNaN(chart.volatilityPercent[20])).toBe(true);
        expect(chart.volatilityPercent[21]).toBeGreaterThan(0);
    });

    it("evaluates models and reports models that cannot fit the window", async () => {
        const analysis = await native.analyse(fixtureManifest, "SIM", fixtureAnalysis);
        expect(analysis.baseline).toBe("rv21");
        expect(analysis.times.length).toBe(FIXTURE_BARS - 1 - fixtureAnalysis.window);
        const [baseline, ewma, garch] = analysis.models;
        expect(baseline?.improvements.squaredReturn).toBeNull();
        expect(ewma?.failure).toBeNull();
        expect(ewma?.volatilityPercent.length).toBe(analysis.times.length);
        expect(ewma?.varPrice.length).toBe(analysis.times.length);
        expect(ewma?.backtest?.observations).toBe(analysis.times.length);
        expect(ewma?.breachTimes.length).toBe(ewma?.backtest?.breaches);
        expect(garch?.failure).toBe("needs a window of at least 500 returns");
    });

    it("rejects unknown symbols and invalid settings", async () => {
        await expect(
            native.loadChart(fixtureManifest, "NOPE", DEFAULT_CHART_SETTINGS),
        ).rejects.toThrow("unknown symbol 'NOPE'");
        expect(() =>
            native.analyse(fixtureManifest, "SIM", {
                ...fixtureAnalysis,
                mode: "rolling" as "sliding",
            }),
        ).toThrow("setting 'mode' must be 'sliding' or 'expanding'");
        expect(() =>
            native.loadChart(fixtureManifest, "SIM", {
                ...DEFAULT_CHART_SETTINGS,
                fastAverage: 2.5,
            }),
        ).toThrow("setting 'fastAverage' must be a whole number");
        expect(() =>
            native.analyse(fixtureManifest, "SIM", { ...fixtureAnalysis, ewmaLambda: 1.5 }),
        ).toThrow("setting 'ewmaLambda' must be between 0 and 1 (exclusive)");
        expect(() =>
            native.analyse(fixtureManifest, "SIM", { ...fixtureAnalysis, confidence: 0.4 }),
        ).toThrow("setting 'confidence' must be between 0.5 and 1 (exclusive)");
    });
});
