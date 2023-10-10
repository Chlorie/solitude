#pragma once

#include <solitude/board.h>
#include <clu/flags.h>

#include "style.h"
#include "../gui/canvas_view.h"

namespace slvs
{
    struct CandidateHighlight
    {
        int cell;
        int candidate;
        Palette::Highlight color;
    };

    struct CellHighlight
    {
        int cell;
        clu::flags<Palette::Highlight> colors;
    };

    void draw_sudoku_grid(const CanvasView& canvas, const Style& style);
    void draw_filled_numbers(
        const CanvasView& canvas, const sltd::Board& board, sltd::PatternMask givens, const Style& style);
    void draw_candidates(const CanvasView& canvas, const sltd::Board& board, const Style& style);
    void draw_candidate_highlights(
        const CanvasView& canvas, std::span<const CandidateHighlight> highlights, const Style& style);
    void draw_cell_highlights(const CanvasView& canvas, std::span<const CellHighlight> highlights, const Style& style);
} // namespace slvs
