#include "common.h"

#include <fmt/format.h>

namespace sltd
{
    std::string cell_name(const int idx)
    {
        return fmt::format("r{}c{}", //
            idx / board_size + 1, idx % board_size + 1);
    }

    std::string house_name(const int idx)
    {
        static constexpr std::string_view house_types[]{"row", "column", "box"};
        return fmt::format("{} {}", //
            house_types[idx / board_size], idx % board_size + 1);
    }

    std::string describe_cells_in_house(const int house_idx, const CandidateMask idx_mask, const char separator)
    {
        if (idx_mask == 0)
            return "∅";
        std::string res;
        const auto& house = house_indices[house_idx];
        for (int i = 0; i < board_size; i++)
            if ((1 << i) & idx_mask)
                res += fmt::format("{}{}", cell_name(house[i]), separator);
        res.pop_back();
        return res;
    }

    std::string describe_candidates(const CandidateMask candidates, const char separator)
    {
        if (candidates == 0)
            return "∅";
        std::string res;
        for (int i = 0; i < board_size; i++)
            if ((1 << i) & candidates)
                res += fmt::format("{}{}", i + 1, separator);
        res.pop_back();
        return res;
    }
} // namespace sltd
