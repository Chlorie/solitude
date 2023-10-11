#include "board.h"

#include <cmath>
#include <numbers>
#include <skia/core/SkPathBuilder.h>
#include <skia/core/SkPathMeasure.h>
#include <skia/effects/SkDashPathEffect.h>
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

        float bend_angle_for_direction(const Arrow::BendDirection bend)
        {
            switch (bend)
            {
                case Arrow::BendDirection::left: return -arrow_bend_angle;
                case Arrow::BendDirection::right: return arrow_bend_angle;
                case Arrow::BendDirection::none:
                default: return 0.f;
            }
        }

        SkVector angle_to_vector(const float angle) noexcept { return {std::cos(angle), std::sin(angle)}; }

        void draw_arrow(const CanvasView& canvas, SkPaint& paint, const Arrow& arrow)
        {
            const SkPoint from_center = get_candidate_position(arrow.from_cell, arrow.from_candidate);
            const SkPoint to_center = get_candidate_position(arrow.to_cell, arrow.to_candidate);
            const SkVector diff = to_center - from_center;
            const float direct_angle = std::atan2(diff.y(), diff.x());
            const float bend_angle = bend_angle_for_direction(arrow.bend);
            const SkPoint from_direction = angle_to_vector(direct_angle + bend_angle);
            const SkPoint to_direction = -angle_to_vector(direct_angle - bend_angle);
            const SkPoint from = from_center + from_direction * candidate_circle_radius;
            const SkPoint to = to_center + to_direction * candidate_circle_radius;
            const SkPoint from_ctrl = from + from_direction * arrow_bend_distance;
            const SkPoint to_ctrl = to + to_direction * arrow_bend_distance;

            // The curve goes backwards, since we need to measure the curve from the end later
            const auto curve = arrow.bend == Arrow::BendDirection::none
                ? SkPath().moveTo(to).lineTo(from)
                : SkPath().moveTo(to).cubicTo(to_ctrl, from_ctrl, from);
            canvas->save();
            // Remove the extra part of the curve at the arrow tip
            canvas->clipPath(SkPathBuilder(SkPathFillType::kInverseWinding)
                                 .addCircle(to.x(), to.y(), arrow_tip_clip_radius)
                                 .detach());
            paint.setStroke(true);
            if (arrow.style == Arrow::Style::dashed)
                paint.setPathEffect(SkDashPathEffect::Make(arrow_dash_intervals, 2, 0));
            canvas->drawPath(curve, paint);
            canvas->restore();

            paint.setStroke(false);
            paint.setPathEffect(nullptr);
            SkPoint arrow_tip_inner;
            SkPathMeasure(curve, false).getPosTan(arrow_tip_inner_length, &arrow_tip_inner, nullptr);
            constexpr float tip_half_angle = arrow_tip_angle / 2;
            const SkVector tip_diff = to - arrow_tip_inner;
            const float tip_main_angle = std::atan2(tip_diff.y(), tip_diff.x());
            canvas->drawPath(SkPath()
                                 .moveTo(to)
                                 .lineTo(to - angle_to_vector(tip_main_angle - tip_half_angle) * arrow_tip_outer_length)
                                 .lineTo(to - angle_to_vector(tip_main_angle) * arrow_tip_inner_length)
                                 .lineTo(to - angle_to_vector(tip_main_angle + tip_half_angle) * arrow_tip_outer_length)
                                 .close(),
                paint);
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
        for (const int i : board.filled.set_bit_indices())
        {
            const int num = std::countr_zero(board.cells[i]);
            const auto pos = get_cell_rect(i).center() - bound.center();
            const bool is_given = givens[i];
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
        for (const auto unfilled = ~board.filled; const int i : unfilled.set_bit_indices())
            for (const int j : sltd::set_bit_indices(board.cells[i]))
            {
                const auto pos = get_candidate_position(i, j) - bound.center();
                region->drawString(std::to_string(j + 1).c_str(), pos.x(), pos.y(), font, paint);
            }
    }

    void draw_candidate_highlights(
        const CanvasView& canvas, const std::span<const CandidateHighlight> highlights, const Style& style)
    {
        const auto region = get_grid_region(canvas);
        SkPaint paint;
        paint.setAntiAlias(true);
        for (const auto& hl : highlights)
            draw_multicolor_circle(region, paint, get_candidate_position(hl.cell, hl.candidate),
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
            const auto rect = get_cell_rect(hl.cell);
            region->save();
            region->clipRect(rect);
            const float radius = rect.width() / std::numbers::sqrt2_v<float>;
            draw_multicolor_circle(region, paint, rect.center(), radius, hl.colors, style.palette.cell_highlight);
            region->restore();
        }
    }

    void draw_arrows(const CanvasView& canvas, const std::span<const Arrow> arrows, const Style& style)
    {
        const auto region = get_grid_region(canvas);
        SkPaint paint(style.palette.arrows);
        paint.setAntiAlias(true);
        paint.setStrokeWidth(arrow_width);
        for (const auto& arrow : arrows)
            draw_arrow(canvas, paint, arrow);
    }
} // namespace slvs
