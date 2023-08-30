#pragma once

#include <optional>

#include "../board.h"

SOLITUDE_SUPPRESS_EXPORT_WARNING

namespace sltd
{
    struct SOLITUDE_API XYWing
    {
        static constexpr std::string_view name = "XY-wing";

        int pivot_idx = 0;
        int pincer_idx[2]{};
        CandidateMask candidate = 0;
        PatternMask eliminations;

        std::string description() const;
        static std::optional<XYWing> try_find(const Board& board);
        void apply_to(Board& board) const;
    };

    struct SOLITUDE_API XYZWing
    {
        static constexpr std::string_view name = "XYZ-wing";

        int pivot_idx = 0;
        int pincer_idx[2]{};
        CandidateMask candidate = 0;
        PatternMask eliminations;

        std::string description() const;
        static std::optional<XYZWing> try_find(const Board& board);
        void apply_to(Board& board) const;
    };

    struct SOLITUDE_API WWing
    {
        static constexpr std::string_view name = "W-wing";

        int ends[2]{};
        int link[2]{};
        CandidateMask link_number = 0;
        CandidateMask candidate = 0;
        PatternMask eliminations;

        std::string description() const;
        static std::optional<WWing> try_find(const Board& board);
        void apply_to(Board& board) const;
    };
} // namespace sltd

SOLITUDE_RESTORE_EXPORT_WARNING
