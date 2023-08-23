#pragma once

#include <cstdint>
#include <bitset>
#include <string_view>
#include <string>

#include "export.h"

namespace sltd
{
    inline constexpr int box_width = 3;
    inline constexpr int box_height = 3;
    inline constexpr int board_size = box_width * box_height;
    inline constexpr int cell_count = board_size * board_size;
    static_assert(board_size <= 16, "Max candidate count is 16");

    using CandidateMask = std::uint16_t;
    using PatternMask = std::bitset<cell_count>;
    inline constexpr CandidateMask full_mask = ~(~CandidateMask{} << board_size);

    struct SOLITUDE_API Board final
    {
        CandidateMask cells[cell_count]{};

        constexpr CandidateMask& at(const int i, const int j) noexcept { return cells[i * board_size + j]; }
        constexpr CandidateMask at(const int i, const int j) const noexcept { return cells[i * board_size + j]; }

        PatternMask pattern_of(int number) const noexcept; // Number is 0-based
        void apply_pattern(int number, PatternMask mask) noexcept; // Number is 0-based

        static Board empty_board() noexcept;
        static Board random_filled_board() noexcept;
        static Board from_repr(std::string_view str);

        int brute_force_solve(int max_solutions = 1'000, bool randomized = false);
        int brute_force_solve2(int max_solutions = 1'000, bool randomized = false);

        void print() const;
        std::string repr() const;
    };
} // namespace sltd
