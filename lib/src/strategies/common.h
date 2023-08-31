#pragma once

#include <array>

#include "solitude/board.h"
#include "solitude/utils.h"

namespace sltd
{
    constexpr auto generate_house_indices()
    {
        // 3 * board_size for rows, columns, and boxes
        std::array<std::array<int, board_size>, 3 * board_size> res{}; // NOLINT
        // Rows and columns
        for (int i = 0; i < board_size; i++)
            for (int j = 0; j < board_size; j++)
            {
                res[i][j] = i * board_size + j;
                res[j + board_size][i] = i * board_size + j;
            }
        // Boxes
        for (int i = 0; i < board_size; i++)
        {
            const int br = i / box_height * box_height, bc = i % box_height * box_width;
            for (int j = 0; j < board_size; j++)
                res[i + 2 * board_size][j] = (br + j / box_width) * board_size + bc + j % box_width;
        }
        return res;
    }

    inline constexpr auto house_indices = generate_house_indices();

    std::array<CandidateMask, board_size> extract_row_patterns(const Board& board, int number);
    std::array<CandidateMask, board_size> extract_column_patterns(const Board& board, int number);
    std::array<CandidateMask, board_size> extract_box_patterns(const Board& board, int number);

    constexpr CandidateMask calc_box_column_intersection()
    {
        CandidateMask res = 0;
        for (int i = 0; i < board_size; i += box_width)
            res |= 1 << i;
        return res;
    }

    inline constexpr CandidateMask row_box_intersection = ~(~CandidateMask{} << box_width);
    inline constexpr CandidateMask column_box_intersection = ~(~CandidateMask{} << box_height);
    inline constexpr CandidateMask box_column_intersection = calc_box_column_intersection();

    constexpr auto generate_peer_masks()
    {
        std::array<PatternMask, cell_count> res{};
        for (const auto& house : house_indices)
        {
            PatternMask house_mask;
            for (const int i : house)
                house_mask.set(i);
            for (const int i : house)
                res[i] |= house_mask;
        }
        for (int i = 0; i < cell_count; i++)
            res[i].reset(i); // A cell is not a peer to itself
        return res;
    }

    inline constexpr auto peer_masks = generate_peer_masks();

    constexpr int countr_zero(const PatternMask& pattern) { return *pattern.set_bit_indices().begin(); }
    PatternMask nvalue_cells(const Board& board, int n);

    template <int Size>
    std::array<int, Size> candidate_mask_to_array(const CandidateMask candidates)
    {
        std::array<int, Size> res{};
        for (int i = 0; const int idx : set_bit_indices(candidates))
            res[i++] = idx;
        return res;
    }

    std::string cell_name(int idx);
    std::string house_name(int idx);
    std::string describe_houses(HouseMask houses, char separator = ',');
    std::string describe_cells_in_house(int house_idx, CandidateMask idx_mask, char separator = ',');
    std::string describe_cells(PatternMask cells, char separator = ',');
    std::string describe_candidates(CandidateMask candidates, char separator = ',');
} // namespace sltd
