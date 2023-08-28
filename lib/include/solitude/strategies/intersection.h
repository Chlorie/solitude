#pragma once

#include <optional>

#include "../board.h"

SOLITUDE_SUPPRESS_EXPORT_WARNING

namespace sltd
{
    struct SOLITUDE_API Intersection
    {
        static constexpr std::string_view name = "Intersection";

        int base_house_idx = 0;
        int cover_house_idx = 0;
        CandidateMask candidate = 0;
        CandidateMask base_idx_mask = 0;
        CandidateMask cover_idx_mask = 0;
        CandidateMask cover_elimination_idx_mask = 0;

        std::string description() const;
        static std::optional<Intersection> try_find(const Board& board);
        void apply_to(Board& board) const;
    };
}

SOLITUDE_RESTORE_EXPORT_WARNING
