#pragma once

#include <solitude/board.h>

#include "style.h"
#include "../gui/canvas_view.h"

namespace slvs
{
    void draw_sudoku_grid(const CanvasView& canvas, const Style& style);
    void draw_filled_numbers(
        const CanvasView& canvas, const sltd::Board& board, sltd::PatternMask givens, const Style& style);
    void draw_candidates(const CanvasView& canvas, const sltd::Board& board, const Style& style);
} // namespace slvs
