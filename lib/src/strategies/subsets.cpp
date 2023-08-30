#include "solitude/strategies/subsets.h"

#include <array>
#include <bit>
#include <fmt/format.h>

#include "common.h"
#include "solitude/utils.h"

namespace sltd
{
    namespace
    {
        constexpr std::string_view tuple_names[]{
            {}, "Single", "Pair", "Triple", "Quadruple", //
            "Quintuple", "Sextuple", "Septuple", "Octuple" //
        };

        template <int Size>
        class NakedSubsetFinder
        {
        public:
            explicit NakedSubsetFinder(const Board& board): board_(board) {}

            std::optional<NakedSubset> try_find()
            {
                for (; house_idx_ < house_indices.size(); house_idx_++)
                {
                    next_set_element<0>();
                    if (res_)
                        return res_;
                }
                return std::nullopt;
            }

        private:
            Board board_;
            int house_idx_ = 0;
            int indices_[Size]{};
            CandidateMask set_candidates_[Size]{};
            std::optional<NakedSubset> res_;

            template <int Layer>
            void next_set_element()
            {
                for (int& i = (indices_[Layer] = (Layer == 0 ? 0 : indices_[Layer - 1] + 1)); i < board_size; ++i)
                {
                    const int cell_idx = house_indices[house_idx_][i];
                    if (board_.filled[cell_idx]) // Current cell is filled, skip it
                        continue;
                    // set_candidates_ for layer 0 is always all zeros
                    const CandidateMask candidates = board_.cells[cell_idx] | set_candidates_[Layer];
                    if (std::popcount(candidates) > Size) // #candidates > #elements, skip it
                        continue;
                    if constexpr (Layer == Size - 1) // Last layer
                    {
                        CandidateMask idx_mask = 0;
                        for (const int j : indices_)
                            idx_mask |= 1 << j;
                        for (const int j : set_bit_indices<CandidateMask>(~idx_mask & full_mask))
                            // Can remove candidates, found the naked set
                            if (board_.cells[house_indices[house_idx_][j]] & candidates)
                            {
                                res_ = NakedSubset{
                                    .house_idx = house_idx_,
                                    .idx_mask = idx_mask,
                                    .candidates = candidates //
                                };
                                return;
                            }
                    }
                    else
                    {
                        set_candidates_[Layer + 1] = candidates;
                        next_set_element<Layer + 1>();
                        if (res_)
                            return;
                    }
                }
            }
        };

        template <int Size>
        std::optional<NakedSubset> find_naked_subset(const Board& board)
        {
            return NakedSubsetFinder<Size>(board).try_find();
        }

        template <int Size>
        class HiddenSubsetFinder
        {
        public:
            explicit HiddenSubsetFinder(const Board& board): board_(board) {}

            std::optional<HiddenSubset> try_find()
            {
                for (; house_idx_ < house_indices.size(); house_idx_++)
                {
                    next_set_element<0>();
                    if (res_)
                        return res_;
                }
                return std::nullopt;
            }

        private:
            Board board_;
            int house_idx_ = 0;
            int indices_[Size]{};
            std::optional<HiddenSubset> res_;

            template <int Layer>
            void next_set_element()
            {
                for (int& i = (indices_[Layer] = (Layer == 0 ? 0 : indices_[Layer - 1] + 1)); i < board_size; ++i)
                {
                    const auto& house = house_indices[house_idx_];
                    if (board_.filled[house[i]]) // Current cell is filled, skip it
                        continue;
                    if constexpr (Layer == Size - 1) // Last layer
                    {
                        CandidateMask idx_mask = 0;
                        for (const int j : indices_)
                            idx_mask |= 1 << j;
                        CandidateMask in_candidates = 0, out_candidates = 0;
                        for (int j = 0; j < board_size; j++)
                            if ((1 << j) & idx_mask) // In subset
                                in_candidates |= board_.cells[house[j]];
                            else
                                out_candidates |= board_.cells[house[j]];
                        if (std::popcount(out_candidates) == board_size - Size && // Complement naked subset
                            (in_candidates & out_candidates)) // Can remove candidates
                        {
                            res_ = HiddenSubset{
                                .house_idx = house_idx_,
                                .idx_mask = idx_mask,
                                .candidates = static_cast<CandidateMask>(full_mask & ~out_candidates) //
                            };
                            return;
                        }
                    }
                    else
                    {
                        next_set_element<Layer + 1>();
                        if (res_)
                            return;
                    }
                }
            }
        };

        template <int Size>
        std::optional<HiddenSubset> find_hidden_subset(const Board& board)
        {
            return HiddenSubsetFinder<Size>(board).try_find();
        }
    } // namespace

    std::string NakedSingle::description() const
    {
        return fmt::format("Naked single: {}={}", //
            cell_name(cell_idx), std::countr_zero(candidate) + 1);
    }

    std::optional<NakedSingle> NakedSingle::try_find(const Board& board)
    {
        for (int i = 0; i < cell_count; i++)
            if (!board.filled[i] && std::has_single_bit(board.cells[i]))
                return NakedSingle{.cell_idx = i, .candidate = board.cells[i]};
        return std::nullopt;
    }

    void NakedSingle::apply_to(Board& board) const
    {
        board.cells[cell_idx] = candidate;
        board.filled.set(cell_idx);
        board.eliminate_candidates_from_naked_single(cell_idx);
    }

    std::string NakedSubset::description() const
    {
        return fmt::format("Naked {}: in {}, {}={}", //
            tuple_names[std::popcount(idx_mask)], house_name(house_idx), //
            describe_cells_in_house(house_idx, idx_mask), //
            describe_candidates(candidates));
    }

    std::optional<NakedSubset> NakedSubset::try_find(const Board& board, const int size)
    {
        static constexpr auto fptrs = []<std::size_t... I>(std::index_sequence<I...>)
        {
            return std::array<std::optional<NakedSubset> (*)(const Board&), sizeof...(I)>{
                find_naked_subset<I + 2>... //
            };
        }(std::make_index_sequence<(board_size / 2 - 1)>{});
        if (size > board_size / 2 || size <= 1)
            return std::nullopt;
        return fptrs[size - 2](board);
    }

    void NakedSubset::apply_to(Board& board) const
    {
        const auto& house = house_indices[house_idx];
        for (const int i : set_bit_indices<CandidateMask>(~idx_mask & full_mask))
            board.cells[house[i]] &= ~candidates;
    }

    std::string HiddenSubset::description() const
    {
        return fmt::format("Hidden {}: in {}, {}={}", //
            tuple_names[std::popcount(idx_mask)], house_name(house_idx), //
            describe_cells_in_house(house_idx, idx_mask), //
            describe_candidates(candidates));
    }

    std::optional<HiddenSubset> HiddenSubset::try_find(const Board& board, const int size)
    {
        static constexpr auto fptrs = []<std::size_t... I>(std::index_sequence<I...>)
        {
            return std::array<std::optional<HiddenSubset> (*)(const Board&), sizeof...(I)>{
                find_hidden_subset<I + 1>... //
            };
        }(std::make_index_sequence<(board_size / 2)>{});
        if (size > board_size / 2 || size < 1)
            return std::nullopt;
        return fptrs[size - 1](board);
    }

    void HiddenSubset::apply_to(Board& board) const
    {
        const auto& house = house_indices[house_idx];
        for (const int i : set_bit_indices(idx_mask))
            board.cells[house[i]] &= candidates;
        if (std::has_single_bit(idx_mask)) // Hidden single
        {
            const int cell_idx = house[std::countr_zero(idx_mask)];
            board.eliminate_candidates_from_naked_single(cell_idx);
            board.filled.set(cell_idx);
        }
    }
} // namespace sltd
