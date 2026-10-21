export const THEME = {
    background: "#0b0d12",
    paneBackground: "#0e1117",
    grid: "#1a1f2b",
    border: "#232838",
    axisText: "#7d8497",
    text: "#d7dbe6",
    rising: "#26a69a",
    falling: "#ef5350",
    crosshair: "#5d6475",
    varBand: "rgba(239, 83, 80, 0.75)",
    breach: "#ff4d4d",
    volumeRising: "rgba(38, 166, 154, 0.45)",
    volumeFalling: "rgba(239, 83, 80, 0.45)",
    readoutBackground: "rgba(14, 17, 23, 0.85)",
    font: '11px "SF Mono", Menlo, Consolas, monospace',
} as const;

export const LINE_COLOURS = {
    fastAverage: "#f5b041",
    slowAverage: "#5b8cff",
    realised: "#c77dff",
    forecasts: ["#4dd0e1", "#ffd54f", "#81c784", "#ff8a65"],
} as const;
