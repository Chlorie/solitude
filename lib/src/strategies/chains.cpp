#include "solitude/strategies/chains.h"

#include <fmt/format.h>
#include <clu/assertion.h>
#include <clu/static_vector.h>

#include "common.h"

namespace sltd
{
    namespace
    {
        constexpr int xchain_max_possible_length = (board_size + 1) / 2 * 2 + board_size / 2 * 3;

        // Find all positions in the pattern that could be an end of a strong link.
        // Since every node in an X-chain must be a strongly linked node, we only
        // need to do the search in this reduced set.
        PatternMask find_all_strong_link_nodes(PatternMask pattern)
        {
            PatternMask res;
            for (const int i : pattern.set_bit_indices())
                [&]
                {
                    if (res.test(i)) // Already set, no need to search again
                        return;
                    for (const auto peers = pattern & peer_masks[i]; //
                         const int j : peers.set_bit_indices())
                        if ((peers & peer_masks[j]).none()) // Strongly linked (no other common peer in pattern)
                        {
                            res.set(i).set(j);
                            return;
                        }
                    pattern.reset(i); // Didn't find any strong link starting from this node, remove it
                }();
            return res;
        }

        class XChainFinder
        {
        public:
            XChainFinder(const PatternMask& full_pattern, const PatternMask& link_pattern, const int length):
                full_pattern_(full_pattern), link_pattern_(link_pattern), length_(length)
            {
            }

            std::optional<XChain> try_find()
            {
                for (const int initiating : link_pattern_.set_bit_indices())
                {
                    not_in_chain_ = link_pattern_.reset(initiating);
                    stack_.push_back({.idx = initiating, .other_link = find_strong_links(initiating)});
                    while (!stack_.empty())
                    {
                        if (stack_.size() == length_ + 1) // We've got a chain with the right length
                        {
                            if (const auto eliminations =
                                    peer_masks[stack_[0].idx] & peer_masks[stack_.back().idx] & full_pattern_;
                                eliminations.any())
                            {
                                std::vector<int> node_idx;
                                node_idx.reserve(stack_.size());
                                for (const auto& e : stack_)
                                    node_idx.push_back(e.idx);
                                return XChain{.node_idx = std::move(node_idx), .eliminations = eliminations};
                            }
                        }
                        else if (auto& other_link = stack_.back().other_link; other_link.any()) // Find the next node
                        {
                            const int next = countr_zero(other_link);
                            other_link.reset(next);
                            not_in_chain_.reset(next);
                            stack_.push_back({
                                .idx = next,
                                .other_link = stack_.size() % 2 ? find_weak_links(next) : find_strong_links(next) //
                            });
                            continue;
                        }
                        // Backtrack
                        const auto& back = stack_.back();
                        not_in_chain_.set(back.idx);
                        stack_.pop_back();
                    }
                }
                return std::move(res_);
            }

        private:
            struct StackEntry
            {
                int idx = 0;
                PatternMask other_link; // Possible candidate cell for the other end of the link
            };

            PatternMask full_pattern_; // All candidate cells
            PatternMask link_pattern_; // Strong link candidate cells
            int length_ = 0;
            clu::static_vector<StackEntry, xchain_max_possible_length> stack_;
            PatternMask not_in_chain_;
            std::optional<XChain> res_;

            PatternMask find_strong_links(const int idx) const
            {
                PatternMask res;
                for (const auto weak = find_weak_links(idx); //
                     const int i : weak.set_bit_indices())
                    if ((full_pattern_ & peer_masks[idx] & peer_masks[i]).none())
                        res.set(i);
                return res;
            }

            PatternMask find_weak_links(const int idx) const { return not_in_chain_ & peer_masks[idx]; }
        };
    } // namespace

    std::string XChain::description() const
    {
        const bool turbot = node_idx.size() == 4;
        std::string res = fmt::format("{}:{} number {}, ", //
            turbot ? "Turbot Fish" : "X-Chain", //
            turbot ? "" : fmt::format(" (length {})", node_idx.size() - 1), //
            describe_candidates(candidate));
        for (int i = 0; i < node_idx.size(); i++)
            res += fmt::format("{}{}", cell_name(node_idx[i]), i % 2 ? '-' : '=');
        res.back() = ',';
        res += fmt::format(" [{}!={}]", describe_cells(eliminations), describe_candidates(candidate));
        return res;
    }

    std::optional<XChain> XChain::try_find(const Board& board, IntRange length)
    {
        const auto patterns = board.all_number_patterns();
        PatternMask strong_link_patterns[board_size];
        int max_pattern_size = 0;
        for (int i = 0; i < board_size; i++)
        {
            strong_link_patterns[i] = find_all_strong_link_nodes(patterns[i]);
            max_pattern_size = std::max(max_pattern_size, strong_link_patterns[i].count());
        }
        length.max = (std::min({length.max, xchain_max_possible_length, max_pattern_size - 1}) - 1) / 2 * 2 + 1;
        length.min = std::max(3, length.min / 2 * 2 + 1);
        for (int len = length.min; len <= length.max; len += 2)
            for (int i = 0; i < board_size; i++)
                if (auto opt = XChainFinder(patterns[i], strong_link_patterns[i], len).try_find())
                {
                    opt->candidate = static_cast<CandidateMask>(1 << i);
                    return std::move(opt);
                }
        return std::nullopt;
    }

    void XChain::apply_to(Board& board) const
    {
        for (const int i : eliminations.set_bit_indices())
            board.cells[i] &= ~candidate;
    }
} // namespace sltd
