#pragma once

#include <array>

#include "solitude/board.h"
#include "solitude/utils.h"

namespace sltd
{
    constexpr PatternMask get_first_column_mask()
    {
        PatternMask res;
        for (int i = 0; i < board_size; i++)
            res.set(i * board_size);
        return res;
    }

    constexpr PatternMask get_first_box_mask()
    {
        PatternMask res;
        for (int i = 0; i < box_width; i++)
            for (int j = 0; j < box_height; j++)
                res.set(i * board_size + j);
        return res;
    }

    inline constexpr PatternMask first_row = PatternMask(full_mask);
    inline constexpr PatternMask first_column = get_first_column_mask();
    inline constexpr PatternMask first_box = get_first_box_mask();

    constexpr PatternMask nth_row(const int i) { return first_row << (i * board_size); }
    constexpr PatternMask nth_column(const int i) { return first_column << i; }
    constexpr PatternMask nth_box(const int i)
    {
        return first_box << (i % box_height * box_width + i / box_height * box_height * board_size);
    }

    constexpr auto generate_house_patterns()
    {
        std::array<PatternMask, 3 * board_size> res; // NOLINT
        for (int i = 0; i < board_size; i++)
        {
            res[i] = nth_row(i);
            res[i + board_size] = nth_column(i);
            res[i + 2 * board_size] = nth_box(i);
        }
        return res;
    }

    inline constexpr auto house_patterns = generate_house_patterns();

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

    template <int Size, typename T>
        requires std::is_unsigned_v<T>
    std::array<int, Size> bitmask_to_array(const T bits)
    {
        std::array<int, Size> res{};
        for (int i = 0; const int idx : sltd::set_bit_indices(bits))
            res[i++] = idx;
        return res;
    }

    template <int Size, int MaskSize>
    std::array<int, Size> bitmask_to_array(const Bitset<MaskSize>& bits)
    {
        std::array<int, Size> res{};
        for (int i = 0; const int idx : bits.set_bit_indices())
            res[i++] = idx;
        return res;
    }

    constexpr PatternMask pattern_from_indices_and_bits(const int* idx, const CandidateMask mask)
    {
        PatternMask res;
        for (const int i : set_bit_indices(mask))
            res.set(idx[i]);
        return res;
    }

    std::string cell_name(int idx);
    std::string house_name(int idx);
    std::string describe_houses(HouseMask houses, char separator = ',');
    std::string describe_cells_in_house(int house_idx, CandidateMask idx_mask, char separator = ',');
    std::string describe_cells(PatternMask cells, char separator = ',');
    std::string describe_candidates(CandidateMask candidates, char separator = ',');
} // namespace sltd
