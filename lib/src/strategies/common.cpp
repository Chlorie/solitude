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
} // namespace sltd
