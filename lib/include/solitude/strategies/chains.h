#pragma once

#include <optional>
#include <vector>

#include "../board.h"

SOLITUDE_SUPPRESS_EXPORT_WARNING

namespace sltd
{
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
} // namespace sltd

SOLITUDE_RESTORE_EXPORT_WARNING
