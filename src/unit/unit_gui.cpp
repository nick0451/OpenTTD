#include "unit_gui.h"
#include "unit.h"
#include "unit_manager.h"
#include "../viewport_type.h"
#include <cmath>
#include "../vehicle_gui.h"
#include "../tilehighlight_func.h"
#include "unit_view.h"

// Very small prototype: find a unit within a small radius and, if found,
// set the place mode callback to simulate selection. Returns true if handled.
bool HandleClickOnUnit(const Viewport &vp, int x, int y, int world_x, int world_y)
{
    (void)vp; (void)x; (void)y;
    // Brute-force search over units (small prototype)
    const int radius = 8; // pixels
    for (Unit *u : UnitManager::Instance().GetUnits()) {
        if (u == nullptr) continue;
        float ux = u->GetX();
        float uy = u->GetY();
        int dx = static_cast<int>(std::abs(ux - world_x));
        int dy = static_cast<int>(std::abs(uy - world_y));
        if (dx <= radius && dy <= radius) {
            // If the player is in an object-placement style mode for units, treat the
            // click as a unit-selection callback instead of opening the generic inspector.
            if (_thd.place_mode & (HT_VEHICLE | HT_UNIT)) {
                if (_thd.GetCallbackWnd() != nullptr && _thd.GetCallbackWnd()->OnVehicleSelect(nullptr)) return true;
            }

            // Keep the selected unit state explicit for a lightweight RTS prototype.
            UnitManager::Instance().SelectUnit(u->get_id());

            // Open the unit view window for normal selection.
            ShowUnitViewWindow(u->get_id());
            return true;
        }
    }
    return false;
}
