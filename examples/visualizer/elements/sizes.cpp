#include "sizes.h"

namespace slvs
{
    SkVector get_candidate_position(const int row, const int column, const int candidate)
    {
        const auto inner_offset =
            SkVector{subdivisions - sltd::box_width, subdivisions - sltd::box_height} * (candidate_region_size / 2) +
            SkVector{candidates_inset, candidates_inset};
        const auto cell_offset = SkVector{static_cast<float>(column), static_cast<float>(row)} * cell_size;
        const int kx = candidate % sltd::box_height, ky = candidate / sltd::box_width;
        const auto candidate_offset =
            SkVector{static_cast<float>(kx) + 0.5f, static_cast<float>(ky) + 0.5f} * candidate_region_size;
        return inner_offset + cell_offset + candidate_offset;
    }
} // namespace slvs
