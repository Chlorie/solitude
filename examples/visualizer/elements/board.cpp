#include "board.h"
#include "sizes.h"
#include "../gui/color_literals.h"
#include "core/SkColorSpace.h"

namespace slvs
{
    namespace
    {
        constexpr float stroke_width(const bool is_box_divider)
        {
            return is_box_divider ? box_divider_width : cell_divider_width;
        }

        CanvasView get_grid_region(CanvasView canvas)
        {
            canvas.best_fit_centered({full_grid_size, full_grid_size});
            constexpr auto offset = box_divider_width / 2;
            canvas.subregion(SkRect::MakeXYWH(offset, offset, grid_size, grid_size));
            return canvas;
        }

        auto get_number_bounds(const SkFont& font)
        {
            std::array<SkRect, sltd::board_size> bounds{};
            for (int i = 0; i < sltd::board_size; i++)
            {
                const std::string num = std::to_string(i + 1);
                font.measureText(num.data(), num.size(), SkTextEncoding::kUTF8, &bounds[i]);
            }
            return bounds;
        }

        SkRect bound_all_rects(const std::span<const SkRect> rects)
        {
            SkRect res = SkRect::MakeEmpty();
            for (const auto& rect : rects)
                res.join(rect);
            return res;
        }
    } // namespace

    void draw_sudoku_grid(const CanvasView& canvas, const Style& style)
    {
        const auto region = get_grid_region(canvas);
        SkPaint paint(style.palette.foreground);
        paint.setStrokeCap(SkPaint::kSquare_Cap);
        for (int i = 0; i <= sltd::board_size; i++)
        {
            const auto pos = static_cast<float>(i) * cell_size;
            paint.setStrokeWidth(stroke_width(i % sltd::box_width == 0));
            region->drawLine(pos, 0, pos, grid_size, paint);
            paint.setStrokeWidth(stroke_width(i % sltd::box_height == 0));
            region->drawLine(0, pos, grid_size, pos, paint);
        }
    }

    void draw_filled_numbers(
        const CanvasView& canvas, const sltd::Board& board, const sltd::PatternMask givens, const Style& style)
    {
        const auto region = get_grid_region(canvas);
        SkPaint paint;
        SkFont font(style.number_typeface, filled_number_size);
        const auto bound = bound_all_rects(get_number_bounds(font));
        for (int i = 0; i < sltd::board_size; i++)
            for (int j = 0; j < sltd::board_size; j++)
            {
                if (!board.filled_at(i, j))
                    continue;
                const int num = std::countr_zero(board.at(i, j));
                const auto pos = get_cell_rect(i, j).center() - bound.center();
                const bool is_given = givens[i * sltd::board_size + j];
                font.setEmbolden(is_given);
                paint.setColor(is_given ? style.palette.foreground : style.palette.non_given_filled_numbers);
                region->drawString(std::to_string(num + 1).c_str(), pos.x(), pos.y(), font, paint);
            }
    }

    void draw_candidates(const CanvasView& canvas, const sltd::Board& board, const Style& style)
    {
        const auto region = get_grid_region(canvas);
        const SkPaint paint(style.palette.foreground);
        const SkFont font(style.number_typeface, candidate_number_size);
        const auto bound = bound_all_rects(get_number_bounds(font));
        for (int i = 0; i < sltd::board_size; i++)
            for (int j = 0; j < sltd::board_size; j++)
            {
                if (board.filled_at(i, j))
                    continue;
                const auto candidates = board.at(i, j);
                for (int k = 0; k < sltd::board_size; k++)
                {
                    if (!(candidates & (1 << k)))
                        continue;
                    const auto pos = get_candidate_position(i, j, k) - bound.center();
                    region->drawString(std::to_string(k + 1).c_str(), pos.x(), pos.y(), font, paint);
                }
            }
    }

    void draw_candidate_highlights(
        const CanvasView& canvas, const std::span<const CandidateHighlight> highlights, const Style& style)
    {
        const auto region = get_grid_region(canvas);
        SkPaint paint;
        paint.setAntiAlias(true);
        for (const auto& hl : highlights)
        {
            paint.setColor(style.palette.candidate_highlight[hl.color]);
            region->drawCircle(
                get_candidate_position(hl.cell / sltd::board_size, hl.cell % sltd::board_size, hl.candidate),
                candidate_circle_radius, paint);
        }
    }

    void draw_cell_highlights(
        const CanvasView& canvas, const std::span<const CellHighlight> highlights, const Style& style)
    {
        const auto region = get_grid_region(canvas);
        SkPaint paint;
        paint.setAntiAlias(true);
        for (const auto& hl : highlights)
        {
            using ColorDataType = decltype(hl.colors)::data_type;
            // TODO: multiple colors
            paint.setColor(style.palette.cell_highlight[std::countr_zero(static_cast<ColorDataType>(hl.colors))]);
            region->drawRect(get_cell_rect(hl.cell / sltd::board_size, hl.cell % sltd::board_size), paint);
        }
    }
} // namespace slvs
