#include "solitude/strategies/sue_de_coq.h"

#include <fmt/format.h>

#include "common.h"

namespace sltd
{
    namespace
    {
        struct IntersectionInfo
        {
            bool is_row = false;
            int box_idx = 0;
            int line_idx = 0;
        };

        struct IntersectionMasks
        {
            PatternMask line;
            PatternMask box;

            constexpr PatternMask intersection() const noexcept { return line & box; }
        };

        constexpr auto get_intersection_info(const PatternMask& pattern)
        {
            auto it = pattern.set_bit_indices().begin();
            const int first = *it, second = *++it;
            const int row1 = first / board_size, col1 = first % board_size, row2 = second / board_size;
            const int box = row1 / box_height * box_height + col1 / box_width;
            return IntersectionInfo{
                .is_row = row1 == row2,
                .box_idx = box,
                .line_idx = row1 == row2 ? row1 : col1 //
            };
        }

        constexpr auto generate_intersection_masks()
        {
            constexpr int row_intersection_count = board_size * box_height;
            constexpr int col_intersection_count = board_size * box_width;
            std::array<IntersectionMasks, row_intersection_count + col_intersection_count> res;
            int i = 0;
            for (int row = 0; row < board_size; row++)
            {
                const int box_begin = row / box_height * box_height;
                for (int box = box_begin; box < box_begin + box_height; box++)
                    res[i++] = {.line = nth_row(row), .box = nth_box(box)};
            }
            for (int col = 0; col < board_size; col++)
            {
                const int box_begin = col / box_width * box_width;
                for (int box = box_begin; box < board_size; box += box_height)
                    res[i++] = {.line = nth_column(col), .box = nth_box(box)};
            }
            return res;
        }

        constexpr auto all_intersections = generate_intersection_masks();

        PatternMask find_eliminations(const Board& board, PatternMask unfiltered, const CandidateMask candidates)
        {
            for (const int i : unfiltered.set_bit_indices())
                if (!(board.cells[i] & candidates))
                    unfiltered.reset(i);
            return unfiltered;
        }

        std::optional<SueDeCoq> find_basic_sdc(const Board& board)
        {
            for (const auto& masks : all_intersections)
            {
                const auto inter = masks.intersection();
                const auto not_filled = ~board.filled & inter;
                const CandidateMask max_idx_mask = (1 << not_filled.count());
                if (max_idx_mask < 0b100) // Unfilled cell count < 2
                    continue;
                const auto cell_idx = bitmask_to_array<box_width>(not_filled);
                for (CandidateMask common_set = 0b11; common_set < max_idx_mask; common_set++)
                {
                    const auto common_set_size = std::popcount(common_set);
                    if (common_set_size < 2) // We need at least 2 cells in the intersection
                        continue;
                    CandidateMask common_candidates{};
                    PatternMask common_cells;
                    for (const int i : set_bit_indices(common_set))
                    {
                        common_candidates |= board.cells[cell_idx[i]];
                        common_cells.set(cell_idx[i]);
                    }
                    if (std::popcount(common_candidates) != common_set_size + 2) // Basic SdC
                        continue;
                    // Two bivalue cells
                    const auto line_remain = masks.line & ~common_cells;
                    const auto box_remain = masks.box & ~common_cells;
                    for (const int line_cell : line_remain.set_bit_indices())
                    {
                        const auto line_candidates = board.cells[line_cell];
                        if ((line_candidates & common_candidates) != line_candidates ||
                            std::popcount(line_candidates) != 2) // Need a bivalue cell
                            continue;
                        for (const int box_cell : box_remain.set_bit_indices())
                        {
                            const auto box_candidates = board.cells[box_cell];
                            // Need another bivalue cell with candidates not in the other one
                            if ((line_candidates & box_candidates) ||
                                (box_candidates & common_candidates) != box_candidates ||
                                std::popcount(box_candidates) != 2)
                                continue;
                            const CandidateMask line_eliminated_candidates =
                                line_candidates | (common_candidates & ~box_candidates);
                            const CandidateMask box_eliminated_candidates =
                                box_candidates | (common_candidates & ~line_candidates);
                            const auto line_eliminations = find_eliminations(
                                board, PatternMask(line_remain).reset(line_cell), line_eliminated_candidates);
                            const auto box_eliminations = find_eliminations(
                                board, PatternMask(box_remain).reset(box_cell), box_eliminated_candidates);
                            if (line_eliminations.any() || box_eliminations.any())
                                return SueDeCoq{
                                    .intersection_cells = common_cells,
                                    .line_cells = PatternMask{}.set(line_cell),
                                    .box_cells = PatternMask{}.set(box_cell),
                                    .intersection_candidates = common_candidates,
                                    .line_candidates = line_candidates,
                                    .box_candidates = box_candidates,
                                    .line_eliminations = line_eliminations,
                                    .box_eliminations = box_eliminations //
                                };
                        }
                    }
                }
            }
            return std::nullopt;
        }
    } // namespace

    std::string SueDeCoq::description() const
    {
        // TODO
        const auto info = get_intersection_info(intersection_cells);
        return fmt::format("Sue de Coq: intersection of {}{} and b{}, "
                           "with {}={}, {}={}, {}={} => {}!={}, {}!={}", //
            info.is_row ? 'r' : 'c', info.line_idx + 1, info.box_idx + 1, //
            describe_cells(intersection_cells), describe_candidates(intersection_candidates), //
            describe_cells(line_cells), describe_candidates(line_candidates), //
            describe_cells(box_cells), describe_candidates(box_candidates), //
            describe_cells(line_eliminations), describe_candidates(line_eliminated_candidates()), //
            describe_cells(box_eliminations), describe_candidates(box_eliminated_candidates()));
    }

    std::optional<SueDeCoq> SueDeCoq::try_find(const Board& board, const bool extended)
    {
        return extended ? std::nullopt /*TODO*/ : find_basic_sdc(board);
    }

    void SueDeCoq::apply_to(Board& board) const
    {
        const CandidateMask lec = line_eliminated_candidates();
        const CandidateMask bec = box_eliminated_candidates();
        for (const auto i : line_eliminations.set_bit_indices())
            board.cells[i] &= ~lec;
        for (const auto i : box_eliminations.set_bit_indices())
            board.cells[i] &= ~bec;
    }
} // namespace sltd
