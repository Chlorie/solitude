#pragma once

#include <optional>

#include "../board.h"

SOLITUDE_SUPPRESS_EXPORT_WARNING

// Consider the set of unfilled cells C that lies at the intersection of Box B and Row (or Column) R. Suppose |C|>=2.
// Let V be the set of candidate values to occur in C. Suppose |V|>=|C|+2. The pattern requires that we find |V|-|C|+n
// cells in B and R, with at least one cell in each, with at least |V|-|C| candidates drawn from V and with n the number
// of candidates not drawn from V. Label the sets of cells CB and CR and their candidates VB and VR. Crucially, no
// candidate from V is allowed to appear in VB and VR. Then C must contain V\(VB U VR) [possibly empty], |VB|-|CB|
// elements of VB and |VR|-|CR| elements of VR. The construction allows us to eliminate candidates VB U (V\VR) from
// B\(C U CB), and candidates VR U (V\VB) from R\(C U CR).

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
