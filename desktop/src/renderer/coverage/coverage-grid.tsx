import { useEffect, useRef, useState } from "react";

import { formatLongDate } from "../chart/chart-format";
import { cellAt, type CoverageCell, type CoverageYear, type DayState } from "./coverage";
import type { CoverageColours } from "./coverage-colours";

const CELL = 11;
const GAP = 3;
const STEP = CELL + GAP;
const MONTHS = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"];
const STATE_LABELS: Readonly<Record<DayState, string>> = {
    bar: "Trading day",
    missing: "Missing session",
    closed: "No session",
    outside: "Outside the data",
};
const WEEKDAYS = ["Mon", "", "Wed", "", "Fri", "", "Sun"];

interface Hover {
    readonly cell: CoverageCell;
    readonly x: number;
    readonly y: number;
}

function YearCanvas({
    year,
    colours,
}: {
    readonly year: CoverageYear;
    readonly colours: CoverageColours;
}) {
    const canvasRef = useRef<HTMLCanvasElement | null>(null);
    const [hover, setHover] = useState<Hover | null>(null);
    const width = year.columns * STEP - GAP;
    const height = year.rows * STEP - GAP;

    useEffect(() => {
        const canvas = canvasRef.current;
        const ctx = canvas?.getContext("2d");
        if (!canvas || !ctx) {
            return;
        }
        const ratio = window.devicePixelRatio || 1;
        canvas.width = Math.round(width * ratio);
        canvas.height = Math.round(height * ratio);
        ctx.setTransform(ratio, 0, 0, ratio, 0, 0);
        ctx.clearRect(0, 0, width, height);
        for (const cell of year.cells) {
            if (cell.state === "outside") {
                continue;
            }
            ctx.fillStyle = colours[cell.state];
            ctx.beginPath();
            ctx.roundRect(cell.column * STEP, cell.row * STEP, CELL, CELL, 2.5);
            ctx.fill();
        }
    }, [year, colours, width, height]);

    const onPointerMove = (event: React.PointerEvent<HTMLCanvasElement>) => {
        const { offsetX, offsetY } = event.nativeEvent;
        const column = Math.floor(offsetX / STEP);
        const row = Math.floor(offsetY / STEP);
        const inside = offsetX % STEP < CELL && offsetY % STEP < CELL;
        const cell = inside ? cellAt(year, column, row) : undefined;
        setHover(
            cell === undefined || cell.state === "outside"
                ? null
                : { cell, x: column * STEP + CELL / 2, y: row * STEP },
        );
    };

    return (
        <div className="coverage-canvas">
            <canvas
                ref={canvasRef}
                style={{ width, height }}
                onPointerMove={onPointerMove}
                onPointerLeave={() => {
                    setHover(null);
                }}
            />
            {hover !== null && (
                <div className="coverage-tooltip" style={{ left: hover.x, top: hover.y }}>
                    <span>{formatLongDate(hover.cell.day * 86400)}</span>
                    <span className={hover.cell.state === "missing" ? "down" : "muted"}>
                        {STATE_LABELS[hover.cell.state]}
                    </span>
                </div>
            )}
        </div>
    );
}

export function CoverageGrid({
    years,
    colours,
}: {
    readonly years: readonly CoverageYear[];
    readonly colours: CoverageColours;
}) {
    const newest = years[0];
    if (newest === undefined) {
        return null;
    }
    return (
        <div className="coverage">
            <div className="coverage-row coverage-months">
                <span />
                <div className="coverage-month-labels">
                    {newest.monthColumns.map((column, month) => (
                        <span key={MONTHS[month]} style={{ left: column * STEP }}>
                            {MONTHS[month]}
                        </span>
                    ))}
                </div>
                <span />
            </div>
            {years.map((year) => (
                <div key={year.year} className="coverage-row">
                    <span className="coverage-year">{year.year}</span>
                    <div className="coverage-body">
                        <div className="coverage-weekdays">
                            {WEEKDAYS.slice(0, year.rows).map((label, row) => (
                                <span key={row}>{label}</span>
                            ))}
                        </div>
                        <YearCanvas year={year} colours={colours} />
                    </div>
                    <span className="coverage-summary">
                        <span>{year.bars.toLocaleString("en-GB")} bars</span>
                        {year.missing > 0 && (
                            <span className="down">
                                {year.missing.toLocaleString("en-GB")} missing
                            </span>
                        )}
                    </span>
                </div>
            ))}
        </div>
    );
}

export function CoverageLegend({ colours }: { readonly colours: CoverageColours }) {
    return (
        <div className="coverage-legend">
            {(["bar", "missing", "closed"] as const).map((state) => (
                <span key={state}>
                    <i style={{ background: colours[state] }} />
                    {STATE_LABELS[state]}
                </span>
            ))}
        </div>
    );
}
