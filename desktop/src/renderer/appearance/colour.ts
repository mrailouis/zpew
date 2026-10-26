const ACCENT_SATURATION = 0.82;
const ACCENT_LIGHTNESS = 0.55;
const LIGHT_BACKGROUND_LUMINANCE = 0.5;
const LIGHT_TEXT = "#ffffff";
const DARK_TEXT = "#0d0d0d";

export function normaliseHue(hue: number): number {
    return ((Math.round(hue) % 360) + 360) % 360;
}

function channel(value: number): string {
    return Math.round(Math.min(1, Math.max(0, value)) * 255)
        .toString(16)
        .padStart(2, "0");
}

export function hslToHex(hue: number, saturation: number, lightness: number): string {
    const chroma = (1 - Math.abs(2 * lightness - 1)) * saturation;
    const sector = normaliseHue(hue) / 60;
    const second = chroma * (1 - Math.abs((sector % 2) - 1));
    const [r, g, b] =
        sector < 1
            ? [chroma, second, 0]
            : sector < 2
              ? [second, chroma, 0]
              : sector < 3
                ? [0, chroma, second]
                : sector < 4
                  ? [0, second, chroma]
                  : sector < 5
                    ? [second, 0, chroma]
                    : [chroma, 0, second];
    const match = lightness - chroma / 2;
    return `#${channel(r + match)}${channel(g + match)}${channel(b + match)}`;
}

export function accentColour(hue: number): string {
    return hslToHex(hue, ACCENT_SATURATION, ACCENT_LIGHTNESS);
}

function linearChannel(hex: string): number {
    const value = Number.parseInt(hex, 16) / 255;
    return value <= 0.04045 ? value / 12.92 : ((value + 0.055) / 1.055) ** 2.4;
}

export function relativeLuminance(hex: string): number {
    const value = hex.replace("#", "");
    return (
        0.2126 * linearChannel(value.slice(0, 2)) +
        0.7152 * linearChannel(value.slice(2, 4)) +
        0.0722 * linearChannel(value.slice(4, 6))
    );
}

export function readableTextOn(background: string): string {
    return relativeLuminance(background) > LIGHT_BACKGROUND_LUMINANCE ? DARK_TEXT : LIGHT_TEXT;
}
