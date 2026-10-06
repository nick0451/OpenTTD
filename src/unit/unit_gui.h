#pragma once

#include "../viewport_type.h"

// Hook called from viewport click handling. Returns true if click handled.
bool HandleClickOnUnit(const Viewport &vp, int x, int y, int world_x, int world_y);
