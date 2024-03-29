#pragma once

#define PANEL_PRINT 0
#define PANEL_MOVE 1
#define PANEL_TEMP 2
#define PANEL_SETTINGS 3
#define PANEL_MACROS 4
#define PANEL_STATS 5
#define PANEL_PRINTER 6
#define PANEL_ERROR 7
#define PANEL_CONNECTING 8

void nav_buttons_setup(unsigned char active_panel);
void nav_style_setup();