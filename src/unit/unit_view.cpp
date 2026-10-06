#include "unit_view.h"
#include "unit_manager.h"
#include "../window_gui.h"
#include "../window_type.h"
#include "../widgets/vehicle_widget.h"
#include "../table/sprites.h"

struct UnitViewWindow : Window {
    UnitViewWindow(WindowDesc &desc, WindowNumber window_number) : Window(desc) {
        this->CreateNestedTree();
        this->FinishInitNested(window_number);
        this->GetWidget<NWidgetCore>(0); // ensure nested tree allocated
    }

    void Close(int data = 0) override {
        (void)data;
        this->Window::Close();
    }

    std::string GetWidgetString(WidgetID widget, StringID stringid) const override {
        return this->Window::GetWidgetString(widget, stringid);
    }
    
    void OnClick([[maybe_unused]] Point pt, WidgetID widget, [[maybe_unused]] int click_count) override {
        if (widget == WID_UV_MOVE) {
            // Start place mode for picking a target tile for movement. Pass this window as callback.
            SetObjectToPlaceWnd(SPR_CURSOR_MOUSE, PAL_NONE, HighLightStyle(HT_POINT | HT_UNIT), this);
            return;
        }
        this->Window::OnClick(pt, widget, click_count);
    }

    void OnPlaceObject([[maybe_unused]] Point pt, TileIndex tile) override {
        int world_x = TileX(tile) * TILE_SIZE + TILE_SIZE / 2;
        int world_y = TileY(tile) * TILE_SIZE + TILE_SIZE / 2;
        Unit *u = UnitManager::Instance().GetUnitById(this->window_number);
        if (u != nullptr) {
            // If the clicked tile is near another unit, treat it as an attack order.
            bool found_enemy = false;
            for (Unit *other : UnitManager::Instance().GetUnits()) {
                if (other == nullptr || other == u) continue;
                const int dx = std::abs(static_cast<int>(other->GetX()) - world_x);
                const int dy = std::abs(static_cast<int>(other->GetY()) - world_y);
                if (dx <= 12 && dy <= 12) {
                    u->SetAttackTarget(other->get_id());
                    found_enemy = true;
                    break;
                }
            }

            if (!found_enemy) {
                u->ClearAttackTarget();
                u->SetMoveTarget(world_x, world_y);
            }

            UnitManager::Instance().SelectUnit(u->get_id());
        }

        ResetObjectToPlace();
    }
};

static WindowDesc _unit_view_desc(
    WindowPosition::Automatic, "view_unit", 200, 100,
    WindowClass::VehicleView, WindowClass::None,
    {}, {}, nullptr
);

void ShowUnitViewWindow(int unit_id)
{
    AllocateWindowDescFront<UnitViewWindow>(_unit_view_desc, unit_id);
}

