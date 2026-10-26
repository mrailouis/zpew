const NOTES: Readonly<Record<string, string>> = {
    binance_vision: "Daily USDT spot klines from Binance Vision, licensed under CC BY-NC-SA 4.0.",
    simulated: "Simulated daily bars from python/simulate_data.py. These are not market data.",
};

export function sourceNote(source: string): string {
    return NOTES[source] ?? `Daily bars from ${source}.`;
}
