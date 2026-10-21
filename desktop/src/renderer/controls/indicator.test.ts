import { describe, expect, it } from "vitest";

import { indicatorFor, measureSegment } from "./indicator";

const segments = [
    { offset: 3, size: 80 },
    { offset: 83, size: 92 },
];

describe("indicatorFor", () => {
    it("sits under the selected segment", () => {
        expect(indicatorFor(segments, 1, null)).toEqual({ offset: 83, size: 92, visible: true });
    });

    it("hides in place when nothing is selected so it fades rather than jumps", () => {
        const shown = indicatorFor(segments, 0, null);
        expect(indicatorFor(segments, -1, shown)).toEqual({ offset: 3, size: 80, visible: false });
        expect(indicatorFor([], 0, null)).toEqual({ offset: 0, size: 0, visible: false });
    });
});

describe("measureSegment", () => {
    const element = {
        offsetLeft: 10,
        offsetWidth: 40,
        offsetTop: 50,
        offsetHeight: 34,
    } as HTMLElement;

    it("measures along the chosen axis", () => {
        expect(measureSegment(element, "horizontal")).toEqual({ offset: 10, size: 40 });
        expect(measureSegment(element, "vertical")).toEqual({ offset: 50, size: 34 });
    });
});
