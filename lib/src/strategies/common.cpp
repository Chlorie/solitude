#include "common.h"

#include <fmt/format.h>

#include "solitude/utils.h"

namespace sltd
{
    namespace
    {
        std::array<CandidateMask, board_size> extract_patterns(
            const Board& board, const int number, const int pattern_idx_offset)
        {
            const CandidateMask bit = 1 << number;
            std::array<CandidateMask, board_size> res{};
            for (std::size_t i = 0; i < board_size; i++)
            {
                const auto& house = house_indices[i + pattern_idx_offset];
                for (int j = 0; j < board_size; j++)
                    res[i] |= ((board.cells[house[j]] & bit) != 0) << j;
            }
            return res;
        }

    } // namespace

    std::array<CandidateMask, board_size> extract_row_patterns(const Board& board, const int number)
    {
        return extract_patterns(board, number, 0);
    }

    std::array<CandidateMask, board_size> extract_column_patterns(const Board& board, const int number)
    {
        return extract_patterns(board, number, board_size);
    }

    std::array<CandidateMask, board_size> extract_box_patterns(const Board& board, const int number)
    {
        return extract_patterns(board, number, 2 * board_size);
    }

    PatternMask nvalue_cells(const Board& board, const int n)
    {
        PatternMask res;
        for (int i = 0; i < cell_count; i++)
            if (std::popcount(board.cells[i]) == n)
                res.set(i);
        return res;
    }

    std::string cell_name(const int idx)
    {
        return fmt::format("r{}c{}", //
            idx / board_size + 1, idx % board_size + 1);
    }

    std::string house_name(const int idx) { return fmt::format("{}{}", "rcb"[idx / board_size], idx % board_size + 1); }

    std::string describe_houses(const HouseMask houses, const char separator)
    {
        if (houses == 0)
            return "∅";
        std::string res;
        for (const int i : set_bit_indices(houses))
            res += fmt::format("{}{}{}", "rcb"[i / board_size], i % board_size + 1, separator);
        res.pop_back();
        return res;
    }

    std::string describe_cells_in_house(const int house_idx, const CandidateMask idx_mask, const char separator)
    {
        if (idx_mask == 0)
            return "∅";
        std::string res;
        const auto& house = house_indices[house_idx];
        for (const int i : set_bit_indices(idx_mask))
            res += fmt::format("{}{}", cell_name(house[i]), separator);
        res.pop_back();
        return res;
    }

    std::string describe_cells(const PatternMask cells, const char separator)
    {
        if (cells.none())
            return "∅";
        std::string res;
        for (int i = 0; i < cell_count; i++)
            if (cells[i])
                res += fmt::format("{}{}", cell_name(i), separator);
        res.pop_back();
        return res;
    }

    std::string describe_candidates(const CandidateMask candidates, const char separator)
    {
        if (candidates == 0)
            return "∅";
        std::string res;
        for (const int i : set_bit_indices(candidates))
            res += fmt::format("{}{}", i + 1, separator);
        res.pop_back();
        return res;
    }
} // namespace sltd
