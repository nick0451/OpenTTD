#include "unit_view.h"
#include "unit_manager.h"
#include "../window_gui.h"
#include "../window_type.h"
#include "../widgets/vehicle_widget.h"
#include "../table/sprites.h"
#include "../table/strings.h"

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
        const Unit *u = UnitManager::Instance().GetUnitById(this->window_number);

        if (widget == WID_UV_CAPTION) {
            return u != nullptr ? "Unit" : "Unit (missing)";
        }

        if (u == nullptr) {
            return this->Window::GetWidgetString(widget, stringid);
        }

        switch (widget) {
            case WID_UV_STATE: {
                const auto state = u->GetTacticalState();
                switch (state) {
                    case Unit::TacticalState::Advance: return "State: Advance";
                    case Unit::TacticalState::Hold: return "State: Hold";
                    case Unit::TacticalState::Retreat: return "State: Retreat";
                    default: return "State: Unknown";
                }
            }
            case WID_UV_HEALTH: return "Health: " + std::to_string(u->GetHealth());
            case WID_UV_SUPPLY: return "Supply: " + std::to_string(u->GetSupply());
            default: return this->Window::GetWidgetString(widget, stringid);
        }
    }
    
    void OnClick([[maybe_unused]] Point pt, WidgetID widget, [[maybe_unused]] int click_count) override {
        if (widget == WID_UV_MOVE) {
            SetObjectToPlaceWnd(SPR_CURSOR_MOUSE, PAL_NONE, HighLightStyle(HT_POINT | HT_UNIT), this);
            return;
        }

        if (widget == WID_UV_STATE) {
            Unit *u = UnitManager::Instance().GetUnitById(this->window_number);
            if (u != nullptr) {
                const auto state = u->GetTacticalState();
                if (state == Unit::TacticalState::Advance) {
                    u->SetTacticalState(Unit::TacticalState::Hold);
                } else if (state == Unit::TacticalState::Hold) {
                    u->SetTacticalState(Unit::TacticalState::Retreat);
                } else {
                    u->SetTacticalState(Unit::TacticalState::Advance);
                }
                this->SetWidgetDirty(WID_UV_STATE);
            }
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

static constexpr std::initializer_list<NWidgetPart> _unit_view_widgets = {
    NWidget(WWT_CAPTION, Colours::Grey, WID_UV_CAPTION),
    NWidget(WWT_LABEL, Colours::Invalid, WID_UV_STATE), SetMinimalSize(180, 12),
    NWidget(WWT_LABEL, Colours::Invalid, WID_UV_HEALTH), SetMinimalSize(180, 12),
    NWidget(WWT_LABEL, Colours::Invalid, WID_UV_SUPPLY), SetMinimalSize(180, 12),
    NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UV_MOVE), SetMinimalSize(72, 12), SetStringTip(STR_BUTTON_MOVE),
};

static WindowDesc _unit_view_desc(
    WindowPosition::Automatic, "view_unit", 220, 110,
    WindowClass::VehicleView, WindowClass::None,
    {},
    _unit_view_widgets
);

void ShowUnitViewWindow(int unit_id)
{
    AllocateWindowDescFront<UnitViewWindow>(_unit_view_desc, unit_id);
}

