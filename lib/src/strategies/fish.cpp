#include "solitude/strategies/fish.h"

#include <fmt/format.h>

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
                        CandidateMask base_sets = 0;
                        for (const int j : base_idx_)
                            base_sets |= 1 << j;

                        // Check if there is any elimination
                        bool has_elimination = false;
                        for (int j = 0; j < board_size; j++)
                            if (((1 << j) & cover_sets) && (cover_patterns[j] & ~base_sets))
                            {
                                has_elimination = true;
                                break;
                            }
                        if (!has_elimination)
                            continue;

                        // Find all eliminations
                        PatternMask eliminations;
                        for (int j = 0; j < board_size; j++)
                        {
                            if (!((1 << j) & cover_sets))
                                continue;
                            // In cover set but not in base set, eliminated
                            const CandidateMask cover_pattern = cover_patterns[j] & ~base_sets;
                            for (int k = 0; k < board_size; k++)
                                if ((1 << k) & cover_pattern)
                                    eliminations.set(row_based ? k * board_size + j : j * board_size + k);
                        }

                        res_ = Fish{
                            .base_sets = static_cast<HouseMask>(base_sets << (row_based ? 0 : board_size)),
                            .cover_sets = static_cast<HouseMask>(cover_sets << (row_based ? board_size : 0)),
                            .candidate = static_cast<CandidateMask>(1 << number_),
                            .eliminations = eliminations //
                        };
                        return;
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
    } // namespace

    std::string Fish::description() const
    {
        return fmt::format("{}: Number {} in {}->{}, {}!={}", //
            get_fish_name(std::popcount(base_sets)), //
            describe_candidates(candidate), //
            describe_houses(base_sets), describe_houses(cover_sets), //
            describe_cells(eliminations), describe_candidates(candidate));
    }

    std::optional<Fish> Fish::try_find(const Board& board, const int size)
    {
        // TODO: currently only vanilla fish are found
        // Vanilla fish size [2, board_size / 2]
        // Mutant fish size [2, board_size * 3 / 2]
        static constexpr auto fptrs = []<std::size_t... I>(std::index_sequence<I...>)
        {
            return std::array<std::optional<Fish> (*)(const Board&), sizeof...(I)>{
                find_vanilla_fish<I + 2>... //
            };
        }(std::make_index_sequence<(board_size / 2 - 1)>{});
        if (size > board_size / 2)
            return std::nullopt;
        return fptrs[size - 2](board);
    }

    void Fish::apply_to(Board& board) const
    {
        for (int i = 0; i < cell_count; i++)
            if (eliminations[i])
                board.cells[i] &= ~candidate;
    }
} // namespace sltd
