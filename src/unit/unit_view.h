#pragma once

#include "../window_gui.h"
#include "../widgets/vehicle_widget.h"
#include "../tilehighlight_func.h"

void ShowUnitViewWindow(int unit_id);

// Widget IDs for UnitView reuse (small subset)
enum UnitViewWidgets : WidgetID {
	WID_UV_CAPTION = WID_VV_CAPTION,
	WID_UV_VIEWPORT = WID_VV_VIEWPORT,
	WID_UV_MOVE = WID_VV_GOTO_DEPOT, // repurpose existing id for Move button
};
