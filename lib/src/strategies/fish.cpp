#include "solitude/strategies/fish.h"

#include <fmt/format.h>

#include "solitude/utils.h"
#include "common.h"

namespace sltd
{
    namespace
    {
        std::string get_fish_name(const int size)
        {
            switch (size)
            {
                case 2: return "X-wing";
                case 3: return "Swordfish";
                case 4: return "Jellyfish";
                case 5: return "Squirmbag";
                case 6: return "Whale";
                case 7: return "Leviathan";
                default: return fmt::format("{}-fish", size);
            }
        }

        // TODO: try another approach and see which is better
        // The current implementation is house-based, i.e. we operate upon
        // an array of board_size bitboards, each with board_size bits.
        // It might be beneficial to use a board-based approach, i.e. we
        // directly manipulate a bitboard of size cell_count.

        template <int Size>
        CandidateMask indices_to_mask(const int (&indices)[Size])
        {
            CandidateMask res = 0;
            for (const int i : indices)
                res |= 1 << i;
            return res;
        }

        void add_eliminations(PatternMask& eliminations, //
            const bool row_based, const int cover_house_idx, const CandidateMask cover_pattern)
        {
            for (const int i : set_bit_indices(cover_pattern))
                eliminations.set(row_based //
                        ? i * board_size + cover_house_idx
                        : cover_house_idx * board_size + i);
        }

        template <int Size>
        class VanillaFishFinder
        {
        public:
            explicit VanillaFishFinder(const Board& board): board_(board) {}

            std::optional<Fish> try_find()
            {
                for (; number_ < board_size; number_++)
                {
                    row_patterns_ = extract_row_patterns(board_, number_);
                    col_patterns_ = extract_column_patterns(board_, number_);
                    next_house<0>(true);
                    if (res_)
                        return res_;
                    next_house<0>(false);
                    if (res_)
                        return res_;
                }
                return std::nullopt;
            }

        private:
            Board board_;
            int number_ = 0;
            std::array<CandidateMask, board_size> row_patterns_{};
            std::array<CandidateMask, board_size> col_patterns_{};
            int base_idx_[Size]{};
            CandidateMask cover_idx_masks_[Size]{};
            std::optional<Fish> res_;

            template <int Layer>
            void next_house(const bool row_based)
            {
                const auto& base_patterns = row_based ? row_patterns_ : col_patterns_;
                const auto& cover_patterns = row_based ? col_patterns_ : row_patterns_;
                for (int& i = (base_idx_[Layer] = (Layer == 0 ? 0 : base_idx_[Layer - 1] + 1)); i < board_size; ++i)
                {
                    const auto base_pattern = base_patterns[i];
                    // It's a hidden single no matter how you look at it. Thank you very much.
                    if (std::popcount(base_pattern) == 1)
                        continue;
                    const CandidateMask cover_sets = cover_idx_masks_[Layer] | base_pattern;
                    // Need more than Size cover sets
                    if (std::popcount(cover_sets) > Size)
                        continue;
                    if constexpr (Layer == Size - 1) // Last layer
                    {
                        // Find eliminations
                        const CandidateMask base_sets = sltd::indices_to_mask(base_idx_);
                        PatternMask eliminations;
                        for (const int j : set_bit_indices(cover_sets))
                            add_eliminations(eliminations, row_based, j, cover_patterns[j] & ~base_sets);

                        if (eliminations.any())
                        {
                            res_ = Fish{
                                .base_sets = static_cast<HouseMask>(base_sets) << (row_based ? 0 : board_size),
                                .cover_sets = static_cast<HouseMask>(cover_sets) << (row_based ? board_size : 0),
                                .candidate = static_cast<CandidateMask>(1 << number_),
                                .eliminations = eliminations //
                            };
                            return;
                        }
                    }
                    else
                    {
                        cover_idx_masks_[Layer + 1] = cover_sets;
                        next_house<Layer + 1>(row_based);
                        if (res_)
                            return;
                    }
                }
            }
        };

        template <int Size>
        std::optional<Fish> find_vanilla_fish(const Board& board)
        {
            return VanillaFishFinder<Size>(board).try_find();
        }

        template <int Size>
        class FinnedFishFinder
        {
        public:
            explicit FinnedFishFinder(const Board& board): board_(board) {}

            std::optional<Fish> try_find()
            {
                for (; number_ < board_size; number_++)
                {
                    row_patterns_ = extract_row_patterns(board_, number_);
                    col_patterns_ = extract_column_patterns(board_, number_);
                    next_house<0>(true);
                    if (res_)
                        return res_;
                    next_house<0>(false);
                    if (res_)
                        return res_;
                }
                return std::nullopt;
            }

        private:
            Board board_;
            int number_ = 0;
            std::array<CandidateMask, board_size> row_patterns_{};
            std::array<CandidateMask, board_size> col_patterns_{};
            int base_idx_[Size]{};
            std::optional<Fish> res_;

            template <int Layer>
            void next_house(const bool row_based)
            {
                const auto& base_patterns = row_based ? row_patterns_ : col_patterns_;
                for (int& i = (base_idx_[Layer] = (Layer == 0 ? 0 : base_idx_[Layer - 1] + 1)); i < board_size; ++i)
                {
                    // It's a hidden single no matter how you look at it. Thank you very much.
                    if (std::popcount(base_patterns[i]) == 1)
                        continue;
                    if constexpr (Layer == Size - 1) // Last layer
                        enumerate_fin_box(row_based);
                    else
                        next_house<Layer + 1>(row_based);
                    if (res_)
                        return;
                }
            }

            // For a non-mutant fish, all of its fins must be able to be covered by a box,
            // or else there would be no valid candidate eliminations.
            void enumerate_fin_box(const bool row_based)
            {
                const auto& base_patterns = row_based ? row_patterns_ : col_patterns_;
                const auto& cover_patterns = row_based ? col_patterns_ : row_patterns_;
                for (int box = 0; box < board_size; box++)
                {
                    // First, treat every base candidate in the box as a "fin"
                    auto base_patterns_no_fins = base_patterns;
                    const int row_offset = box / box_height * box_height, col_offset = box % box_height * box_width;
                    if (row_based)
                    {
                        const CandidateMask mask = ~(row_box_intersection << col_offset);
                        for (int i = row_offset; i < row_offset + box_height; i++)
                            base_patterns_no_fins[i] &= mask;
                    }
                    else
                    {
                        const CandidateMask mask = ~(column_box_intersection << row_offset);
                        for (int i = col_offset; i < col_offset + box_width; i++)
                            base_patterns_no_fins[i] &= mask;
                    }

                    // Next, check whether what is left is a valid fish
                    CandidateMask cover_sets = 0;
                    for (const int i : base_idx_)
                        cover_sets |= base_patterns_no_fins[i];
                    if (std::popcount(cover_sets) != Size) // We need Size cover sets
                        continue;
                    const CandidateMask cover_sets_intersecting_with_fin_box = cover_sets &
                        (row_based ? (row_box_intersection << col_offset) : (column_box_intersection << row_offset));
                    if (!cover_sets_intersecting_with_fin_box)
                        continue;

                    // Then, find all the eliminations
                    const CandidateMask base_sets = sltd::indices_to_mask(base_idx_);
                    PatternMask eliminations;
                    const CandidateMask intersection_mask = row_based //
                        ? (column_box_intersection << row_offset)
                        : (row_box_intersection << col_offset);
                    for (const int i : set_bit_indices(cover_sets_intersecting_with_fin_box))
                        add_eliminations(eliminations, row_based, i, //
                            cover_patterns[i] & ~base_sets & intersection_mask);

                    // We've found some eliminations, save the results
                    if (eliminations.any())
                    {
                        // We need to find all the fins
                        PatternMask fins;
                        for (const int base : base_idx_)
                            for (const CandidateMask fin_mask = base_patterns[base] & ~cover_sets;
                                 const int i : set_bit_indices(fin_mask))
                                fins.set(row_based ? base * board_size + i : i * board_size + base);

                        res_ = Fish{
                            .base_sets = static_cast<HouseMask>(base_sets) << (row_based ? 0 : board_size),
                            .cover_sets = static_cast<HouseMask>(cover_sets) << (row_based ? board_size : 0),
                            .candidate = static_cast<CandidateMask>(1 << number_),
                            .fins = fins,
                            .eliminations = eliminations //
                        };
                        return;
                    }
                }
            }
        };

        template <int Size>
        std::optional<Fish> find_finned_fish(const Board& board)
        {
            return FinnedFishFinder<Size>(board).try_find();
        }
    } // namespace

    std::string Fish::description() const
    {
        return fmt::format("{}{}: number {} in {}->{},{} [{}!={}]", //
            fins.any() ? "Finned " : "", get_fish_name(std::popcount(base_sets)), //
            describe_candidates(candidate), //
            describe_houses(base_sets), describe_houses(cover_sets), //
            fins.any() ? fmt::format(" fin{} {},", fins.count() == 1 ? "" : "s", describe_cells(fins)) : "", //
            describe_cells(eliminations), describe_candidates(candidate));
    }

    std::optional<Fish> Fish::try_find(const Board& board, const int size, const bool finned)
    {
        // TODO: currently only non-mutant fish are found
        // Non-mutant fish size [2, board_size / 2]
        // Mutant fish size [2, board_size * 3 / 2]
        static constexpr auto vanilla_fptrs = []<std::size_t... I>(std::index_sequence<I...>)
        {
            return std::array<std::optional<Fish> (*)(const Board&), sizeof...(I)>{
                find_vanilla_fish<I + 2>... //
            };
        }(std::make_index_sequence<(board_size / 2 - 1)>{});
        static constexpr auto finned_fptrs = []<std::size_t... I>(std::index_sequence<I...>)
        {
            return std::array<std::optional<Fish> (*)(const Board&), sizeof...(I)>{
                find_finned_fish<I + 2>... //
            };
        }(std::make_index_sequence<(board_size / 2 - 1)>{});
        if (size > board_size / 2 || size <= 1)
            return std::nullopt;
        return (finned ? finned_fptrs : vanilla_fptrs)[size - 2](board);
    }

    void Fish::apply_to(Board& board) const
    {
        for (const int i : eliminations.set_bit_indices())
            board.cells[i] &= ~candidate;
    }
} // namespace sltd
