import { useRef } from "react";

interface ResizeHandleProps {
    readonly width: number;
    readonly onResize: (width: number) => void;
    readonly onResizeEnd: (width: number) => void;
    readonly onReset: () => void;
}

export function ResizeHandle({ width, onResize, onResizeEnd, onReset }: ResizeHandleProps) {
    const dragRef = useRef<{ x: number; width: number } | null>(null);

    return (
        <div
            className="resize-handle"
            role="separator"
            aria-orientation="vertical"
            aria-label="Resize sidebar"
            onPointerDown={(event) => {
                event.currentTarget.setPointerCapture(event.pointerId);
                dragRef.current = { x: event.clientX, width };
            }}
            onPointerMove={(event) => {
                const drag = dragRef.current;
                if (drag !== null) {
                    onResize(drag.width + event.clientX - drag.x);
                }
            }}
            onPointerUp={(event) => {
                const drag = dragRef.current;
                event.currentTarget.releasePointerCapture(event.pointerId);
                dragRef.current = null;
                if (drag !== null) {
                    onResizeEnd(drag.width + event.clientX - drag.x);
                }
            }}
            onDoubleClick={onReset}
        />
    );
}
