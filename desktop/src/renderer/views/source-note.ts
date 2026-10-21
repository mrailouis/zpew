const NOTES: Readonly<Record<string, string>> = {
    tiingo: "Daily bars from Tiingo, adjusted for splits and dividends.",
    simulated: "Simulated daily bars from python/simulate_data.py. These are not market data.",
};

export function sourceNote(source: string): string {
    return NOTES[source] ?? `Daily bars from ${source}.`;
}
