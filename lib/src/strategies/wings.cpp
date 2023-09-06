#include "solitude/strategies/wings.h"

#include <fmt/format.h>

#include "common.h"

namespace sltd
{
    std::string XYWing::description() const
    {
        return fmt::format("XY-Wing: {}->{},{}, [{}!={}]", //
            cell_name(pivot_idx), cell_name(pincer_idx[0]), cell_name(pincer_idx[1]), //
            describe_cells(eliminations), describe_candidates(candidate));
    }

    std::optional<XYWing> XYWing::try_find(const Board& board)
    {
        const auto patterns = board.all_number_patterns();
        for (const auto bivalue = nvalue_cells(board, 2); //
             const int pivot : bivalue.set_bit_indices())
        {
            const auto bivalue_peers = peer_masks[pivot] & bivalue;
            if (bivalue_peers.count() < 2) // We need at least two pincers
                continue;
            const auto [x, y] = bitmask_to_array<2>(board.cells[pivot]);
            const CandidateMask not_xy = ~board.cells[pivot];
            // Ensure that z is not x or y
            const auto xz_pattern = bivalue_peers & patterns[x] & ~patterns[y];
            const auto yz_pattern = bivalue_peers & patterns[y] & ~patterns[x];
            for (const int first : xz_pattern.set_bit_indices())
            {
                const CandidateMask z_mask = board.cells[first] & not_xy;
                const int z = std::countr_zero(z_mask);
                for (const int second : yz_pattern.set_bit_indices())
                {
                    if (z_mask != (board.cells[second] & not_xy)) // Not the same z
                        continue;
                    if (const auto eliminations = peer_masks[first] & peer_masks[second] & patterns[z];
                        eliminations.any())
                        return XYWing{
                            .pivot_idx = pivot,
                            .pincer_idx = {first, second},
                            .candidate = z_mask,
                            .eliminations = eliminations //
                        };
                }
            }
        }
        return std::nullopt;
    }

    void XYWing::apply_to(Board& board) const
    {
        for (const int i : eliminations.set_bit_indices())
            board.cells[i] &= ~candidate;
    }

    std::string XYZWing::description() const
    {
        return fmt::format("XYZ-Wing: {}->{},{}, [{}!={}]", //
            cell_name(pivot_idx), cell_name(pincer_idx[0]), cell_name(pincer_idx[1]), //
            describe_cells(eliminations), describe_candidates(candidate));
    }

    std::optional<XYZWing> XYZWing::try_find(const Board& board)
    {
        const auto patterns = board.all_number_patterns();
        const auto bivalue = nvalue_cells(board, 2);
        for (const auto trivalue = nvalue_cells(board, 3); //
             const int pivot : trivalue.set_bit_indices())
        {
            const auto bivalue_peers = peer_masks[pivot] & bivalue;
            if (bivalue_peers.count() < 2) // We need at least two pincers
                continue;
            const CandidateMask xyz_mask = board.cells[pivot];
            for (const int first : bivalue_peers.set_bit_indices())
            {
                const auto xz_mask = board.cells[first];
                if ((xz_mask & xyz_mask) != xz_mask) // xz should be in xyz
                    continue;
                const CandidateMask y_mask = xyz_mask & ~xz_mask;
                const int y = std::countr_zero(y_mask);
                for (const auto bivalue_peers_with_y = bivalue_peers & patterns[y];
                     const int second : bivalue_peers_with_y.set_bit_indices())
                {
                    const auto yz_mask = board.cells[second];
                    if ((yz_mask & xyz_mask) != yz_mask) // yz should be in xyz
                        continue;
                    const CandidateMask z_mask = xz_mask & yz_mask;
                    const int z = std::countr_zero(z_mask);
                    if (const auto eliminations =
                            peer_masks[first] & peer_masks[second] & peer_masks[pivot] & patterns[z];
                        eliminations.any())
                        return XYZWing{
                            .pivot_idx = pivot,
                            .pincer_idx = {first, second},
                            .candidate = z_mask,
                            .eliminations = eliminations //
                        };
                }
            }
        }
        return std::nullopt;
    }

    void XYZWing::apply_to(Board& board) const
    {
        for (const int i : eliminations.set_bit_indices())
            board.cells[i] &= ~candidate;
    }

    std::string WWing::description() const
    {
        return fmt::format("W-Wing: linked by {}, {}-{}={}-{}, [{}!={}]", //
            describe_candidates(link_number), //
            cell_name(ends[0]), cell_name(link[0]), cell_name(link[1]), cell_name(ends[1]), //
            describe_cells(eliminations), describe_candidates(candidate));
    }

    std::optional<WWing> WWing::try_find(const Board& board)
    {
        const auto patterns = board.all_number_patterns();
        for (const auto bivalue = nvalue_cells(board, 2); //
             const int first_end : bivalue.set_bit_indices())
        {
            const auto pair = bitmask_to_array<2>(board.cells[first_end]);
            // The other end should have the same candidates as the first one,
            // and the two should not see each other (or it would just be a naked pair)
            for (const auto second_end_pattern =
                     (bivalue & patterns[pair[0]] & patterns[pair[1]] & ~peer_masks[first_end]).reset(first_end);
                 const int second_end : second_end_pattern.set_bit_indices())
            {
                const auto common_peers = peer_masks[first_end] & peer_masks[second_end];
                for (const int eliminated : pair) // Eliminated number, one of the two candidate numbers
                {
                    const auto eliminations = common_peers & patterns[eliminated];
                    if (eliminations.none())
                        continue;
                    const int linked = pair[0] + pair[1] - eliminated; // Strong linked number
                    for (const auto first_link_pattern = peer_masks[first_end] & patterns[linked];
                         const int first_link : first_link_pattern.set_bit_indices())
                        for (const auto second_link_pattern = peer_masks[second_end] & patterns[linked];
                             const int second_link : second_link_pattern.set_bit_indices())
                        {
                            // 1. The two linked cells must see each other
                            // 2. The two cells' common peers must not contain the linked number
                            if (!peer_masks[first_link].test(second_link) ||
                                (peer_masks[first_link] & peer_masks[second_link] & patterns[linked]).any())
                                continue;
                            return WWing{
                                .ends = {first_end, second_end},
                                .link = {first_link, second_link},
                                .link_number = static_cast<CandidateMask>(1 << linked),
                                .candidate = static_cast<CandidateMask>(1 << eliminated),
                                .eliminations = eliminations //
                            };
                        }
                }
            }
        }
        return std::nullopt;
    }

    void WWing::apply_to(Board& board) const
    {
        for (const int i : eliminations.set_bit_indices())
            board.cells[i] &= ~candidate;
    }
} // namespace sltd
