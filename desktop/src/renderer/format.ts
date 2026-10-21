export function formatCommit(commit: string, dirty: boolean): string {
    const short = commit.length > 7 ? commit.slice(0, 7) : commit;
    return dirty ? `${short} (uncommitted changes)` : short;
}

export function formatCount(value: number, noun: string): string {
    return `${value.toLocaleString("en-GB")} ${value === 1 ? noun : `${noun}s`}`;
}

export function modelLabel(name: string): string {
    return name.toUpperCase();
}
