#pragma once

#include "../window_gui.h"
#include "../widgets/vehicle_widget.h"
#include "../tilehighlight_func.h"

void ShowUnitViewWindow(int unit_id);

// Widget IDs for the lightweight unit readout.
enum UnitViewWidgets : WidgetID {
	WID_UV_CAPTION = WID_VV_CAPTION,
	WID_UV_VIEWPORT = WID_VV_VIEWPORT,
	WID_UV_STATE = WID_VV_CAPTION + 1,
	WID_UV_HEALTH = WID_VV_CAPTION + 2,
	WID_UV_SUPPLY = WID_VV_CAPTION + 3,
	WID_UV_MOVE = WID_VV_GOTO_DEPOT,
};
