import { describe, expect, it } from "vitest";

import { chooseLevel, timeTicks } from "./time-ticks";

function weekdayTimes(fromIso: string, count: number): Float64Array {
    const times: number[] = [];
    const date = new Date(`${fromIso}T00:00:00Z`);
    while (times.length < count) {
        const day = date.getUTCDay();
        if (day !== 0 && day !== 6) {
            times.push(date.getTime() / 1000);
        }
        date.setUTCDate(date.getUTCDate() + 1);
    }
    return Float64Array.from(times);
}

describe("chooseLevel", () => {
    it("picks the finest calendar unit whose labels stay far enough apart", () => {
        const weekdaysPerDay = 5 / 7;
        expect(chooseLevel(weekdaysPerDay, 100, 60)).toBe(0);
        expect(chooseLevel(weekdaysPerDay, 40, 60)).toBe(1);
        expect(chooseLevel(weekdaysPerDay, 4, 60)).toBe(2);
        expect(chooseLevel(weekdaysPerDay, 0.5, 60)).toBe(4);
        expect(chooseLevel(weekdaysPerDay, 0.01, 60)).toBe(8);
    });
});

describe("timeTicks", () => {
    it("labels the first bar of each month, using the year for January", () => {
        const times = weekdayTimes("2020-11-02", 120);
        const ticks = timeTicks(times, 0, times.length, 4, 60);
        expect(ticks.map((t) => t.label)).toEqual(["Dec", "2021", "Feb", "Mar", "Apr"]);
        for (const tick of ticks) {
            const date = new Date((times[tick.index] ?? 0) * 1000);
            const previous = new Date((times[tick.index - 1] ?? 0) * 1000);
            expect(date.getUTCMonth()).not.toBe(previous.getUTCMonth());
        }
    });

    it("switches to years when zoomed far out", () => {
        const times = weekdayTimes("2010-01-01", 2600);
        const ticks = timeTicks(times, 0, times.length, 0.3, 60);
        expect(ticks.every((t) => /^\d{4}$/.test(t.label))).toBe(true);
        expect(ticks.length).toBeGreaterThan(3);
    });

    it("keeps labels at least the minimum spacing apart", () => {
        const times = weekdayTimes("2015-01-01", 2000);
        const pixelsPerBar = 0.7;
        const ticks = timeTicks(times, 0, times.length, pixelsPerBar, 80);
        for (let i = 1; i < ticks.length; i += 1) {
            const gap = ((ticks[i]?.index ?? 0) - (ticks[i - 1]?.index ?? 0)) * pixelsPerBar;
            expect(gap).toBeGreaterThanOrEqual(80);
        }
    });

    it("returns nothing for fewer than two visible bars", () => {
        expect(timeTicks(weekdayTimes("2020-01-01", 1), 0, 1, 10, 60)).toEqual([]);
    });
});
