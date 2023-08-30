#include "solitude/strategies/intersection.h"

#include <fmt/format.h>

#include "common.h"
#include "solitude/utils.h"

namespace sltd
{
    std::string Intersection::description() const
    {
        return fmt::format("Intersection: {}->{}, {}={}", //
            house_name(base_house_idx), house_name(cover_house_idx), //
            describe_cells_in_house(base_house_idx, base_idx_mask, '/'), //
            describe_candidates(candidate));
    }

    std::optional<Intersection> Intersection::try_find(const Board& board)
    {
        for (int number = 0; number < board_size; number++)
        {
            const auto row_patterns = extract_row_patterns(board, number);
            const auto col_patterns = extract_column_patterns(board, number);
            const auto box_patterns = extract_box_patterns(board, number);
            for (int box = 0; box < board_size; box++)
            {
                const auto box_pattern = box_patterns[box];
                const int box_row = box / box_height, box_col = box % box_height;
                const auto row_intersection = row_box_intersection << (box_width * box_col);
                const auto col_intersection = column_box_intersection << (box_height * box_row);
                const int row_offset = box_row * box_height, col_offset = box_col * box_width;
                // Box-row intersection
                for (int row = 0; row < box_height; row++)
                {
                    const auto row_pattern = row_patterns[row_offset + row];
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
                    const auto col_pattern = col_patterns[col_offset + col];
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
        for (const int i : set_bit_indices(cover_elimination_idx_mask))
            board.cells[house[i]] &= ~candidate;
    }
} // namespace sltd
