import path from "node:path";

export function manifestFromArguments(argv: readonly string[]): string | null {
    const index = argv.indexOf("--manifest");
    const value = index >= 0 ? argv[index + 1] : undefined;
    return value !== undefined && value.length > 0 ? path.resolve(value) : null;
}
