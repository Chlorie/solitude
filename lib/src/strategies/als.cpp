#include "solitude/strategies/als.h"

#include <vector>
#include <algorithm>
#include <clu/static_vector.h>
#include <fmt/format.h>

#include "common.h"

namespace sltd
{
    namespace
    {
        struct Als
        {
            PatternMask cells;
            CandidateMask candidates = 0;
            PatternMask common_peers[board_size];
        };

        auto find_unfilled_cells_in_house(const Board& board, const int house)
        {
            clu::static_vector<int, board_size> res;
            for (const int i : house_indices[house])
                if (!board.filled[i])
                    res.push_back(i);
            return res;
        }

        PatternMask find_common_peers_in_pattern(const PatternMask& pattern)
        {
            static constexpr auto full_pattern = PatternMask{}.set();
            auto res = full_pattern;
            for (const int i : pattern.set_bit_indices())
                res &= peer_masks[i];
            return res;
        }

        std::vector<Als> find_all_als(const Board& board)
        {
            const auto patterns = board.all_number_patterns();
            std::vector<Als> res;
            std::vector<CandidateMask> candidate_cache;
            candidate_cache.resize(1 << board_size);
            for (int house = 0; house < house_indices.size(); house++)
            {
                candidate_cache[0] = 0;
                const auto unfilled = find_unfilled_cells_in_house(board, house);
                for (int i = 0; i < unfilled.size(); i++)
                {
                    const CandidateMask msb = 1 << i;
                    const CandidateMask msb_candidates = board.cells[unfilled[i]];
                    for (CandidateMask j = msb; j < msb << 1; j++)
                    {
                        const CandidateMask candidates = msb_candidates | candidate_cache[j - msb];
                        candidate_cache[j] = candidates;
                        if (std::popcount(candidates) - std::popcount(j) == 1) // Found an ALS
                        {
                            Als& als = res.emplace_back();
                            als.cells = pattern_from_indices_and_bits(unfilled.data(), j);
                            als.candidates = candidates;
                            for (int k = 0; k < board_size; k++)
                                als.common_peers[k] = find_common_peers_in_pattern(als.cells & patterns[k]);
                        }
                    }
                }
            }
            std::ranges::sort(res, std::less{}, [](const Als& als) { return std::popcount(als.candidates); });
            return res;
        }

        CandidateMask find_candidates_in_pattern(const Board& board, const PatternMask& pattern)
        {
            if (pattern.none())
                return 0;
            CandidateMask res = 0;
            for (const int i : pattern.set_bit_indices())
                res |= board.cells[i];
            return res;
        }

        std::pair<int, int> find_rccs( //
            const Board& board, const std::array<PatternMask, board_size>& patterns, //
            const Als& als1, const Als& als2)
        {
            std::pair rccs{board_size, board_size};
            const PatternMask overlap = als1.cells & als2.cells;
            const CandidateMask overlapping_candidates = find_candidates_in_pattern(board, overlap);
            const CandidateMask rcc_candidates = als1.candidates & als2.candidates & ~overlapping_candidates;
            if (!rcc_candidates) // The overlapping area may not contain an RCC
                return rccs;
            for (const int rcc : set_bit_indices(rcc_candidates))
                // All the RCC cells must see each other
                if (als1.common_peers[rcc].contains(als2.cells & patterns[rcc]) && //
                    als2.common_peers[rcc].contains(als1.cells & patterns[rcc]))
                {
                    if (rccs.first == board_size) // The first RCC
                        rccs.first = rcc;
                    else // The second RCC, no need to search more
                    {
                        rccs.second = rcc;
                        return rccs;
                    }
                }
            return rccs;
        }
    } // namespace

    std::string AlsXZ::description() const
    {
        std::string res = fmt::format("ALS-XZ: {}={}, {}={}, RCC={} =>", //
            describe_cells(als[0]), describe_candidates(candidates[0]), //
            describe_cells(als[1]), describe_candidates(candidates[1]), //
            describe_candidates(rcc));
        for (int i = 0; i < board_size; i++)
            if (eliminations[i].any())
                res += fmt::format(" {}!={},", describe_cells(eliminations[i]), i + 1);
        res.pop_back();
        return res;
    }

    std::optional<AlsXZ> AlsXZ::try_find(const Board& board)
    {
        const auto patterns = board.all_number_patterns();
        const auto als = find_all_als(board);
        for (std::size_t i = 0; i < als.size(); i++)
            for (std::size_t j = i + 1; j < als.size(); j++)
            {
                const CandidateMask common = als[i].candidates & als[j].candidates;
                if (std::popcount(common) < 2) // We need at least one for RCC and one for elimination
                    continue;
                const auto [rcc1, rcc2] = find_rccs(board, patterns, als[i], als[j]);
                if (rcc1 == board_size) // No RCC
                    continue;
                const CandidateMask eliminated_candidates = rcc2 == board_size ? common ^ (1 << rcc1) : common;
                const CandidateMask rccs = rcc2 == board_size ? 1 << rcc1 : (1 << rcc1) | (1 << rcc2);
                AlsXZ res{
                    .als = {als[i].cells, als[j].cells},
                    .candidates = {als[i].candidates, als[j].candidates},
                    .rcc = rccs //
                };
                PatternMask all_eliminations;
                for (const int ec : set_bit_indices(eliminated_candidates))
                {
                    const auto ec_peers = als[i].common_peers[ec] & als[j].common_peers[ec];
                    all_eliminations |= (res.eliminations[ec] = ec_peers & patterns[ec]);
                }
                if (rcc2 != board_size) // Double RCC
                {
                    for (const CandidateMask rcc1_extras = als[i].candidates & ~rccs;
                         const int ec : set_bit_indices(rcc1_extras))
                    {
                        const auto ec_peers = als[i].common_peers[ec] & patterns[ec];
                        res.eliminations[ec] |= ec_peers;
                        all_eliminations |= ec_peers;
                    }
                    for (const CandidateMask rcc2_extras = als[j].candidates & ~rccs;
                         const int ec : set_bit_indices(rcc2_extras))
                    {
                        const auto ec_peers = als[j].common_peers[ec] & patterns[ec];
                        res.eliminations[ec] |= ec_peers;
                        all_eliminations |= ec_peers;
                    }
                }
                if (all_eliminations.any())
                    return res;
            }
        return std::nullopt;
    }

    void AlsXZ::apply_to(Board& board) const
    {
        for (int i = 0; i < board_size; i++)
            for (const int j : eliminations[i].set_bit_indices())
                board.cells[j] &= ~(1 << i);
    }

    std::string AlsXYWing::description() const
    {
        std::string res = fmt::format("ALS-XY-Wing: pivot {}={}, pincers {}={}, {}={}, X,Y={},{}, Z={} =>", //
            describe_cells(pivot), describe_candidates(pivot_candidates), //
            describe_cells(pincers[0]), describe_candidates(pincer_candidates[0]), //
            describe_cells(pincers[1]), describe_candidates(pincer_candidates[1]), //
            describe_candidates(x), describe_candidates(y), //
            describe_candidates((pincer_candidates[0] & ~x) & (pincer_candidates[1] & ~y)));
        for (int i = 0; i < board_size; i++)
            if (eliminations[i].any())
                res += fmt::format(" {}!={},", describe_cells(eliminations[i]), i + 1);
        res.pop_back();
        return res;
    }

    std::optional<AlsXYWing> AlsXYWing::try_find(const Board& board)
    {
        const auto patterns = board.all_number_patterns();
        const auto als = find_all_als(board);
        for (std::size_t i = 0; i < als.size(); i++) // Pincer 1
            for (std::size_t j = i + 1; j < als.size(); j++) // Pivot
            {
                const auto [x, extra1] = find_rccs(board, patterns, als[i], als[j]);
                // If the two ALSs share 2 RCCs you should just use an ALS-XZ move instead
                if (x == board_size || extra1 != board_size)
                    continue;
                const CandidateMask z1_mask = als[i].candidates ^ (1 << x);
                for (std::size_t k = i + 1; k < als.size(); k++) // Pincer 2
                {
                    if (!(als[k].candidates & z1_mask)) // Pincer 2 doesn't contain a possible Z
                        continue;
                    const auto [y, extra2] = find_rccs(board, patterns, als[j], als[k]);
                    if (y == board_size || x == y || extra2 != board_size)
                        continue;
                    const CandidateMask z_mask = z1_mask & (als[k].candidates ^ (1 << y));
                    if (!z_mask)
                        continue;
                    AlsXYWing res{
                        .pivot = als[j].cells,
                        .pincers = {als[i].cells, als[k].cells},
                        .pivot_candidates = als[j].candidates,
                        .pincer_candidates = {als[i].candidates, als[k].candidates},
                        .x = static_cast<CandidateMask>(1 << x),
                        .y = static_cast<CandidateMask>(1 << y) //
                    };
                    PatternMask all_eliminations;
                    for (const int z : set_bit_indices(z_mask))
                    {
                        const auto ec_peers = als[i].common_peers[z] & als[k].common_peers[z];
                        all_eliminations |= (res.eliminations[z] = ec_peers & patterns[z]);
                    }
                    if (all_eliminations.any())
                        return res;
                }
            }
        return std::nullopt;
    }

    void AlsXYWing::apply_to(Board& board) const
    {
        for (int i = 0; i < board_size; i++)
            for (const int j : eliminations[i].set_bit_indices())
                board.cells[j] &= ~(1 << i);
    }
} // namespace sltd
