#pragma once

#include <numbers>

#include "board.h"

namespace slvs
{
    constexpr float pi = std::numbers::pi_v<float>;
    constexpr int subdivisions = std::max(sltd::box_width, sltd::box_height);
    constexpr float cell_size = 60;
    constexpr float candidates_inset = 3;
    constexpr float candidate_region_size = (cell_size - 2 * candidates_inset) / subdivisions;
    constexpr float candidate_circle_radius = candidate_region_size / 2 * 0.95f;
    constexpr float candidate_multicolor_offset_degrees = -70;
    constexpr float box_divider_width = 4;
    constexpr float cell_divider_width = 2;
    constexpr float grid_size = cell_size * sltd::board_size; ///< Only the main grid region
    constexpr float full_grid_size = grid_size + box_divider_width; ///< The whole image
    constexpr float filled_number_size = 40;
    constexpr float candidate_number_size = filled_number_size / subdivisions;
    constexpr float arrow_width = 0.35f * candidate_circle_radius;
    constexpr float arrow_tip_angle = pi / 4;
    constexpr float arrow_tip_inner_length = 3.5f * arrow_width;
    constexpr float arrow_tip_outer_length = 6 * arrow_width;
    constexpr float arrow_tip_clip_radius = 0.8f * arrow_tip_inner_length;
    constexpr float arrow_bend_angle = pi / 6;
    constexpr float arrow_bend_distance = 2.0f * candidate_region_size;
    constexpr float arrow_dash_intervals[]{4 * arrow_width, 3 * arrow_width};

    SkRect get_cell_rect(int cell);
    SkVector get_candidate_position(int cell, int candidate); ///< Center point of a candidate
} // namespace slvs
