#pragma once

#include <optional>
#include <vector>

#include "../board.h"

SOLITUDE_SUPPRESS_EXPORT_WARNING

namespace sltd
{
    struct SOLITUDE_API ChainNode
    {
        enum Type : uint8_t
        {
            normal,
            grouped
        };

        std::uint16_t group_mask = 0; // Bit i is set for cell #i in house #idx
        std::uint8_t idx = 0; // Cell index for normal nodes, house index for group nodes
        std::uint8_t candidate : 4 = 0; // The candidate number in question
        Type type : 2 = normal;
        bool set : 2 = false; // Whether the candidate is set or not
    };

    struct SOLITUDE_API IntRange
    {
        int min = 0;
        int max = board_size * cell_count;
    };

    struct SOLITUDE_API XChain
    {
        static constexpr std::string_view name = "X-Chain";

        std::vector<int> node_idx;
        PatternMask eliminations;
        CandidateMask candidate = 0;

        std::string description() const;
        static std::optional<XChain> try_find(const Board& board, IntRange length);
        void apply_to(Board& board) const;
    };

    struct SOLITUDE_API XYChain
    {
        static constexpr std::string_view name = "XY-Chain";

        std::vector<int> node_idx;
        PatternMask eliminations;
        CandidateMask candidate = 0;

        std::string description() const;
        static std::optional<XYChain> try_find(const Board& board, IntRange length);
        void apply_to(Board& board) const;
    };

    struct SOLITUDE_API Chain
    {
        static constexpr std::string_view name = "Chain";

        std::vector<ChainNode> nodes;
        PatternMask eliminations[board_size];

        std::string description() const;
        static std::optional<Chain> try_find(const Board& board, IntRange length, bool allow_grouped_nodes);
        void apply_to(Board& board) const;
    };
} // namespace sltd

SOLITUDE_RESTORE_EXPORT_WARNING
