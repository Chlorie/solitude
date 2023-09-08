#pragma once

#include <optional>

#include "../board.h"

SOLITUDE_SUPPRESS_EXPORT_WARNING

namespace sltd
{
    struct SOLITUDE_API AlsXZ
    {
        static constexpr std::string_view name = "ALS-XZ";

        PatternMask als[2];
        CandidateMask candidates[2]{};
        CandidateMask rcc = 0;
        PatternMask eliminations[board_size];

        std::string description() const;
        static std::optional<AlsXZ> try_find(const Board& board);
        void apply_to(Board& board) const;
    };

    struct SOLITUDE_API AlsXYWing
    {
        static constexpr std::string_view name = "ALS-XY-Wing";

        PatternMask pivot, pincers[2];
        CandidateMask pivot_candidates = 0, pincer_candidates[2]{};
        CandidateMask x = 0, y = 0;
        PatternMask eliminations[board_size];
        
        std::string description() const;
        static std::optional<AlsXYWing> try_find(const Board& board);
        void apply_to(Board& board) const;
    };
} // namespace sltd

SOLITUDE_RESTORE_EXPORT_WARNING
