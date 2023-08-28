#pragma once

#include <string>
#include <optional>

#include "../board.h"

SOLITUDE_SUPPRESS_EXPORT_WARNING

namespace sltd
{
    struct SOLITUDE_API NakedSingle
    {
        static constexpr std::string_view name = "NakedSingle";

        int cell_idx = 0;
        CandidateMask candidate = 0;

        std::string description() const;
        static std::optional<NakedSingle> try_find(const Board& board);
        void apply_to(Board& board) const;
    };

    struct SOLITUDE_API NakedSubset
    {
        static constexpr std::string_view name = "NakedSubset";

        int house_idx = 0;
        CandidateMask idx_mask = 0; // Cell bitmask in the house
        CandidateMask candidates = 0;

        std::string description() const;
        static std::optional<NakedSubset> try_find(const Board& board, int size);
        void apply_to(Board& board) const;
    };

    struct SOLITUDE_API HiddenSubset
    {
        static constexpr std::string_view name = "HiddenSubset";

        int house_idx = 0;
        CandidateMask idx_mask = 0; // Cell bitmask in the house
        CandidateMask candidates = 0;

        std::string description() const;
        static std::optional<HiddenSubset> try_find(const Board& board, int size);
        void apply_to(Board& board) const;
    };
} // namespace sltd

SOLITUDE_RESTORE_EXPORT_WARNING
