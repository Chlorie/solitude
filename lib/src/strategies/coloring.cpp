#include "solitude/strategies/coloring.h"

#include <fmt/format.h>

#include "common.h"

namespace sltd
{
    namespace
    {
        PatternMask peers_to_any_of(const PatternMask& pattern)
        {
            PatternMask res;
            for (const int i : pattern.set_bit_indices())
                res |= peer_masks[i];
            return res;
        }

        int get_first_set_bit_index(const PatternMask& pattern)
        {
            for (const int i : pattern.set_bit_indices())
                return i;
            return cell_count;
        }
    } // namespace

    std::string RemotePair::description() const
    {
        return fmt::format("Remote Pair: 1st group {}, 2nd group {}, [{}!={}]", //
            describe_cells(groups[0]), describe_cells(groups[1]), //
            describe_cells(eliminations), describe_candidates(candidates));
    }

    std::optional<RemotePair> RemotePair::try_find(const Board& board)
    {
        const auto patterns = board.all_number_patterns();
        for (auto uncolored = nvalue_cells(board, 2); //
             const int initiating : uncolored.set_bit_indices())
        {
            const auto [x, y] = candidate_mask_to_array<2>(board.cells[initiating]);
            auto uncolored_same_bivalues = uncolored & patterns[x] & patterns[y];
            PatternMask colors[2], prev;
            prev.set(initiating);
            int color = 0;
            while (prev.any())
            {
                colors[color] |= prev;
                uncolored_same_bivalues &= ~prev;
                color = 1 - color; // Switch to another color
                PatternMask current;
                for (const int j : prev.set_bit_indices())
                    current |= uncolored_same_bivalues & peer_masks[j];
                prev = current;
            }

            // Check common peer of the two groups
            const PatternMask common_peers = peers_to_any_of(colors[0]) & peers_to_any_of(colors[1]);
            if (const auto eliminations = common_peers & (patterns[x] | patterns[y]); //
                eliminations.any())
                return RemotePair{
                    .groups = {colors[0], colors[1]},
                    .eliminations = eliminations,
                    .candidates = board.cells[initiating] //
                };

            uncolored &= ~(colors[0] | colors[1]);
        }
        return std::nullopt;
    }

    void RemotePair::apply_to(Board& board) const
    {
        for (const int i : eliminations.set_bit_indices())
            board.cells[i] &= ~candidates;
    }

    SimpleColorType SimpleColors::type() const noexcept
    {
        using enum SimpleColorType;
        return eliminating_pair[0] == eliminating_pair[1] ? trap : wrap;
    }

    std::string SimpleColors::description() const
    {
        if (type() == SimpleColorType::trap)
            return fmt::format("Simple Color Trap: number {}, 1st color {}, 2nd color {}, [{}!={}]", //
                describe_candidates(candidate), describe_cells(colors[0]), describe_cells(colors[1]), //
                describe_cells(eliminations), describe_candidates(candidate));
        else
            return fmt::format("Simple Color Wrap: number {}, remaining color {}, eliminated [{}!={}] due to {}-{}", //
                describe_candidates(candidate), describe_cells(colors[0]), //
                describe_cells(eliminations), describe_candidates(candidate), //
                cell_name(eliminating_pair[0]), cell_name(eliminating_pair[1]));
    }

    std::optional<SimpleColors> SimpleColors::try_find(const Board& board)
    {
        for (int number = 0; number < board_size; number++)
        {
            const auto pattern = board.pattern_of(number);
            if (pattern.none())
                continue;
            for (PatternMask uncolored = pattern; //
                 const int initiating : uncolored.set_bit_indices())
            {
                PatternMask colors[2], prev;
                prev.set(initiating);
                int color = 0;
                while (prev.any())
                {
                    colors[color] |= prev;
                    uncolored &= ~prev;
                    color = 1 - color; // Switch to another color
                    PatternMask current;
                    for (const int j : prev.set_bit_indices())
                        for (const auto uncolored_peers = uncolored & peer_masks[j];
                             const int k : uncolored_peers.set_bit_indices())
                            // Strong link, common peers don't contain target number
                            if ((peer_masks[j] & peer_masks[k] & pattern).none())
                                current.set(k);
                    prev = current;
                }
                const PatternMask color_peers[]{peers_to_any_of(colors[0]), peers_to_any_of(colors[1])};

                // Check simple color wrap
                for (int i = 0; i < 2; i++)
                    if (const auto weak_links = color_peers[i] & colors[i];
                        weak_links.any()) // This color sees itself (weakly linked)
                    {
                        const int first_link = get_first_set_bit_index(weak_links);
                        const int second_link = get_first_set_bit_index(peer_masks[first_link] & weak_links);
                        return SimpleColors{
                            .colors = {colors[1 - i], colors[i]},
                            .eliminations = colors[i],
                            .eliminating_pair = {first_link, second_link},
                            .candidate = static_cast<CandidateMask>(1 << number) //
                        };
                    }

                // Check simple color trap
                if (const auto eliminations = color_peers[0] & color_peers[1] & pattern; //
                    eliminations.any())
                    return SimpleColors{
                        .colors = {colors[0], colors[1]},
                        .eliminations = eliminations,
                        .candidate = static_cast<CandidateMask>(1 << number) //
                    };
            }
        }
        return std::nullopt;
    }

    void SimpleColors::apply_to(Board& board) const
    {
        for (const int i : eliminations.set_bit_indices())
            board.cells[i] &= ~candidate;
    }
} // namespace sltd
