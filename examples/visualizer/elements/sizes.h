#pragma once

#include "board.h"

namespace slvs
{
    constexpr int subdivisions = std::max(sltd::box_width, sltd::box_height);
    constexpr float cell_size = 60;
    constexpr float candidate_region_size = cell_size / subdivisions;
    constexpr float box_divider_width = 4;
    constexpr float cell_divider_width = 2;
    constexpr float grid_size = cell_size * sltd::board_size; ///< Only the main grid region
    constexpr float full_grid_size = grid_size + box_divider_width; ///< The whole image
    constexpr float filled_number_size = 40;
    constexpr float candidate_number_size = filled_number_size / subdivisions;
} // namespace slvs
