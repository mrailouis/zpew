export interface Scientific {
    readonly mantissa: string;
    readonly exponent: number;
}

const MINUS = "−";
const PLAIN_MIN_EXPONENT = -3;
const PLAIN_MAX_EXPONENT = 2;

export function signed(value: string): string {
    return value.startsWith("-") ? `${MINUS}${value.slice(1)}` : value;
}

export function splitScientific(value: number, digits = 3): Scientific | null {
    if (!Number.isFinite(value)) {
        return null;
    }
    if (value === 0) {
        return { mantissa: "0", exponent: 0 };
    }
    const [mantissa = "", exponent = "0"] = value.toExponential(digits - 1).split("e");
    const power = Number(exponent);
    if (power >= PLAIN_MIN_EXPONENT && power <= PLAIN_MAX_EXPONENT) {
        return { mantissa: signed(value.toPrecision(digits)), exponent: 0 };
    }
    return { mantissa: signed(mantissa), exponent: power };
}

export function formatImprovement(percent: number): string {
    if (!Number.isFinite(percent)) {
        return "–";
    }
    return percent >= 0 ? `+${percent.toFixed(2)}%` : `${MINUS}${Math.abs(percent).toFixed(2)}%`;
}

export function formatPValue(p: number): string {
    if (!Number.isFinite(p)) {
        return "–";
    }
    return p < 0.001 ? "< 0.001" : p.toFixed(3);
}

export function formatBreaches(breaches: number, observations: number): string {
    return `${breaches.toLocaleString("en-GB")} / ${observations.toLocaleString("en-GB")}`;
}

export function formatExponent(exponent: number): string {
    return signed(String(exponent));
}
