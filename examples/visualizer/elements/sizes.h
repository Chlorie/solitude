#pragma once

#include "board.h"

namespace slvs
{
    constexpr int subdivisions = std::max(sltd::box_width, sltd::box_height);
    constexpr float cell_size = 60;
    constexpr float candidates_inset = 3;
    constexpr float candidate_region_size = (cell_size - 2 * candidates_inset) / subdivisions;
    constexpr float candidate_circle_radius = candidate_region_size / 2 * 0.95f;
    constexpr float box_divider_width = 4;
    constexpr float cell_divider_width = 2;
    constexpr float grid_size = cell_size * sltd::board_size; ///< Only the main grid region
    constexpr float full_grid_size = grid_size + box_divider_width; ///< The whole image
    constexpr float filled_number_size = 40;
    constexpr float candidate_number_size = filled_number_size / subdivisions;

    SkRect get_cell_rect(int row, int column);
    SkVector get_candidate_position(int row, int column, int candidate); ///< Center point of a candidate
} // namespace slvs
