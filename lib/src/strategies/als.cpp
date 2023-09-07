#include "solitude/strategies/als.h"

#include <vector>
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
        };

        auto find_unfilled_cells_in_house(const Board& board, const int house)
        {
            clu::static_vector<int, board_size> res;
            for (const int i : house_indices[house])
                if (!board.filled[i])
                    res.push_back(i);
            return res;
        }

        std::vector<Als> find_all_als(const Board& board)
        {
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
                            res.push_back({
                                .cells = pattern_from_indices_and_bits(unfilled.data(), j),
                                .candidates = candidates //
                            });
                    }
                }
            }
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

        PatternMask find_common_peers_in_pattern(const PatternMask& pattern)
        {
            PatternMask res = PatternMask{}.flip();
            for (const int i : pattern.set_bit_indices())
                res &= peer_masks[i];
            return res;
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
                // We need at least two common candidates between the two ALSs:
                // One for the RCC, another one for the eliminations
                const CandidateMask common = als[i].candidates & als[j].candidates;
                if (std::popcount(common) < 2)
                    continue;
                const PatternMask overlap = als[i].cells & als[j].cells;
                const CandidateMask overlapping_candidates = find_candidates_in_pattern(board, overlap);
                const CandidateMask rcc_candidates = common & ~overlapping_candidates;
                if (!rcc_candidates) // The overlapping area may not contain an RCC
                    continue;
                for (const int rcc : set_bit_indices(rcc_candidates))
                {
                    const auto first_rcc = als[i].cells & patterns[rcc];
                    const auto second_rcc = als[j].cells & patterns[rcc];
                    const auto first_peers = find_common_peers_in_pattern(first_rcc);
                    const auto second_peers = find_common_peers_in_pattern(second_rcc);
                    // All the RCC cells must see each other
                    if (!first_peers.contains(second_rcc) || !second_peers.contains(first_rcc))
                        continue;
                    // Find all eliminations
                    const CandidateMask eliminated_candidates = common ^ (1 << rcc);
                    AlsXZ res{
                        .als = {als[i].cells, als[j].cells},
                        .candidates = {als[i].candidates, als[j].candidates},
                        .rcc = static_cast<CandidateMask>(1 << rcc) //
                    };
                    PatternMask all_eliminations;
                    for (const int ec : set_bit_indices(eliminated_candidates))
                    {
                        const auto all_ec = (als[i].cells | als[j].cells) & patterns[ec];
                        all_eliminations |=
                            (res.eliminations[ec] = find_common_peers_in_pattern(all_ec) & patterns[ec]);
                    }
                    if (all_eliminations.any())
                        return res;
                }
            }
        return std::nullopt;
    }

    void AlsXZ::apply_to(Board& board) const
    {
        for (int i = 0; i < board_size; i++)
        {
            if (eliminations[i].none())
                continue;
            for (const int j : eliminations[i].set_bit_indices())
                board.cells[j] &= ~(1 << i);
        }
    }
} // namespace sltd
