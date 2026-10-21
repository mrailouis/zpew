import { withAlpha, type ChartTheme } from "../appearance/chart-theme";
import type { DayState } from "./coverage";

export type CoverageColours = Readonly<Record<Exclude<DayState, "outside">, string>>;

export function coverageColours(theme: ChartTheme): CoverageColours {
    return {
        bar: withAlpha(theme.accent, 0.6),
        missing: theme.breach,
        closed: theme.separator,
    };
}
