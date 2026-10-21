export const MAIN_VIEWS = ["chart", "models", "data"] as const;

export type MainView = (typeof MAIN_VIEWS)[number];
export type View = MainView | "settings";

export const VIEW_LABELS: Readonly<Record<MainView, string>> = {
    chart: "Chart",
    models: "Models",
    data: "Data",
};
