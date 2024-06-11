#pragma once

enum PANEL_TYPE {
    PANEL_FILES = 0,
    PANEL_MOVE = 1,
    PANEL_TEMP = 2,
    PANEL_SETTINGS = 3,
    PANEL_MACROS = 4,
    PANEL_STATS = 5,
    PANEL_PRINTER = 6,
    PANEL_ERROR = 7,
    PANEL_CONNECTING = 8,
    PANEL_PROGRESS = 9,
};

void nav_buttons_setup(PANEL_TYPE active_panel);
void nav_style_setup();