#include "solitude/strategies/intersection.h"

#include <fmt/format.h>

#include "common.h"

namespace sltd
{
    namespace
    {
        auto extract_patterns(const Board& board, const int number)
        {
            const CandidateMask bit = 1 << number;
            std::array<CandidateMask, house_indices.size()> res{};
            for (std::size_t i = 0; i < house_indices.size(); i++)
            {
                const auto& house = house_indices[i];
                for (int j = 0; j < board_size; j++)
                    res[i] |= ((board.cells[house[j]] & bit) != 0) << j;
            }
            return res;
        }

        constexpr CandidateMask calc_box_column_intersection()
        {
            CandidateMask res = 0;
            for (int i = 0; i < board_size; i += box_width)
                res |= 1 << i;
            return res;
        }

        constexpr CandidateMask row_box_intersection = ~(~CandidateMask{} << box_width);
        constexpr CandidateMask column_box_intersection = ~(~CandidateMask{} << box_height);
        constexpr CandidateMask box_column_intersection = calc_box_column_intersection();
    } // namespace

    std::string Intersection::description() const
    {
        return fmt::format("Intersection: {} -> {}, {}={}", //
            house_name(base_house_idx), house_name(cover_house_idx), //
            describe_cells_in_house(base_house_idx, base_idx_mask, '/'), //
            describe_candidates(candidate));
    }

    std::optional<Intersection> Intersection::try_find(const Board& board)
    {
        for (int number = 0; number < board_size; number++)
        {
            const auto patterns = extract_patterns(board, number);
            for (int box = 0; box < board_size; box++)
            {
                const auto box_pattern = patterns[2 * board_size + box];
                const int box_row = box / box_height, box_col = box % box_height;
                const auto row_intersection = row_box_intersection << (box_width * box_col);
                const auto col_intersection = column_box_intersection << (box_height * box_row);
                const int row_offset = box_row * box_height, col_offset = box_col * box_width;
                // Box-row intersection
                for (int row = 0; row < box_height; row++)
                {
                    const auto row_pattern = patterns[row_offset + row];
                    const auto box_intersection = row_box_intersection << (box_width * row);
                    const bool box_is_base = (box_pattern & box_intersection) == box_pattern;
                    const bool row_is_base = (row_pattern & row_intersection) == row_pattern;
                    if (box_is_base != row_is_base)
                        return Intersection{
                            .base_house_idx = box_is_base ? 2 * board_size + box : row_offset + row,
                            .cover_house_idx = box_is_base ? row_offset + row : 2 * board_size + box,
                            .candidate = static_cast<CandidateMask>(1 << number),
                            .base_idx_mask = box_is_base ? box_pattern : row_pattern,
                            .cover_idx_mask = box_is_base ? row_pattern : box_pattern,
                            .cover_elimination_idx_mask = static_cast<CandidateMask>(
                                ~(box_is_base ? (row_pattern & row_intersection) : (box_pattern & box_intersection)) &
                                full_mask) //
                        };
                }
                // Box-col intersection
                for (int col = 0; col < box_width; col++)
                {
                    const auto col_pattern = patterns[board_size + col_offset + col];
                    const auto box_intersection = box_column_intersection << col;
                    const bool box_is_base = (box_pattern & box_intersection) == box_pattern;
                    const bool col_is_base = (col_pattern & col_intersection) == col_pattern;
                    if (box_is_base != col_is_base)
                        return Intersection{
                            .base_house_idx = box_is_base ? 2 * board_size + box : board_size + col_offset + col,
                            .cover_house_idx = box_is_base ? board_size + col_offset + col : 2 * board_size + box,
                            .candidate = static_cast<CandidateMask>(1 << number),
                            .base_idx_mask = box_is_base ? box_pattern : col_pattern,
                            .cover_idx_mask = box_is_base ? col_pattern : box_pattern,
                            .cover_elimination_idx_mask = static_cast<CandidateMask>(
                                ~(box_is_base ? (col_pattern & col_intersection) : (box_pattern & box_intersection)) &
                                full_mask) //
                        };
                }
            }
        }
        return std::nullopt;
    }

    void Intersection::apply_to(Board& board) const
    {
        const auto& house = house_indices[cover_house_idx];
        for (int i = 0; i < board_size; i++)
            if (cover_elimination_idx_mask & (1 << i))
                board.cells[house[i]] &= ~candidate;
    }
} // namespace sltd
