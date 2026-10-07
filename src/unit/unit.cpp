#include "unit.h"
#include "unit_manager.h"
#include <algorithm>
#include <limits>
#include <queue>
#include <utility>
#include <vector>

Unit::Unit(Type type, int id)
: id_(id), type_(type), x_(0), y_(0), heading_(0), health_(100) {}

Unit::~Unit() {}

int Unit::get_id() const { return id_; }

Unit::Type Unit::get_type() const { return type_; }

float Unit::GetMoveSpeed() const {
    float base_speed = 1.0f;
    switch (type_) {
        case Type::Infantry: base_speed = 0.8f; break;
        case Type::Armor:    base_speed = 1.5f; break;
        case Type::Artillery:base_speed = 0.7f; break;
        case Type::Air:      base_speed = 2.2f; break;
        case Type::Naval:    base_speed = 1.1f; break;
        default:            base_speed = 1.0f; break;
    }

    const float pressure_penalty = std::min(0.55f, GetFrontlinePressure() * 0.08f);
    return base_speed * (1.0f - pressure_penalty);
}

float Unit::GetAttackRange() const {
    switch (type_) {
        case Type::Infantry: return 10.0f;
        case Type::Armor:    return 14.0f;
        case Type::Artillery:return 18.0f;
        case Type::Air:      return 20.0f;
        case Type::Naval:    return 16.0f;
        default:            return 12.0f;
    }
}

int Unit::GetAttackDamage() const {
    switch (type_) {
        case Type::Infantry: return 8;
        case Type::Armor:    return 14;
        case Type::Artillery:return 18;
        case Type::Air:      return 12;
        case Type::Naval:    return 10;
        default:            return 10;
    }
}

int Unit::GetRoleAttackWeight() const {
    switch (type_) {
        case Type::Infantry: return 1;
        case Type::Armor:    return 3;
        case Type::Artillery:return 2;
        case Type::Air:      return 2;
        case Type::Naval:    return 1;
        default:            return 1;
    }
}

int Unit::GetRoleDefenseWeight() const {
    switch (type_) {
        case Type::Infantry: return 3;
        case Type::Armor:    return 2;
        case Type::Artillery:return 1;
        case Type::Air:      return 1;
        case Type::Naval:    return 2;
        default:            return 1;
    }
}

float Unit::GetTerrainPenaltyAt(int wx, int wy) const {
    const int tile_x = wx / 32;
    const int tile_y = wy / 32;
    const int pseudo = (tile_x * 13 + tile_y * 17) % 10;

    float terrain_cost = 1.0f;
    if (pseudo >= 7) terrain_cost = 2.4f;
    else if (pseudo >= 4) terrain_cost = 1.6f;

    if (base_x_ >= 0 && base_y_ >= 0) {
        const int base_tile_x = base_x_ / 32;
        const int base_tile_y = base_y_ / 32;
        const int approach = std::abs(tile_x - base_tile_x) + std::abs(tile_y - base_tile_y);
        if (approach <= 4) terrain_cost *= 0.9f;
    }

    return terrain_cost;
}

float Unit::GetSupplyPenalty() const {
    const float supply_pressure = (100.0f - std::max(0, supply_)) / 100.0f;
    const float logistics_penalty = std::max(0.0f, GetSupplyLinePenalty() - 1.0f);
    const float frontline_penalty = std::min(1.0f, GetFrontlinePressure() * 0.12f);

    if (supply_ >= 75) return 1.0f + logistics_penalty + frontline_penalty;
    if (supply_ >= 40) return 1.2f + logistics_penalty + frontline_penalty;
    if (supply_ >= 20) return 1.5f + supply_pressure + logistics_penalty + frontline_penalty;
    return 1.9f + supply_pressure + logistics_penalty + frontline_penalty;
}

float Unit::GetSupplyLinePenalty() const {
    if (base_x_ < 0 || base_y_ < 0) return 1.0f;

    const int distance = std::abs(static_cast<int>(x_) - base_x_) + std::abs(static_cast<int>(y_) - base_y_);
    if (distance <= 32) return 1.0f;
    if (distance <= 96) return 1.1f + (distance / 300.0f);
    if (distance <= 180) return 1.2f + (distance / 200.0f);
    return 1.5f + (distance / 150.0f);
}

bool Unit::IsUnderSupplyPressure() const {
    return supply_ < 50 || GetSupplyLinePenalty() > 1.25f || GetFrontlinePressure() > 3;
}

int Unit::GetFrontlinePressure() const {
    int pressure = 0;
    for (const Unit *other : UnitManager::Instance().GetUnits()) {
        if (other == nullptr || other == this || other->GetOwner() == owner_) continue;

        const float dx = other->GetX() - x_;
        const float dy = other->GetY() - y_;
        const float dist = std::sqrt(dx * dx + dy * dy);
        if (dist <= 180.0f) {
            pressure += static_cast<int>((180.0f - dist) / 28.0f);
        }
    }
    return pressure;
}

Unit *Unit::FindNearestEnemy() const {
    Unit *nearest = nullptr;
    float best_distance = std::numeric_limits<float>::max();

    for (Unit *other : UnitManager::Instance().GetUnits()) {
        if (other == nullptr || other == this || other->GetOwner() == owner_) continue;

        const float dx = other->GetX() - x_;
        const float dy = other->GetY() - y_;
        const float dist = std::sqrt(dx * dx + dy * dy);

        if (dist < best_distance) {
            best_distance = dist;
            nearest = other;
        }
    }

    return nearest;
}

float Unit::EvaluateTargetPriority(const Unit *target) const {
    if (target == nullptr || target == this || target->GetOwner() == owner_) return -std::numeric_limits<float>::max();

    const float dx = target->GetX() - x_;
    const float dy = target->GetY() - y_;
    const float dist = std::sqrt(dx * dx + dy * dy);
    const float pressure_bonus = static_cast<float>(target->GetFrontlinePressure()) * 1.5f;
    const float health_bonus = static_cast<float>(target->GetHealth()) * 0.25f;
    const float danger_penalty = static_cast<float>(GetFrontlinePressure()) * 1.0f;

    return (200.0f - dist) + pressure_bonus + health_bonus - danger_penalty;
}

Unit *Unit::FindBestThreatTarget() const {
    Unit *best_target = nullptr;
    float best_score = -std::numeric_limits<float>::max();

    for (Unit *other : UnitManager::Instance().GetUnits()) {
        if (other == nullptr || other == this || other->GetOwner() == owner_) continue;

        const float score = EvaluateTargetPriority(other);
        if (score > best_score) {
            best_score = score;
            best_target = other;
        }
    }

    return best_target;
}

int Unit::GetTownDefenseRating(int town_x, int town_y) const {
    const int distance = std::abs(static_cast<int>(x_) - town_x) + std::abs(static_cast<int>(y_) - town_y);
    if (distance <= 20) return 12 + GetRoleDefenseWeight() * 4;
    if (distance <= 60) return 8 + GetRoleDefenseWeight() * 2;
    return 0;
}

bool Unit::ShouldReturnToBase() const {
    if (base_x_ < 0 || base_y_ < 0) return false;

    const int distance = std::abs(static_cast<int>(x_) - base_x_) + std::abs(static_cast<int>(y_) - base_y_);
    return health_ < 60 || supply_ < 25 || distance > 220;
}

bool Unit::AtBase() const {
    return base_x_ >= 0 && base_y_ >= 0 && std::abs(static_cast<int>(x_) - base_x_) <= 4 && std::abs(static_cast<int>(y_) - base_y_) <= 4;
}

float Unit::GetFormationOffsetX() const {
    if (formation_style_ == FormationStyle::None) return 0.0f;

    const float spread = 18.0f + static_cast<float>(formation_index_) * 12.0f;
    switch (formation_style_) {
        case FormationStyle::Column:
            return std::cos(heading_ + (formation_index_ % 2 == 0 ? 0.0f : 1.57079632679f)) * spread;
        case FormationStyle::Wedge:
            return std::cos(heading_ + (formation_index_ % 2 == 0 ? 0.0f : 0.8f)) * spread;
        case FormationStyle::Line:
            return std::cos(heading_) * spread;
        default:
            return 0.0f;
    }
}

float Unit::GetFormationOffsetY() const {
    if (formation_style_ == FormationStyle::None) return 0.0f;

    const float spread = 18.0f + static_cast<float>(formation_index_) * 12.0f;
    switch (formation_style_) {
        case FormationStyle::Column:
            return std::sin(heading_ + (formation_index_ % 2 == 0 ? 0.0f : 1.57079632679f)) * spread;
        case FormationStyle::Wedge:
            return std::sin(heading_ + (formation_index_ % 2 == 0 ? 0.0f : 0.8f)) * spread;
        case FormationStyle::Line:
            return std::sin(heading_) * spread;
        default:
            return 0.0f;
    }
}

bool Unit::IsMoving() const {
    return HasMoveTarget();
}

std::vector<std::pair<int, int>> Unit::BuildRoute(int target_x, int target_y) const {
    if (target_x == static_cast<int>(std::round(x_)) && target_y == static_cast<int>(std::round(y_))) {
        return {};
    }

    const int start_cell_x = static_cast<int>(std::round(x_ / 32.0f));
    const int start_cell_y = static_cast<int>(std::round(y_ / 32.0f));
    const int goal_cell_x = static_cast<int>(std::round(target_x / 32.0f));
    const int goal_cell_y = static_cast<int>(std::round(target_y / 32.0f));

    const int radius = 36;
    const int min_x = std::max(0, std::min(start_cell_x, goal_cell_x) - radius);
    const int max_x = std::max(start_cell_x, goal_cell_x) + radius;
    const int min_y = std::max(0, std::min(start_cell_y, goal_cell_y) - radius);
    const int max_y = std::max(start_cell_y, goal_cell_y) + radius;

    const int width = max_x - min_x + 1;
    const int height = max_y - min_y + 1;
    std::vector<std::vector<int>> g_cost(width, std::vector<int>(height, std::numeric_limits<int>::max()));
    std::vector<std::vector<int>> parent_x(width, std::vector<int>(height, -1));
    std::vector<std::vector<int>> parent_y(width, std::vector<int>(height, -1));

    const auto cell_id = [&](int cx, int cy) {
        return (cy - min_y) * width + (cx - min_x);
    };

    const auto to_world_x = [&](int cx) { return (cx * 32) + 16; };
    const auto to_world_y = [&](int cy) { return (cy * 32) + 16; };

    std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, std::greater<std::pair<int, int>>> frontier;
    const int start_idx = cell_id(start_cell_x, start_cell_y);
    const int goal_idx = cell_id(goal_cell_x, goal_cell_y);
    g_cost[start_cell_x - min_x][start_cell_y - min_y] = 0;
    frontier.emplace(0, start_idx);

    while (!frontier.empty()) {
        const auto current = frontier.top();
        frontier.pop();

        const int cx = current.second % width + min_x;
        const int cy = current.second / width + min_y;
        const int gx = cx - min_x;
        const int gy = cy - min_y;

        if (current.second == goal_idx) {
            break;
        }

        for (int ox = -1; ox <= 1; ++ox) {
            for (int oy = -1; oy <= 1; ++oy) {
                if (ox == 0 && oy == 0) continue;
                if (std::abs(ox) == 1 && std::abs(oy) == 1) continue;

                const int nx = cx + ox;
                const int ny = cy + oy;
                if (nx < min_x || nx > max_x || ny < min_y || ny > max_y) continue;

                const int n_gx = nx - min_x;
                const int n_gy = ny - min_y;
                const int step_cost = static_cast<int>(10.0f * GetTerrainPenaltyAt(nx * 32, ny * 32));
                const int tentative = g_cost[gx][gy] + step_cost;

                if (tentative < g_cost[n_gx][n_gy]) {
                    g_cost[n_gx][n_gy] = tentative;
                    parent_x[n_gx][n_gy] = cx;
                    parent_y[n_gx][n_gy] = cy;
                    const int heuristic = std::abs(goal_cell_x - nx) + std::abs(goal_cell_y - ny);
                    frontier.emplace(tentative + heuristic, cell_id(nx, ny));
                }
            }
        }
    }

    std::vector<std::pair<int, int>> route;
    int cx = goal_cell_x;
    int cy = goal_cell_y;
    if (cx >= min_x && cx <= max_x && cy >= min_y && cy <= max_y && g_cost[cx - min_x][cy - min_y] != std::numeric_limits<int>::max()) {
        while (cx != start_cell_x || cy != start_cell_y) {
            route.emplace_back(to_world_x(cx), to_world_y(cy));
            const int px = parent_x[cx - min_x][cy - min_y];
            const int py = parent_y[cx - min_x][cy - min_y];
            if (px == -1 || py == -1) break;
            cx = px;
            cy = py;
        }
    }

    if (route.empty()) {
        route.emplace_back(target_x, target_y);
        return route;
    }

    std::reverse(route.begin(), route.end());
    route.emplace_back(target_x, target_y);
    return route;
}

void Unit::tick() {
    if (attack_cooldown_ > 0) {
        --attack_cooldown_;
    }

    if (base_x_ >= 0 && base_y_ >= 0) {
        const int dist = std::abs(static_cast<int>(x_) - base_x_) + std::abs(static_cast<int>(y_) - base_y_);
        if (dist > 180) {
            supply_ = std::max(0, supply_ - 2);
        } else if (dist < 60) {
            supply_ = std::min(100, supply_ + 1);
        }
    }

    if (health_ > 0 && health_ < 100 && AtBase()) {
        health_ = std::min(100, health_ + 2);
        supply_ = std::min(100, supply_ + 2);
    }

    if (IsUnderSupplyPressure() && !AtBase()) {
        health_ = std::max(0, health_ - 1);
    }

    if (formation_leader_id_ >= 0 && formation_leader_id_ != id_) {
        Unit *leader = UnitManager::Instance().GetUnitById(formation_leader_id_);
        if (leader != nullptr && leader->IsAlive()) {
            const float target_x = leader->GetX() + GetFormationOffsetX();
            const float target_y = leader->GetY() + GetFormationOffsetY();
            move_target_x_ = static_cast<int>(target_x);
            move_target_y_ = static_cast<int>(target_y);
        }
    }

    Unit *nearest_enemy = FindNearestEnemy();
    Unit *priority_target = FindBestThreatTarget();
    const int frontline_pressure = GetFrontlinePressure();
    const bool low_health = health_ < 35;
    const bool low_supply = supply_ < 25;
    const bool enemy_in_range = nearest_enemy != nullptr && std::sqrt((nearest_enemy->GetX() - x_) * (nearest_enemy->GetX() - x_) + (nearest_enemy->GetY() - y_) * (nearest_enemy->GetY() - y_)) <= GetAttackRange() * 1.5f;

    if (low_health || low_supply || frontline_pressure >= 7) {
        SetTacticalState(TacticalState::Retreat);
    } else if (frontline_pressure >= 5) {
        SetTacticalState(TacticalState::Hold);
    } else if (priority_target != nullptr && health_ > 40 && supply_ > 25) {
        SetTacticalState(TacticalState::Advance);
    }

    if (tactical_state_ == TacticalState::Retreat && base_x_ >= 0 && base_y_ >= 0) {
        SetMoveTarget(base_x_, base_y_);
        ClearAttackTarget();
    }

    if (priority_target != nullptr && tactical_state_ == TacticalState::Advance && health_ > 35 && supply_ > 25) {
        const float dist_to_enemy = std::sqrt((priority_target->GetX() - x_) * (priority_target->GetX() - x_) + (priority_target->GetY() - y_) * (priority_target->GetY() - y_));
        if (dist_to_enemy <= GetAttackRange() * 2.0f) {
            SetAttackTarget(priority_target->get_id());
        } else {
            SetMoveTarget(static_cast<int>(priority_target->GetX()), static_cast<int>(priority_target->GetY()));
        }
    } else if (priority_target == nullptr && tactical_state_ == TacticalState::Advance && health_ > 35 && supply_ > 25) {
        const int objective_x = UnitManager::Instance().GetObjectiveX();
        const int objective_y = UnitManager::Instance().GetObjectiveY();
        const float dist_to_objective = std::sqrt((objective_x - x_) * (objective_x - x_) + (objective_y - y_) * (objective_y - y_));
        if (dist_to_objective > 8.0f) {
            SetMoveTarget(objective_x, objective_y);
        }
    }

    if (HasAssignedObjective()) {
        const float dist_to_objective = std::sqrt((assigned_objective_x_ - x_) * (assigned_objective_x_ - x_) + (assigned_objective_y_ - y_) * (assigned_objective_y_ - y_));
        if (dist_to_objective > 8.0f && !HasAttackTarget() && !HasMoveTarget()) {
            SetMoveTarget(assigned_objective_x_, assigned_objective_y_);
        }
    }

    if (tactical_state_ == TacticalState::Hold && priority_target != nullptr) {
        const float dist_to_enemy = std::sqrt((priority_target->GetX() - x_) * (priority_target->GetX() - x_) + (priority_target->GetY() - y_) * (priority_target->GetY() - y_));
        if (dist_to_enemy <= GetAttackRange() * 1.6f) {
            SetAttackTarget(priority_target->get_id());
        }
    }

    if (tactical_state_ == TacticalState::Retreat && base_x_ >= 0 && base_y_ >= 0) {
        SetMoveTarget(base_x_, base_y_);
    }

    if (enemy_in_range && nearest_enemy != nullptr && tactical_state_ != TacticalState::Retreat) {
        SetAttackTarget(nearest_enemy->get_id());
    }

    if (!route_points_.empty() && route_index_ < static_cast<int>(route_points_.size())) {
        move_target_x_ = route_points_[route_index_].first;
        move_target_y_ = route_points_[route_index_].second;
    }

    if (attack_target_id_ >= 0) {
        Unit *target = UnitManager::Instance().GetUnitById(attack_target_id_);
        if (target == nullptr || !target->IsAlive()) {
            ClearAttackTarget();
        } else {
            const float dx = target->GetX() - x_;
            const float dy = target->GetY() - y_;
            const float dist = std::sqrt(dx * dx + dy * dy);
            const float attack_range = GetAttackRange();

            if (dist <= attack_range) {
                const float target_angle = std::atan2(dy, dx);
                const float delta = std::atan2(std::sin(target_angle - heading_), std::cos(target_angle - heading_));
                const float turn_rate = 0.20f;
                if (std::abs(delta) > 0.01f) {
                    heading_ += std::copysign(std::min(std::abs(delta), turn_rate), delta);
                }

                if (attack_cooldown_ <= 0) {
                    target->TakeDamage(GetAttackDamage());
                    attack_cooldown_ = 20;
                }
                return;
            }

            SetMoveTarget(static_cast<int>(target->GetX()), static_cast<int>(target->GetY()));
            ClearFormation();
        }
    }

    if (ShouldReturnToBase()) {
        SetMoveTarget(base_x_, base_y_);
        ClearFormation();
    }

    if (!HasMoveTarget()) return;

    const float dx = move_target_x_ - x_;
    const float dy = move_target_y_ - y_;
    const float dist = std::sqrt(dx * dx + dy * dy);
    if (dist <= 1.0f) {
        if (!route_points_.empty()) {
            ++route_index_;
            if (route_index_ < static_cast<int>(route_points_.size())) {
                move_target_x_ = route_points_[route_index_].first;
                move_target_y_ = route_points_[route_index_].second;
                return;
            }
        }

        if (base_x_ >= 0 && base_y_ >= 0 && move_target_x_ == base_x_ && move_target_y_ == base_y_) {
            health_ = std::min(100, health_ + 10);
        }
        Stop();
        return;
    }

    const float target_angle = std::atan2(dy, dx);
    const float delta = std::atan2(std::sin(target_angle - heading_), std::cos(target_angle - heading_));
    const float turn_rate = 0.18f;

    if (std::abs(delta) > 0.01f) {
        heading_ += std::copysign(std::min(std::abs(delta), turn_rate), delta);
    }

    const float terrain_penalty = GetTerrainPenaltyAt(static_cast<int>(x_), static_cast<int>(y_));
    const float supply_penalty = GetSupplyPenalty();
    const float speed = (GetMoveSpeed() / terrain_penalty) / supply_penalty;
    const float step_x = std::cos(heading_) * speed;
    const float step_y = std::sin(heading_) * speed;

    if (std::sqrt(step_x * step_x + step_y * step_y) >= dist) {
        x_ = static_cast<float>(move_target_x_);
        y_ = static_cast<float>(move_target_y_);
        Stop();
        return;
    }

    x_ += step_x;
    y_ += step_y;
}

void Unit::SetMoveTarget(int wx, int wy) {
    route_points_ = BuildRoute(wx, wy);
    route_index_ = 0;
    if (route_points_.empty()) {
        move_target_x_ = wx;
        move_target_y_ = wy;
        return;
    }

    move_target_x_ = route_points_[0].first;
    move_target_y_ = route_points_[0].second;
}

void Unit::SetTacticalState(TacticalState state) {
    tactical_state_ = state;
}

void Unit::SetFormation(int leader_id, int formation_index, FormationStyle style) {
    formation_leader_id_ = leader_id;
    formation_index_ = formation_index;
    formation_style_ = style;
}

void Unit::ClearFormation() {
    formation_leader_id_ = -1;
    formation_index_ = 0;
    formation_style_ = FormationStyle::None;
}

bool Unit::HasFormation() const {
    return formation_leader_id_ >= 0 && formation_style_ != FormationStyle::None;
}

void Unit::SetBasePosition(int wx, int wy) {
    base_x_ = wx;
    base_y_ = wy;
    if (health_ < 60 && (base_x_ >= 0 && base_y_ >= 0)) {
        SetMoveTarget(base_x_, base_y_);
    }
}

bool Unit::HasBase() const {
    return base_x_ >= 0 && base_y_ >= 0;
}

void Unit::SetAssignedObjective(int wx, int wy) {
    assigned_objective_x_ = wx;
    assigned_objective_y_ = wy;
}

bool Unit::HasAssignedObjective() const {
    return assigned_objective_x_ >= 0 && assigned_objective_y_ >= 0;
}

void Unit::SetAttackTarget(int target_unit_id) {
    attack_target_id_ = target_unit_id;
    move_target_x_ = -1;
    move_target_y_ = -1;
}

void Unit::ClearAttackTarget() {
    attack_target_id_ = -1;
    attack_cooldown_ = 0;
}

bool Unit::HasMoveTarget() const {
    return move_target_x_ >= 0 && move_target_y_ >= 0;
}

bool Unit::HasAttackTarget() const {
    return attack_target_id_ >= 0;
}

void Unit::TakeDamage(int amount) {
    health_ = std::max(0, health_ - amount);
    if (health_ <= 0) {
        ClearAttackTarget();
        ClearFormation();
        Stop();
    }
}

void Unit::Stop() {
    route_points_.clear();
    route_index_ = 0;
    move_target_x_ = -1;
    move_target_y_ = -1;
}
