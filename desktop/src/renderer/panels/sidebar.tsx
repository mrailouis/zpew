import { useEffect, useMemo, useRef, useState } from "react";

import type { AssetSummary } from "../../shared/api";
import {
    clampSidebarWidth,
    DEFAULT_PREFERENCES,
    type Preferences,
} from "../appearance/preferences";
import { ChevronIcon, SearchIcon, SettingsIcon, SidebarLeftIcon } from "../shell/icons";
import { filterAssets, groupAssets, toggleGroup, type AssetGroup } from "./asset-groups";
import { ResizeHandle } from "./resize-handle";

interface SidebarProps {
    readonly open: boolean;
    readonly assets: readonly AssetSummary[];
    readonly selected: string | null;
    readonly settingsOpen: boolean;
    readonly preferences: Preferences;
    readonly onPreferences: (patch: Partial<Preferences>) => void;
    readonly onSelect: (symbol: string) => void;
    readonly onSettings: () => void;
}

interface GroupProps {
    readonly group: AssetGroup;
    readonly collapsed: boolean;
    readonly selected: string | null;
    readonly selectedRef: React.RefObject<HTMLButtonElement | null>;
    readonly onToggle: () => void;
    readonly onSelect: (symbol: string) => void;
}

function Group({ group, collapsed, selected, selectedRef, onToggle, onSelect }: GroupProps) {
    return (
        <div className={collapsed ? "group collapsed" : "group"}>
            <button
                type="button"
                className="group-label"
                aria-expanded={!collapsed}
                onClick={onToggle}
            >
                <span>{group.label}</span>
                <ChevronIcon size={14} />
            </button>
            <div className="group-body">
                <div className="group-inner" inert={collapsed}>
                    {group.assets.map((asset) => (
                        <button
                            key={asset.symbol}
                            ref={asset.symbol === selected ? selectedRef : undefined}
                            type="button"
                            title={asset.name}
                            className={asset.symbol === selected ? "list-row selected" : "list-row"}
                            onClick={() => {
                                onSelect(asset.symbol);
                            }}
                        >
                            <span className="list-symbol">{asset.symbol}</span>
                            <span className="list-name">{asset.name}</span>
                        </button>
                    ))}
                </div>
            </div>
        </div>
    );
}

export function Sidebar(props: SidebarProps) {
    const { preferences, onPreferences } = props;
    const [filter, setFilter] = useState("");
    const [dragWidth, setDragWidth] = useState<number | null>(null);
    const groups = useMemo(
        () => groupAssets(filterAssets(props.assets, filter)),
        [props.assets, filter],
    );
    const selectedRef = useRef<HTMLButtonElement | null>(null);
    const searching = filter.trim() !== "";
    const width = dragWidth ?? preferences.sidebarWidth;

    useEffect(() => {
        selectedRef.current?.scrollIntoView({ block: "nearest" });
    }, [props.selected]);

    const frameClass = [
        "sidebar-frame",
        props.open ? "" : "closed",
        dragWidth === null ? "" : "resizing",
    ]
        .filter((name) => name !== "")
        .join(" ");

    return (
        <div className={frameClass} style={{ width: props.open ? width : 0 }} inert={!props.open}>
            <aside className="sidebar" style={{ width }}>
                <div className="sidebar-top">
                    <button
                        type="button"
                        className="icon-button"
                        title="Hide sidebar"
                        onClick={() => {
                            onPreferences({ showWatchlist: false });
                        }}
                    >
                        <SidebarLeftIcon />
                    </button>
                </div>
                <div className="sidebar-title">zpew</div>
                <label className="search">
                    <SearchIcon size={16} />
                    <input
                        placeholder="Search"
                        value={filter}
                        onChange={(event) => {
                            setFilter(event.target.value);
                        }}
                    />
                </label>
                <div className="sidebar-scroll">
                    {groups.map((group) => (
                        <Group
                            key={group.assetClass}
                            group={group}
                            collapsed={
                                !searching && preferences.collapsedGroups.includes(group.assetClass)
                            }
                            selected={props.settingsOpen ? null : props.selected}
                            selectedRef={selectedRef}
                            onToggle={() => {
                                onPreferences({
                                    collapsedGroups: toggleGroup(
                                        preferences.collapsedGroups,
                                        group.assetClass,
                                    ),
                                });
                            }}
                            onSelect={props.onSelect}
                        />
                    ))}
                    {groups.length === 0 && <p className="empty">No matches</p>}
                </div>
                <button
                    type="button"
                    className={
                        props.settingsOpen
                            ? "list-row sidebar-footer selected"
                            : "list-row sidebar-footer"
                    }
                    onClick={props.onSettings}
                >
                    <SettingsIcon size={17} />
                    <span>Settings</span>
                </button>
            </aside>
            <ResizeHandle
                width={preferences.sidebarWidth}
                onResize={(next) => {
                    setDragWidth(clampSidebarWidth(next));
                }}
                onResizeEnd={(next) => {
                    setDragWidth(null);
                    onPreferences({ sidebarWidth: clampSidebarWidth(next) });
                }}
                onReset={() => {
                    onPreferences({ sidebarWidth: DEFAULT_PREFERENCES.sidebarWidth });
                }}
            />
        </div>
    );
}
