#pragma once

#include <optional>

#include "../board.h"

SOLITUDE_SUPPRESS_EXPORT_WARNING

namespace sltd
{
    struct SOLITUDE_API SueDeCoq
    {
        static constexpr std::string_view name = "Sue de Coq";

        PatternMask intersection_cells, line_cells, box_cells;
        CandidateMask intersection_candidates{}, line_candidates{}, box_candidates{};
        PatternMask line_eliminations, box_eliminations;

        CandidateMask line_eliminated_candidates() const noexcept
        {
            return line_candidates | (intersection_candidates & ~box_candidates);
        }

        CandidateMask box_eliminated_candidates() const noexcept
        {
            return box_candidates | (intersection_candidates & ~line_candidates);
        }

        std::string description() const;
        static std::optional<SueDeCoq> try_find(const Board& board, bool extended);
        void apply_to(Board& board) const;
    };
} // namespace sltd

SOLITUDE_RESTORE_EXPORT_WARNING
