#pragma once

#include <optional>

#include "../board.h"

SOLITUDE_SUPPRESS_EXPORT_WARNING

namespace sltd
{
    // A remote pair can be replicated by two simple color traps
    struct SOLITUDE_API RemotePair
    {
        static constexpr std::string_view name = "Remote Pair";

        PatternMask groups[2];
        PatternMask eliminations;
        CandidateMask candidates = 0;

        std::string description() const;
        static std::optional<RemotePair> try_find(const Board& board);
        void apply_to(Board& board) const;
    };

    enum class SimpleColorType
    {
        trap,
        wrap
    };

    struct SOLITUDE_API SimpleColors
    {
        static constexpr std::string_view name = "Simple Colors";

        PatternMask colors[2];
        PatternMask eliminations;
        int eliminating_pair[2]{};
        CandidateMask candidate = 0;

        SimpleColorType type() const noexcept;
        std::string description() const;
        static std::optional<SimpleColors> try_find(const Board& board);
        void apply_to(Board& board) const;
    };
} // namespace sltd

SOLITUDE_RESTORE_EXPORT_WARNING
