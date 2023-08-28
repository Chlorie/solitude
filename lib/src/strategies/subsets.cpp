#include "solitude/strategies/subsets.h"

#include <array>
#include <bit>
#include <fmt/format.h>

#include "common.h"

namespace sltd
{
    namespace
    {
        constexpr std::string_view tuple_names[]{
            {}, "single", "pair", "triple", "quadruple", //
            "quintuple", "sextuple", "septuple", "octuple" //
        };

        template <int Size>
        class NakedSubsetImpl
        {
        public:
            explicit NakedSubsetImpl(const Board& board): board_(board) {}

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
                        for (int j = 0; j < board_size; j++)
                        {
                            if ((1 << j) & idx_mask)
                                continue;
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
        std::optional<NakedSubset> naked_subset_impl(const Board& board)
        {
            return NakedSubsetImpl<Size>(board).try_find();
        }

        template <int Size>
        class HiddenSubsetImpl
        {
        public:
            explicit HiddenSubsetImpl(const Board& board): board_(board) {}

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
        std::optional<HiddenSubset> hidden_subset_impl(const Board& board)
        {
            return HiddenSubsetImpl<Size>(board).try_find();
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
        board.reduce_candidates_from_naked_single(cell_idx);
    }

    std::string NakedSubset::description() const
    {
        std::string res = fmt::format("Naked {}: in {}, ", tuple_names[std::popcount(idx_mask)], house_name(house_idx));
        const auto& house = house_indices[house_idx];
        for (int i = 0; i < board_size; i++)
            if ((1 << i) & idx_mask)
                res += fmt::format("{},", cell_name(house[i]));
        res.back() = '=';
        for (int i = 0; i < board_size; i++)
            if ((1 << i) & candidates)
                res += fmt::format("{},", i + 1);
        res.pop_back();
        return res;
    }

    std::optional<NakedSubset> NakedSubset::try_find(const Board& board, const int size)
    {
        static constexpr auto fptrs = []<std::size_t... I>(std::index_sequence<I...>)
        {
            return std::array<std::optional<NakedSubset> (*)(const Board&), sizeof...(I)>{
                naked_subset_impl<I + 2>... //
            };
        }(std::make_index_sequence<(board_size / 2 - 1)>{});
        if (size > board_size / 2)
            return std::nullopt;
        return fptrs[size - 2](board);
    }

    void NakedSubset::apply_to(Board& board) const
    {
        const auto& house = house_indices[house_idx];
        for (int i = 0; i < board_size; i++)
        {
            if ((1 << i) & idx_mask)
                continue;
            board.cells[house[i]] &= ~candidates;
        }
    }

    std::string HiddenSubset::description() const
    {
        std::string res =
            fmt::format("Hidden {}: in {}, ", tuple_names[std::popcount(idx_mask)], house_name(house_idx));
        const auto& house = house_indices[house_idx];
        for (int i = 0; i < board_size; i++)
            if ((1 << i) & idx_mask)
                res += fmt::format("{},", cell_name(house[i]));
        res.back() = '=';
        for (int i = 0; i < board_size; i++)
            if ((1 << i) & candidates)
                res += fmt::format("{},", i + 1);
        res.pop_back();
        return res;
    }

    std::optional<HiddenSubset> HiddenSubset::try_find(const Board& board, const int size)
    {
        static constexpr auto fptrs = []<std::size_t... I>(std::index_sequence<I...>)
        {
            return std::array<std::optional<HiddenSubset> (*)(const Board&), sizeof...(I)>{
                hidden_subset_impl<I + 1>... //
            };
        }(std::make_index_sequence<(board_size / 2)>{});
        if (size > board_size / 2)
            return std::nullopt;
        return fptrs[size - 1](board);
    }

    void HiddenSubset::apply_to(Board& board) const
    {
        const auto& house = house_indices[house_idx];
        for (int i = 0; i < board_size; i++)
            if ((1 << i) & idx_mask)
                board.cells[house[i]] &= candidates;
        if (std::has_single_bit(idx_mask)) // Hidden single
            board.reduce_candidates_from_naked_single(house[std::countr_zero(idx_mask)]);
    }
} // namespace sltd
