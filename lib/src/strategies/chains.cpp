#include "solitude/strategies/chains.h"

#include <unordered_map>
#include <unordered_set>
#include <fmt/format.h>
#include <clu/assertion.h>
#include <clu/static_vector.h>

#include "common.h"

namespace sltd
{
    namespace
    {
        constexpr int idx_in_box(const int cell_idx) noexcept
        {
            const int row = cell_idx / board_size;
            return row % box_height * box_width + cell_idx % box_width;
        }

        constexpr PatternMask get_group_mask(const ChainNode node) noexcept
        {
            if (node.idx < board_size) // Row-box intersection
                return PatternMask(node.line_mask) << (board_size * node.idx);
            const int column = node.idx - board_size;
            PatternMask res;
            for (const int i : set_bit_indices(node.line_mask))
                res.set(i * board_size + column);
            return res;
        }

        constexpr PatternMask get_group_peers(const ChainNode node) noexcept
        {
            const auto group_mask = get_group_mask(node);
            const auto houses = house_patterns[node.idx] | house_patterns[node.box_idx + 2 * board_size];
            return houses & ~group_mask;
        }

        constexpr PatternMask get_node_peers(const ChainNode node) noexcept
        {
            return node.type == ChainNode::normal ? peer_masks[node.idx] : get_group_peers(node);
        }

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
                    not_in_chain_ = link_pattern_;
                    not_in_chain_.reset(initiating);
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
                        else if (auto& other = stack_.back().other_link; other.any()) // Find the next node
                        {
                            const int next = countr_zero(other);
                            other.reset(next);
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
                return std::nullopt;
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

        class XYChainFinder
        {
        public:
            XYChainFinder(const Board& board, const PatternMask& bivalue, const int length):
                board_(board), patterns_(board_.all_number_patterns()), bivalue_(bivalue), length_(length)
            {
            }

            std::optional<XYChain> try_find()
            {
                for (const int initiating : bivalue_.set_bit_indices())
                {
                    not_in_chain_ = bivalue_;
                    for (const auto candidates = bitmask_to_array<2>(board_.cells[initiating]);
                         const int set_candidate : candidates)
                    {
                        const int eliminated_candidate = candidates[0] + candidates[1] - set_candidate;
                        push_stack(initiating, eliminated_candidate, eliminated_candidate);
                        while (!stack_.empty())
                        {
                            if (stack_.size() == length_ + 1) // We've got a chain with the right length
                            {
                                if (const auto eliminations = patterns_[eliminated_candidate] &
                                        peer_masks[stack_[0].idx] & peer_masks[stack_.back().idx];
                                    eliminations.any())
                                {
                                    std::vector<int> node_idx;
                                    node_idx.reserve(stack_.size());
                                    for (const auto& e : stack_)
                                        node_idx.push_back(e.idx);
                                    return XYChain{
                                        .node_idx = std::move(node_idx),
                                        .eliminations = eliminations,
                                        .candidate = static_cast<CandidateMask>(1 << eliminated_candidate) //
                                    };
                                }
                            }
                            else if (auto& top = stack_.back(); top.other_link.any()) // Find the next node
                            {
                                const int next = countr_zero(top.other_link);
                                top.other_link.reset(next);
                                push_stack(next, top.number, eliminated_candidate);
                                continue;
                            }
                            // Backtrack
                            const auto& back = stack_.back();
                            not_in_chain_.set(back.idx);
                            stack_.pop_back();
                        }
                    }
                }
                return std::nullopt;
            }

        private:
            struct StackEntry
            {
                int idx = 0;
                int number = 0;
                PatternMask other_link; // Possible candidate cell for the other end of the link
            };

            Board board_;
            std::array<PatternMask, board_size> patterns_;
            PatternMask bivalue_;
            int length_ = 0;
            clu::static_vector<StackEntry, cell_count> stack_;
            PatternMask not_in_chain_;

            void push_stack(const int cell, const int prev_number, const int eliminated_candidate)
            {
                const CandidateMask set_number_mask = board_.cells[cell] ^ (1 << prev_number);
                const int set_number = std::countr_zero(set_number_mask);
                auto other_link_pattern = peer_masks[cell] & not_in_chain_ & patterns_[set_number];
                // The penultimate node cannot be set to the eliminated candidate
                // If the 3rd last number in the chain is to be eliminated, this won't happen
                if (stack_.size() == length_ - 2 && set_number != eliminated_candidate)
                    other_link_pattern &= ~patterns_[eliminated_candidate];
                // The last node must be set to the eliminated candidate
                if (stack_.size() == length_ - 1)
                    other_link_pattern &= patterns_[eliminated_candidate];
                not_in_chain_.reset(cell);
                stack_.push_back({
                    .idx = cell,
                    .number = set_number,
                    .other_link = other_link_pattern //
                });
            }
        };

        class ChainFinder
        {
        public:
            ChainFinder(const Board& board, const std::size_t max_length):
                board_(board), patterns_(board.all_number_patterns()), max_length_(max_length)
            {
            }

            std::optional<Chain> try_find()
            {
                find_all_nodes();
                find_length_one_chains();
                for (std::size_t length = 2; length <= max_length_; length++)
                    if (auto opt = try_next_length(length))
                        return opt;
                return std::nullopt;
            }

        private:
            static constexpr auto null_idx = static_cast<std::size_t>(-1);

            struct NodeEntry
            {
                struct LinkPieceIndex
                {
                    std::size_t node_idx = 0;
                    std::size_t prev_link_idx = 0;
                };

                ChainNode node;
                std::size_t first_order_neighbor_count = 0;
                std::size_t last_length_begin_idx = 0;
                std::vector<bool> neighbors;
                std::vector<bool> linked;
                std::vector<LinkPieceIndex> links;

                void add_piece(const std::size_t node_idx, const std::size_t prev_link_idx = null_idx)
                {
                    linked[node_idx] = true;
                    links.push_back({node_idx, prev_link_idx});
                }

                bool try_add_piece(const std::size_t node_idx, const std::size_t prev_link_idx = null_idx)
                {
                    if (linked[node_idx])
                        return false;
                    add_piece(node_idx, prev_link_idx);
                    return true;
                }
            };

            Board board_;
            std::array<PatternMask, board_size> patterns_;
            std::size_t max_length_ = 0;
            std::unordered_map<ChainNode, std::size_t> node_idx_;
            std::vector<NodeEntry> nodes_; // Even nodes are unset, odd nodes are set
            std::array<std::vector<std::size_t>, house_indices.size()> group_node_idx_;

            void find_all_nodes()
            {
                find_normal_nodes();
                find_group_nodes();
                for (auto& entry : nodes_)
                    entry.linked.resize(nodes_.size()); // Initialize the bitmap
            }

            void add_new_node(const ChainNode node)
            {
                node_idx_[node] = nodes_.size();
                nodes_.emplace_back().node = node;
            }

            constexpr static ChainNode make_normal_node( //
                const int cell_idx, const int number, const bool set) noexcept
            {
                return {
                    .idx = static_cast<std::uint8_t>(cell_idx),
                    .candidate = static_cast<std::uint8_t>(number),
                    .type = ChainNode::normal,
                    .set = set //
                };
            }

            constexpr static ChainNode make_group_node(const int* cell_idx, const CandidateMask mask, //
                const IntersectionInfo& info, const int number, const bool set)
            {
                CandidateMask line_mask = 0;
                CandidateMask box_mask = 0;
                for (const int idx : set_bit_indices(mask))
                {
                    const int cell = cell_idx[idx];
                    line_mask |= 1 << (info.is_row ? cell % board_size : cell / board_size);
                    box_mask |= 1 << idx_in_box(cell);
                }
                return {
                    .line_mask = line_mask,
                    .box_mask = box_mask,
                    .idx = static_cast<std::uint8_t>((info.is_row ? 0 : board_size) + info.line_idx),
                    .box_idx = static_cast<std::uint8_t>(info.box_idx),
                    .candidate = static_cast<std::uint8_t>(number),
                    .type = ChainNode::group,
                    .set = set //
                };
            }

            void find_normal_nodes()
            {
                for (int i = 0; i < cell_count; i++)
                {
                    if (board_.filled[i])
                        continue;
                    for (const int j : set_bit_indices(board_.cells[i]))
                    {
                        add_new_node(make_normal_node(i, j, false));
                        add_new_node(make_normal_node(i, j, true));
                    }
                }
            }

            void find_group_nodes()
            {
                for (const auto& inter : all_intersections)
                {
                    const auto unfilled = inter.line & inter.box & ~board_.filled;
                    if (unfilled.count() < 2)
                        continue;
                    const auto info = get_intersection_info(unfilled);
                    for (int i = 0; i < board_size; i++)
                    {
                        const auto num_mask = unfilled & patterns_[i];
                        static constexpr auto array_size = std::max(box_height, box_width);
                        const CandidateMask max_mask = 1 << num_mask.count();
                        if (max_mask < 3)
                            continue;
                        const auto cell_idx = bitmask_to_array<array_size>(num_mask);
                        for (CandidateMask cell_mask = 3; cell_mask < max_mask; cell_mask++)
                        {
                            add_new_node(make_group_node(cell_idx.data(), cell_mask, info, i, false));
                            add_new_node(make_group_node(cell_idx.data(), cell_mask, info, i, true));
                            add_latest_group_node_idx();
                        }
                    }
                }
            }

            void add_latest_group_node_idx()
            {
                const auto& node = nodes_.back().node;
                group_node_idx_[node.idx].push_back(nodes_.size() - 2);
                group_node_idx_[node.box_idx + 2 * board_size].push_back(nodes_.size() - 2);
            }

            void find_length_one_chains()
            {
                for (std::size_t i = 0; i < nodes_.size(); i += 2)
                {
                    if (nodes_[i].node.type == ChainNode::normal)
                        find_normal_links(i);
                    else
                        find_group_links(i);
                }
                for (auto& entry : nodes_)
                {
                    entry.first_order_neighbor_count = entry.links.size();
                    entry.neighbors = entry.linked;
                }
            }

            void find_normal_links(const std::size_t idx)
            {
                auto& strong_entry = nodes_[idx]; // Even is unset, strong links begins from cell!=x
                auto& weak_entry = nodes_[idx + 1]; // Odd is set, weak links begins from cell==x
                const auto node = strong_entry.node;

                // Weak link from the current candidate to all other candidates in the same cell
                int count = 0;
                std::size_t other_idx = 0;
                for (const int i : set_bit_indices<CandidateMask>(board_.cells[node.idx] ^ (1 << node.candidate)))
                {
                    other_idx = node_idx_.at(make_normal_node(node.idx, i, false));
                    weak_entry.try_add_piece(other_idx); // idx + 1 for "set" node
                    count++;
                }
                if (count == 1) // Bivalue cell, thus a strong link also exists
                    strong_entry.try_add_piece(other_idx + 1);

                for (const auto pattern = peer_masks[node.idx] & patterns_[node.candidate];
                     const int i : pattern.set_bit_indices())
                {
                    other_idx = node_idx_.at(make_normal_node(i, node.candidate, false));
                    weak_entry.try_add_piece(other_idx);
                    // Strong link check: no extra cells with the same candidate in the common peers
                    if ((peer_masks[i] & pattern).none())
                        strong_entry.try_add_piece(other_idx + 1);
                }
            }

            void find_group_links(const std::size_t idx)
            {
                auto& strong_entry = nodes_[idx]; // Even is unset, strong links begins from cell!=x
                auto& weak_entry = nodes_[idx + 1]; // Odd is set, weak links begins from cell==x
                const auto node = strong_entry.node;
                const auto group_pattern = get_group_mask(node);

                // Cell-group links
                for (const auto cells = //
                     (house_patterns[node.idx] | house_patterns[node.box_idx + 2 * board_size]) & //
                         ~group_pattern & patterns_[node.candidate];
                     const int i : cells.set_bit_indices())
                {
                    const std::size_t other_idx = node_idx_.at(make_normal_node(i, node.candidate, false));
                    weak_entry.try_add_piece(other_idx);
                    nodes_[other_idx + 1].try_add_piece(idx);
                    // Strong link check: no extra cells with the same candidate in the common peers
                    if ((peer_masks[i] & cells).none())
                    {
                        strong_entry.try_add_piece(other_idx + 1);
                        nodes_[other_idx].try_add_piece(idx + 1);
                    }
                }

                // Group-group links
                const int group_size = std::popcount(node.line_mask);
                const auto find_links = [&](const int house_idx, CandidateMask ChainNode::*mask_ptr)
                {
                    const int candidates_left =
                        (patterns_[node.candidate] & house_patterns[house_idx]).count() - group_size;
                    for (const std::size_t other_idx : group_node_idx_[house_idx])
                    {
                        const auto other_node = nodes_[other_idx].node;
                        if (other_node.candidate != node.candidate || (node.*mask_ptr & other_node.*mask_ptr))
                            continue;
                        weak_entry.try_add_piece(other_idx);
                        nodes_[other_idx + 1].try_add_piece(idx);
                        // Strong link check: no extra cells with the same candidate in the common peers
                        if (std::popcount(other_node.*mask_ptr) == candidates_left)
                        {
                            strong_entry.try_add_piece(other_idx + 1);
                            nodes_[other_idx].try_add_piece(idx + 1);
                        }
                    }
                };
                find_links(node.idx, &ChainNode::line_mask); // Linked by common line
                find_links(node.box_idx + 2 * board_size, &ChainNode::box_mask); // Linked by common box
            }

            std::optional<Chain> try_next_length(const std::size_t current_length)
            {
                bool added_new_piece = false;
                for (std::size_t node_idx = 0; auto& entry : nodes_)
                {
                    const std::size_t begin = entry.last_length_begin_idx, end = entry.links.size();
                    entry.last_length_begin_idx = end;
                    for (std::size_t link_idx = begin; link_idx < end; link_idx++)
                    {
                        const auto& end_entry = nodes_[entry.links[link_idx].node_idx];
                        for (std::size_t i = 0; i < end_entry.first_order_neighbor_count; i++)
                        {
                            const std::size_t new_end = end_entry.links[i].node_idx;
                            if (new_end == node_idx)
                            {
                                // Only check when node_idx is not set to cut half of the repeated work
                                // Length-2 loops are definitely trivial so skip those
                                if (node_idx % 2 != 0 || current_length == 2)
                                    continue;
                                entry.add_piece(new_end, link_idx);
                                if (auto opt = check_aic_loop(node_idx))
                                    return opt;
                                entry.links.pop_back(); // Self-loop is not useful for further search
                                continue;
                            }
                            if (!entry.try_add_piece(new_end, link_idx))
                                continue;
                            added_new_piece = true;
                            if (new_end / 2 == node_idx / 2)
                            {
                                if (node_idx % 2 == 0)
                                    return construct_verity_chain(node_idx);
                                else
                                    return construct_aic(node_idx);
                            }
                        }
                    }
                    node_idx++;
                }
                if (!added_new_piece)
                    max_length_ = 0; // No new nodes, terminate the outer loop by resetting the max length
                return std::nullopt;
            }

            // The node weak links to itself (A==X -> A!=X), thus eliminating itself.
            // To find the AIC, follow the logic chain a-b'=...=c-a', and find all the
            // candidates that see both b and c, eliminate them.
            Chain construct_aic(const std::size_t node_idx) const
            {
                Chain res{.nodes = collect_nodes_from_last_link(node_idx)};
                res.nodes.pop_back();
                res.nodes.erase(res.nodes.begin()); // Remove the first and last node (a and a')
                const auto& first_entry = nodes_[node_idx_.at(res.nodes.front()) + 1]; // +1 for changing b' to b
                const auto& last_entry = nodes_[node_idx_.at(res.nodes.back())];
                std::vector<bool> first_weak_linked(nodes_.size());
                for (std::size_t i = 0; i < first_entry.first_order_neighbor_count; i++)
                    first_weak_linked[first_entry.links[i].node_idx] = true;
                for (std::size_t i = 0; i < last_entry.first_order_neighbor_count; i++)
                {
                    if (const std::size_t idx = last_entry.links[i].node_idx; first_weak_linked[idx])
                    {
                        if (const ChainNode node = nodes_[idx].node; node.type == ChainNode::normal)
                            res.eliminations[node.candidate].set(node.idx);
                        else
                            res.eliminations[node.candidate] |= get_group_mask(node);
                    }
                }
                return res;
            }

            // The node strong links to itself (A!=X -> A==X), proving a verity.
            Chain construct_verity_chain(const std::size_t node_idx) const
            {
                Chain res{.nodes = collect_nodes_from_last_link(node_idx)};
                const auto node = nodes_[node_idx].node;
                res.eliminations[node.candidate] = patterns_[node.candidate] &
                    (node.type == ChainNode::normal ? peer_masks[node.idx] : get_group_peers(node));
                return res;
            }

            // Just check if any weak link in the loop is not also a strong link by far
            // This function is only called when node_idx % 2 == 0
            std::optional<Chain> check_aic_loop(const std::size_t node_idx) const
            {
                const auto& links = nodes_[node_idx].links;
                std::size_t current_link = links.size() - 1;
                Chain res;
                bool has_eliminations = false;
                while (true)
                {
                    const std::size_t prev_link = links[current_link].prev_link_idx;
                    if (prev_link == null_idx)
                        break;
                    const std::size_t current_idx = links[current_link].node_idx;
                    const std::size_t prev_idx = links[prev_link].node_idx;
                    // Current node is unset, this is a weak link
                    // Weak link is a -> b', we need to check whether a' -> b (corresponding strong link) is also true
                    if (current_idx % 2 == 0 && !nodes_[prev_idx - 1].neighbors[current_idx + 1])
                    {
                        has_eliminations = true;
                        if (const ChainNode prev = nodes_[prev_idx].node, current = nodes_[current_idx].node;
                            has_same_cells(prev, current)) // Linked by same cell
                        {
                            if (prev.type == ChainNode::normal)
                            {
                                for (const CandidateMask eliminations =
                                         board_.cells[prev.idx] ^ (1 << prev.candidate) ^ (1 << current.candidate);
                                     const int i : set_bit_indices(eliminations))
                                    res.eliminations[i].set(prev.idx);
                            }
                            else
                            {
                                const auto cells = get_group_mask(prev);
                                for (int i = 0; i < board_size; i++)
                                {
                                    if (i == prev.candidate || i == current.candidate)
                                        continue;
                                    res.eliminations[i] |= cells & patterns_[i];
                                }
                            }
                        }
                        else // Linked by same number
                        {
                            const auto common_peer = get_node_peers(prev) & get_node_peers(current);
                            res.eliminations[prev.candidate] |= common_peer & patterns_[prev.candidate];
                        }
                    }
                    current_link = prev_link;
                }
                if (has_eliminations)
                {
                    res.nodes = collect_nodes_from_last_link(node_idx);
                    return std::move(res);
                }
                return std::nullopt;
            }

            std::vector<ChainNode> collect_nodes_from_last_link(const std::size_t node_idx) const
            {
                std::vector<ChainNode> nodes;
                const auto& links = nodes_[node_idx].links;
                const auto* current = &links.back();
                while (true)
                {
                    nodes.push_back(nodes_[current->node_idx].node);
                    if (current->prev_link_idx == null_idx)
                    {
                        nodes.push_back(nodes_[node_idx].node);
                        break;
                    }
                    current = &links[current->prev_link_idx];
                }
                std::ranges::reverse(nodes);
                return nodes;
            }
        };

        std::string describe_chain_node(const ChainNode node, const bool is_first)
        {
            const std::string_view link_char = node.set ? "=" : "-";
            return fmt::format("{}{}{} {} ", is_first ? "" : link_char, node.candidate + 1, link_char,
                node.type == ChainNode::normal ? cell_name(node.idx)
                                               : describe_cells_in_house(node.idx, node.line_mask));
        }
    } // namespace

    bool has_same_cells(const ChainNode lhs, const ChainNode rhs) noexcept
    {
        if (lhs.type != rhs.type)
            return false;
        if (lhs.type == ChainNode::normal)
            return lhs.idx == rhs.idx;
        return lhs.idx == rhs.idx && lhs.line_mask == rhs.line_mask; // Group nodes
    }

    std::string XChain::description() const
    {
        const bool turbot = node_idx.size() == 4;
        std::string res = fmt::format("{}:{} number {}, ", //
            turbot ? "Turbot Fish" : "X-Chain", //
            turbot ? "" : fmt::format(" (length {})", node_idx.size() - 1), //
            describe_candidates(candidate));
        for (int i = 0; i < node_idx.size(); i++)
            res += fmt::format("{}{}", cell_name(node_idx[i]), i % 2 ? '-' : '=');
        res.back() = ' ';
        res += fmt::format("=> {}!={}", describe_cells(eliminations), describe_candidates(candidate));
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
                    return opt;
                }
        return std::nullopt;
    }

    void XChain::apply_to(Board& board) const
    {
        for (const int i : eliminations.set_bit_indices())
            board.cells[i] &= ~candidate;
    }

    std::string XYChain::description() const
    {
        const std::string number = describe_candidates(candidate);
        std::string res = fmt::format("XY-Chain: (length {}) {}-", node_idx.size() - 1, number);
        for (const int i : node_idx)
            res += fmt::format("{}-", cell_name(i));
        res += fmt::format("{} => {}!={}", number, describe_cells(eliminations), number);
        return res;
    }

    std::optional<XYChain> XYChain::try_find(const Board& board, IntRange length)
    {
        const auto bivalue = nvalue_cells(board, 2);
        length.min = std::max(length.min, 2);
        length.max = std::min(length.max, bivalue.count() - 1);
        for (int len = length.min; len <= length.max; len++)
            if (auto opt = XYChainFinder(board, bivalue, len).try_find())
                return opt;
        return std::nullopt;
    }

    void XYChain::apply_to(Board& board) const
    {
        for (const int i : eliminations.set_bit_indices())
            board.cells[i] &= ~candidate;
    }

    std::string Chain::description() const
    {
        const auto last_node = nodes.back();
        std::string res = (nodes.front() == last_node) ? "Continuous Nice Loop: " : "Alternate Inference Chain: ";
        res += describe_chain_node(nodes.front(), true);
        for (auto it = std::next(nodes.begin()); it != nodes.end(); ++it)
            if (!has_same_cells(*std::prev(it), *it))
                res += describe_chain_node(*it, false);
        res += fmt::format("{}{} =>", last_node.set ? '-' : '=', last_node.candidate + 1);
        for (int i = 0; i < board_size; i++)
            if (eliminations[i].any())
                res += fmt::format(" {}!={},", describe_cells(eliminations[i]), i + 1);
        res.pop_back();
        return res;
    }

    std::optional<Chain> Chain::try_find(const Board& board, const std::size_t max_length)
    {
        return ChainFinder(board, max_length).try_find();
    }

    void Chain::apply_to(Board& board) const
    {
        for (int i = 0; i < board_size; i++)
            for (const int j : eliminations[i].set_bit_indices())
                board.cells[j] &= ~(1 << i);
    }
} // namespace sltd
