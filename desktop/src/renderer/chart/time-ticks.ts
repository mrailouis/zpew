export interface TimeTick {
    readonly index: number;
    readonly label: string;
}

const MONTHS = [
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec",
] as const;
const SECONDS_PER_DAY = 86400;

interface Level {
    readonly days: number;
    readonly key: (date: Date) => number;
    readonly label: (date: Date) => string;
}

const year = (d: Date) => d.getUTCFullYear();
const month = (d: Date) => d.getUTCMonth();
const monthName = (d: Date) => MONTHS[month(d)] ?? "";

const LEVELS: readonly Level[] = [
    {
        days: 1,
        key: (d) => Math.floor(d.getTime() / 86_400_000),
        label: (d) => `${String(d.getUTCDate())} ${monthName(d)}`,
    },
    {
        days: 7,
        key: (d) => Math.floor((d.getTime() / 86_400_000 + 3) / 7),
        label: (d) => `${String(d.getUTCDate())} ${monthName(d)}`,
    },
    {
        days: 30,
        key: (d) => year(d) * 12 + month(d),
        label: (d) => (month(d) === 0 ? String(year(d)) : monthName(d)),
    },
    {
        days: 91,
        key: (d) => year(d) * 4 + Math.floor(month(d) / 3),
        label: (d) => (month(d) < 3 ? String(year(d)) : monthName(d)),
    },
    {
        days: 182,
        key: (d) => year(d) * 2 + Math.floor(month(d) / 6),
        label: (d) => (month(d) < 6 ? String(year(d)) : monthName(d)),
    },
    { days: 365, key: year, label: (d) => String(year(d)) },
    { days: 730, key: (d) => Math.floor(year(d) / 2), label: (d) => String(year(d)) },
    { days: 1826, key: (d) => Math.floor(year(d) / 5), label: (d) => String(year(d)) },
    { days: 3652, key: (d) => Math.floor(year(d) / 10), label: (d) => String(year(d)) },
];

export function chooseLevel(barsPerDay: number, pixelsPerBar: number, minSpacing: number): number {
    const index = LEVELS.findIndex((level) => level.days * barsPerDay * pixelsPerBar >= minSpacing);
    return index === -1 ? LEVELS.length - 1 : index;
}

export function timeTicks(
    times: ArrayLike<number>,
    start: number,
    end: number,
    pixelsPerBar: number,
    minSpacing: number,
): TimeTick[] {
    if (end - start < 2) {
        return [];
    }
    const first = times[start] ?? 0;
    const last = times[end - 1] ?? first;
    const spanDays = Math.max(1, (last - first) / SECONDS_PER_DAY);
    const level = LEVELS[chooseLevel((end - start - 1) / spanDays, pixelsPerBar, minSpacing)];
    if (level === undefined) {
        return [];
    }
    const ticks: TimeTick[] = [];
    let previous = level.key(new Date((times[start - 1] ?? first - SECONDS_PER_DAY) * 1000));
    let lastPlaced = Number.NEGATIVE_INFINITY;
    for (let index = start; index < end; index += 1) {
        const date = new Date((times[index] ?? 0) * 1000);
        const key = level.key(date);
        if (key !== previous && (index - lastPlaced) * pixelsPerBar >= minSpacing) {
            ticks.push({ index, label: level.label(date) });
            lastPlaced = index;
        }
        previous = key;
    }
    return ticks;
}
