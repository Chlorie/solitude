#pragma once

#include <cstdint>
#include <bitset>
#include <string_view>
#include <string>

#include "export.h"

SOLITUDE_SUPPRESS_EXPORT_WARNING

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
        PatternMask filled;

        auto filled_at(const int r, const int c) noexcept { return filled[r * board_size + c]; }
        bool filled_at(const int r, const int c) const noexcept { return filled[r * board_size + c]; }
        auto& at(const int r, const int c) noexcept { return cells[r * board_size + c]; }
        auto at(const int r, const int c) const noexcept { return cells[r * board_size + c]; }

        void set_number_at(const int num, const int idx) noexcept
        {
            cells[idx] = 1 << num;
            filled.set(idx);
        }
        void set_number_at(const int num, const int r, const int c) noexcept { set_number_at(num, r * board_size + c); }
        void set_number_at(int num, PatternMask mask) noexcept;

        void set_unknown_at(const int idx) noexcept
        {
            cells[idx] = full_mask;
            filled.set(idx, false);
        }
        void set_unknown_at(const int r, const int c) noexcept { set_unknown_at(r * board_size + c); }

        PatternMask pattern_of(int num) const noexcept;

        static Board empty_board() noexcept;
        static Board random_filled_board() noexcept;
        static Board from_repr(std::string_view str);
        static Board from_full_repr(std::string_view str);

        bool eliminate_candidates_from_naked_single(const int idx) noexcept
        {
            return eliminate_candidates_from_naked_single(idx / board_size, idx % board_size);
        }
        bool eliminate_candidates_from_naked_single(int r, int c) noexcept;
        bool eliminate_candidates() noexcept;

        int brute_force_solve(int max_solutions = 1'000, bool randomized = false);

        void print(bool display_candidates_with_braille_dots = false) const;
        std::string repr() const;
        std::string full_repr() const;
    };
} // namespace sltd

SOLITUDE_RESTORE_EXPORT_WARNING
