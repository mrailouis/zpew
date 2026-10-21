import type { ZpewApi } from "../shared/api";

declare global {
    interface Window {
        readonly zpew: ZpewApi;
    }
}
