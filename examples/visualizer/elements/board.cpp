#include "board.h"

#include <numbers>
#include <skia/core/SkPath.h>
#include <solitude/utils.h>

#include "constants.h"
#include "../gui/color_literals.h"

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

        SkPath make_sector(
            const SkPoint center, const float radius, const float from_degrees, const float sweep_degrees)
        {
            const auto bounding_rect =
                SkRect::MakeLTRB(center.x() - radius, center.y() - radius, center.x() + radius, center.y() + radius);
            return SkPath{}.addArc(bounding_rect, from_degrees, sweep_degrees).lineTo(center).close();
        }

        void draw_multicolor_circle(const CanvasView& canvas, SkPaint& paint, const SkPoint center, const float radius,
            const clu::flags<Palette::Highlight> color_flags, const std::span<const SkColor4f> colors)
        {
            using ColorDataType = decltype(color_flags)::data_type;
            const auto bits = static_cast<ColorDataType>(color_flags);
            const int n_colors = std::popcount(bits);
            if (bits == 0)
                return;
            const float sweep_degrees = 360.f / static_cast<float>(n_colors);
            for (int idx = -1; const int color : sltd::set_bit_indices(bits))
            {
                idx++;
                paint.setColor(colors[color]);
                canvas->drawPath(
                    make_sector(center, radius, //
                        candidate_multicolor_offset_degrees + static_cast<float>(idx) * sweep_degrees, sweep_degrees),
                    paint);
            }
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
            draw_multicolor_circle(region, paint,
                get_candidate_position(hl.cell / sltd::board_size, hl.cell % sltd::board_size, hl.candidate),
                candidate_circle_radius, hl.colors, style.palette.candidate_highlight);
    }

    void draw_cell_highlights(
        const CanvasView& canvas, const std::span<const CellHighlight> highlights, const Style& style)
    {
        const auto region = get_grid_region(canvas);
        SkPaint paint;
        paint.setAntiAlias(true);
        for (const auto& hl : highlights)
        {
            const auto rect = get_cell_rect(hl.cell / sltd::board_size, hl.cell % sltd::board_size);
            region->save();
            region->clipRect(rect);
            const float radius = rect.width() / std::numbers::sqrt2_v<float>;
            draw_multicolor_circle(region, paint, rect.center(), radius, hl.colors, style.palette.cell_highlight);
            region->restore();
        }
    }
} // namespace slvs
