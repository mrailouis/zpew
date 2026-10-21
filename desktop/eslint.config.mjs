import js from "@eslint/js";
import { defineConfig } from "eslint/config";
import reactHooks from "eslint-plugin-react-hooks";
import tseslint from "typescript-eslint";

export default defineConfig(
    { ignores: ["dist/**", "build/**", "release/**", "node_modules/**"] },
    js.configs.recommended,
    tseslint.configs.strictTypeChecked,
    {
        languageOptions: {
            parserOptions: {
                projectService: {
                    allowDefaultProject: ["*.mjs", "*.mts"],
                },
                tsconfigRootDir: import.meta.dirname,
            },
        },
    },
    {
        files: ["src/renderer/**/*.{ts,tsx}"],
        plugins: { "react-hooks": reactHooks },
        rules: reactHooks.configs.recommended.rules,
    },
    {
        rules: {
            "@typescript-eslint/no-explicit-any": "error",
            "@typescript-eslint/no-non-null-assertion": "error",
            "@typescript-eslint/ban-ts-comment": "error",
        },
    },
);
