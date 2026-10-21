import { describe, expect, it } from "vitest";

import { buildCoverage, cellAt, dayNumber, isSession, missingDays, weekday } from "./coverage";

const seconds = (y: number, m: number, d: number) => Date.UTC(y, m - 1, d) / 1000;
const day = (y: number, m: number, d: number) => dayNumber(seconds(y, m, d));

describe("weekday and isSession", () => {
    it("numbers weekdays from Monday", () => {
        expect(weekday(0)).toBe(3);
        expect(weekday(day(2001, 9, 10))).toBe(0);
        expect(weekday(day(2001, 9, 16))).toBe(6);
    });

    it("treats weekends as sessions only on the daily calendar", () => {
        expect(isSession(day(2001, 9, 15), "weekdays")).toBe(false);
        expect(isSession(day(2001, 9, 15), "daily")).toBe(true);
        expect(isSession(day(2001, 9, 14), "weekdays")).toBe(true);
    });
});

describe("missingDays", () => {
    it("lists the sessions strictly inside each gap", () => {
        const gap = {
            lastBefore: seconds(2001, 9, 10),
            firstAfter: seconds(2001, 9, 17),
            missingSessions: 4,
        };
        expect([...missingDays([gap], "weekdays")]).toEqual([
            day(2001, 9, 11),
            day(2001, 9, 12),
            day(2001, 9, 13),
            day(2001, 9, 14),
        ]);
        expect(missingDays([gap], "daily").size).toBe(6);
    });
});

describe("buildCoverage", () => {
    const times = [
        seconds(2001, 9, 7),
        seconds(2001, 9, 10),
        seconds(2001, 9, 17),
        seconds(2002, 1, 2),
    ];
    const gaps = [
        { lastBefore: seconds(2001, 9, 10), firstAfter: seconds(2001, 9, 17), missingSessions: 4 },
    ];

    it("returns one year per calendar year, newest first", () => {
        expect(buildCoverage(times, gaps, "weekdays").map((y) => y.year)).toEqual([2002, 2001]);
        expect(buildCoverage([], [], "weekdays")).toEqual([]);
    });

    it("classifies bars, missing sessions, closed days and days outside the data", () => {
        const year = buildCoverage(times, gaps, "weekdays")[1];
        const state = (m: number, d: number) =>
            year?.cells.find((cell) => cell.day === day(2001, m, d))?.state;
        expect(state(9, 6)).toBe("outside");
        expect(state(9, 7)).toBe("bar");
        expect(state(9, 12)).toBe("missing");
        expect(state(9, 18)).toBe("closed");
        expect(year?.bars).toBe(3);
        expect(year?.missing).toBe(4);
    });

    it("lays weekday calendars out in five rows and daily ones in seven", () => {
        const weekdays = buildCoverage(times, gaps, "weekdays")[1];
        const daily = buildCoverage(times, gaps, "daily")[1];
        expect(weekdays?.rows).toBe(5);
        expect(weekdays?.cells.every((cell) => cell.row < 5)).toBe(true);
        expect(daily?.rows).toBe(7);
        expect(daily?.cells).toHaveLength(365);
    });

    it("places each day in its Monday-based week column", () => {
        const year = buildCoverage(times, gaps, "daily")[1];
        expect(year?.columns).toBe(53);
        expect(year?.monthColumns[0]).toBe(0);
        const monday = year && cellAt(year, 0, 0);
        expect(monday?.day).toBe(day(2001, 1, 1));
        const lastDay = year?.cells.at(-1);
        expect(lastDay).toMatchObject({ day: day(2001, 12, 31), column: 52, row: 0 });
    });
});
