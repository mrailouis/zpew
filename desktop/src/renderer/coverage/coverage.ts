import type { ChartGap } from "../../shared/api";

export type DayState = "bar" | "missing" | "closed" | "outside";

export interface CoverageCell {
    readonly day: number;
    readonly state: DayState;
    readonly column: number;
    readonly row: number;
}

export interface CoverageYear {
    readonly year: number;
    readonly rows: number;
    readonly columns: number;
    readonly cells: readonly CoverageCell[];
    readonly bars: number;
    readonly missing: number;
    readonly monthColumns: readonly number[];
}

const SECONDS_PER_DAY = 86400;
const THURSDAY = 3;

export function dayNumber(secondsSinceEpoch: number): number {
    return Math.floor(secondsSinceEpoch / SECONDS_PER_DAY);
}

export function weekday(day: number): number {
    return (((day + THURSDAY) % 7) + 7) % 7;
}

export function isSession(day: number, calendar: string): boolean {
    return calendar === "daily" || weekday(day) < 5;
}

function utcDay(year: number, month: number, date: number): number {
    return Date.UTC(year, month, date) / 1000 / SECONDS_PER_DAY;
}

export function missingDays(gaps: readonly ChartGap[], calendar: string): Set<number> {
    const missing = new Set<number>();
    for (const gap of gaps) {
        for (let day = dayNumber(gap.lastBefore) + 1; day < dayNumber(gap.firstAfter); day += 1) {
            if (isSession(day, calendar)) {
                missing.add(day);
            }
        }
    }
    return missing;
}

function stateOf(
    day: number,
    first: number,
    last: number,
    bars: ReadonlySet<number>,
    missing: ReadonlySet<number>,
): DayState {
    if (day < first || day > last) {
        return "outside";
    }
    if (bars.has(day)) {
        return "bar";
    }
    return missing.has(day) ? "missing" : "closed";
}

function buildYear(
    year: number,
    calendar: string,
    first: number,
    last: number,
    bars: ReadonlySet<number>,
    missing: ReadonlySet<number>,
): CoverageYear {
    const start = utcDay(year, 0, 1);
    const end = utcDay(year + 1, 0, 1);
    const offset = weekday(start);
    const rows = calendar === "daily" ? 7 : 5;
    const cells: CoverageCell[] = [];
    let barCount = 0;
    let missingCount = 0;
    for (let day = start; day < end; day += 1) {
        const row = weekday(day);
        if (row >= rows) {
            continue;
        }
        const state = stateOf(day, first, last, bars, missing);
        barCount += state === "bar" ? 1 : 0;
        missingCount += state === "missing" ? 1 : 0;
        cells.push({ day, state, column: Math.floor((day - start + offset) / 7), row });
    }
    const monthColumns = Array.from({ length: 12 }, (_, month) =>
        Math.floor((utcDay(year, month, 1) - start + offset) / 7),
    );
    const columns = Math.floor((end - 1 - start + offset) / 7) + 1;
    return { year, rows, columns, cells, bars: barCount, missing: missingCount, monthColumns };
}

export function buildCoverage(
    times: ArrayLike<number>,
    gaps: readonly ChartGap[],
    calendar: string,
): CoverageYear[] {
    if (times.length === 0) {
        return [];
    }
    const bars = new Set<number>();
    for (let i = 0; i < times.length; i += 1) {
        bars.add(dayNumber(times[i] ?? 0));
    }
    const first = dayNumber(times[0] ?? 0);
    const last = dayNumber(times[times.length - 1] ?? 0);
    const missing = missingDays(gaps, calendar);
    const firstYear = new Date(first * SECONDS_PER_DAY * 1000).getUTCFullYear();
    const lastYear = new Date(last * SECONDS_PER_DAY * 1000).getUTCFullYear();
    const years: CoverageYear[] = [];
    for (let year = lastYear; year >= firstYear; year -= 1) {
        years.push(buildYear(year, calendar, first, last, bars, missing));
    }
    return years;
}

export function cellAt(year: CoverageYear, column: number, row: number): CoverageCell | undefined {
    return year.cells.find((cell) => cell.column === column && cell.row === row);
}
