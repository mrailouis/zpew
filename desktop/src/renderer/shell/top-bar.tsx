import { Segmented } from "../controls/segmented";
import { SidebarLeftIcon, SidebarRightIcon } from "./icons";
import { MAIN_VIEWS, VIEW_LABELS, type MainView, type View } from "./views";

interface TopBarProps {
    readonly view: View;
    readonly showSidebar: boolean;
    readonly showInspector: boolean;
    readonly onView: (view: MainView) => void;
    readonly onToggleSidebar: () => void;
    readonly onToggleInspector: () => void;
}

export function TopBar(props: TopBarProps) {
    return (
        <header className="topbar">
            <div className="topbar-side">
                {!props.showSidebar && (
                    <button
                        type="button"
                        className="icon-button"
                        title="Show sidebar"
                        onClick={props.onToggleSidebar}
                    >
                        <SidebarLeftIcon />
                    </button>
                )}
            </div>
            <Segmented
                label="View"
                options={MAIN_VIEWS}
                value={props.view === "settings" ? null : props.view}
                format={(view) => VIEW_LABELS[view]}
                onChange={props.onView}
            />
            <div className="topbar-side end">
                {!props.showInspector && (
                    <button
                        type="button"
                        className="icon-button"
                        title="Show forecasts panel"
                        onClick={props.onToggleInspector}
                    >
                        <SidebarRightIcon />
                    </button>
                )}
            </div>
        </header>
    );
}
