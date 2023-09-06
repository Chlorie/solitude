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
                const int box_begin = col / box_width;
                for (int box = box_begin; box < board_size; box += box_height)
                    res[i++] = {.line = nth_column(col), .box = nth_box(box)};
            }
            return res;
        }

        constexpr auto all_intersections = generate_intersection_masks();

        constexpr PatternMask pattern_from_indices_and_bits(const int* idx, const CandidateMask mask)
        {
            PatternMask res;
            for (const int i : set_bit_indices(mask))
                res.set(idx[i]);
            return res;
        }

        PatternMask find_eliminations(const Board& board, PatternMask unfiltered, const CandidateMask candidates)
        {
            for (const int i : unfiltered.set_bit_indices())
                if (!(board.cells[i] & candidates))
                    unfiltered.reset(i);
            return unfiltered;
        }

        class SueDeCoqFinder
        {
        public:
            explicit SueDeCoqFinder(const Board& board): board_(board) {}

            std::optional<SueDeCoq> try_find(const bool extended)
            {
                // Iterate all the intersections
                for (const auto& masks : all_intersections)
                {
                    const auto inter = masks.intersection();
                    const auto not_filled = ~board_.filled & inter;
                    const CandidateMask max_idx_mask = (1 << not_filled.count());
                    if (max_idx_mask < 0b100) // Unfilled cell count < 2
                        continue;
                    const auto cell_idx = bitmask_to_array<box_width>(not_filled);
                    for (CandidateMask common_set = 0b11; common_set < max_idx_mask; common_set++)
                    {
                        if (common_set == 7)
                            continue;
                        const auto common_set_size = std::popcount(common_set);
                        if (common_set_size < 2) // We need at least 2 cells in the intersection
                            continue;
                        common_candidates_ = 0;
                        common_cells_ = pattern_from_indices_and_bits(cell_idx.data(), common_set);
                        for (const int i : set_bit_indices(common_set))
                            common_candidates_ |= board_.cells[cell_idx[i]];
                        // The intersection cells should be at least an AALS
                        // Basic SdC allows at most an AALS
                        common_extras_ = std::popcount(common_candidates_) - common_set_size;
                        if (common_extras_ < 2 || (!extended && common_extras_ != 2))
                            continue;
                        const auto remain_mask = ~common_cells_ & ~board_.filled;
                        line_remain_ = masks.line & remain_mask;
                        box_remain_ = masks.box & remain_mask;
                        extended ? find_line_and_box_als() : find_bivalue_line_and_box_cells();
                        if (res_)
                            return res_;
                    }
                }
                return std::nullopt;
            }

        private:
            Board board_;
            CandidateMask common_candidates_{};
            PatternMask common_cells_;
            int common_extras_ = 2;
            PatternMask line_remain_, box_remain_;
            std::optional<SueDeCoq> res_;

            void find_bivalue_line_and_box_cells()
            {
                for (const int line_cell : line_remain_.set_bit_indices())
                {
                    const auto line_candidates = board_.cells[line_cell];
                    if ((line_candidates & common_candidates_) != line_candidates ||
                        std::popcount(line_candidates) != 2) // Need a bivalue cell
                        continue;
                    for (const int box_cell : box_remain_.set_bit_indices())
                    {
                        const auto box_candidates = board_.cells[box_cell];
                        // Need another bivalue cell with candidates not in the other one
                        if ((line_candidates & box_candidates) ||
                            (box_candidates & common_candidates_) != box_candidates ||
                            std::popcount(box_candidates) != 2)
                            continue;
                        const CandidateMask line_eliminated_candidates =
                            line_candidates | (common_candidates_ & ~box_candidates);
                        const CandidateMask box_eliminated_candidates =
                            box_candidates | (common_candidates_ & ~line_candidates);
                        const auto line_eliminations = find_eliminations(
                            board_, PatternMask(line_remain_).reset(line_cell), line_eliminated_candidates);
                        const auto box_eliminations = find_eliminations(
                            board_, PatternMask(box_remain_).reset(box_cell), box_eliminated_candidates);
                        if (line_eliminations.any() || box_eliminations.any())
                        {
                            res_ = SueDeCoq{
                                .intersection_cells = common_cells_,
                                .line_cells = PatternMask{}.set(line_cell),
                                .box_cells = PatternMask{}.set(box_cell),
                                .intersection_candidates = common_candidates_,
                                .line_candidates = line_candidates,
                                .box_candidates = box_candidates,
                                .line_eliminations = line_eliminations,
                                .box_eliminations = box_eliminations //
                            };
                            return;
                        }
                    }
                }
            }

            void find_line_and_box_als()
            {
                const int line_remain_count = line_remain_.count(), box_remain_count = box_remain_.count();
                const auto line_remain_idx = bitmask_to_array<board_size - 2>(line_remain_);
                const auto box_remain_idx = bitmask_to_array<board_size - 2>(box_remain_);
                CandidateMask line_candidates = 0, box_candidates = 0;
                for (CandidateMask line_als = 0; (line_als = //
                         find_next_als(line_remain_idx.data(), line_als, 1 << line_remain_count, line_candidates));)
                    for (CandidateMask box_als = 0; (box_als = //
                             find_next_als(box_remain_idx.data(), box_als, 1 << box_remain_count, box_candidates));)
                    {
                        // No intersection candidate is allowed to appear in both the line ALS and the box ALS
                        if (line_candidates & box_candidates & common_candidates_)
                            continue;
                        // Set size constraint
                        if (std::popcount(static_cast<CandidateMask>(line_candidates & common_candidates_)) +
                                std::popcount(static_cast<CandidateMask>(box_candidates & common_candidates_)) !=
                            common_extras_ + 2)
                            continue;
                        // Now we have found a valid SdC, check if any candidates could be eliminated
                        const CandidateMask line_eliminated_candidates =
                            line_candidates | (common_candidates_ & ~box_candidates);
                        const CandidateMask box_eliminated_candidates =
                            box_candidates | (common_candidates_ & ~line_candidates);
                        const auto line_cells = pattern_from_indices_and_bits(line_remain_idx.data(), line_als);
                        const auto box_cells = pattern_from_indices_and_bits(box_remain_idx.data(), box_als);
                        const auto line_eliminations = find_eliminations( //
                            board_, line_remain_ & ~line_cells, line_eliminated_candidates);
                        const auto box_eliminations = find_eliminations( //
                            board_, box_remain_ & ~box_cells, box_eliminated_candidates);
                        if (line_eliminations.any() || box_eliminations.any())
                        {
                            res_ = SueDeCoq{
                                .intersection_cells = common_cells_,
                                .line_cells = line_cells,
                                .box_cells = box_cells,
                                .intersection_candidates = common_candidates_,
                                .line_candidates = line_candidates,
                                .box_candidates = box_candidates,
                                .line_eliminations = line_eliminations,
                                .box_eliminations = box_eliminations //
                            };
                            return;
                        }
                    }
            }

            CandidateMask find_next_als(const int* remain_idx, //
                const CandidateMask prev, const CandidateMask max, CandidateMask& als_candidates) const
            {
                for (CandidateMask mask = prev + 1; mask < max; mask++)
                {
                    als_candidates = 0;
                    for (const int i : set_bit_indices(mask))
                        als_candidates |= board_.cells[remain_idx[i]];
                    if (std::popcount(als_candidates) - std::popcount(mask) != 1) // Not an ALS
                        continue;
                    const CandidateMask overlap = als_candidates & common_candidates_;
                    // Overlap count must be not fewer than 2 (SdC definition)
                    // not greater than common_extras_ (or else the other ALS would have fewer than two common
                    // candidates with the intersection set, breaking the SdC)
                    if (const int overlap_count = std::popcount(overlap);
                        overlap_count > common_extras_ || overlap_count < 2)
                        continue;
                    return mask;
                }
                return 0;
            }
        };
    } // namespace

    std::string SueDeCoq::description() const
    {
        const auto info = get_intersection_info(intersection_cells);
        std::string res = fmt::format("Sue de Coq: intersection of {}{} and b{}, "
                                      "with {}={}, {}={}, {}={} =>", //
            info.is_row ? 'r' : 'c', info.line_idx + 1, info.box_idx + 1, //
            describe_cells(intersection_cells), describe_candidates(intersection_candidates), //
            describe_cells(line_cells), describe_candidates(line_candidates), //
            describe_cells(box_cells), describe_candidates(box_candidates));
        if (line_eliminations.any())
            res += fmt::format(" {}!={},", //
                describe_cells(line_eliminations), describe_candidates(line_eliminated_candidates()));
        if (box_eliminations.any())
            res += fmt::format(" {}!={},", //
                describe_cells(box_eliminations), describe_candidates(box_eliminated_candidates()));
        if (res.back() == ',')
            res.pop_back();
        return res;
    }

    std::optional<SueDeCoq> SueDeCoq::try_find(const Board& board, const bool extended)
    {
        return SueDeCoqFinder(board).try_find(extended);
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
