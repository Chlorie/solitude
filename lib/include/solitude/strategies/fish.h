#pragma once

#include <optional>

#include "../board.h"

SOLITUDE_SUPPRESS_EXPORT_WARNING

namespace sltd
{
    struct SOLITUDE_API Fish
    {
        static constexpr std::string_view name = "Fish";

        HouseMask base_sets = 0;
        HouseMask cover_sets = 0;
        CandidateMask candidate = 0;
        PatternMask eliminations;

        std::string description() const;
        static std::optional<Fish> try_find(const Board& board, int size);
        void apply_to(Board& board) const;
    };
}

SOLITUDE_RESTORE_EXPORT_WARNING
