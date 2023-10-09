#include "board.h"
#include "sizes.h"
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
                const auto cell_center =
                    SkVector{static_cast<float>(j) + 0.5f, static_cast<float>(i) + 0.5f} * cell_size;
                const auto pos = cell_center - bound.center();
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
        const auto inner_offset =
            SkVector{subdivisions - sltd::box_width, subdivisions - sltd::box_height} * (candidate_region_size / 2);
        for (int i = 0; i < sltd::board_size; i++)
            for (int j = 0; j < sltd::board_size; j++)
            {
                if (board.filled_at(i, j))
                    continue;
                const auto candidates = board.at(i, j);
                const auto cell_offset = SkVector{static_cast<float>(j), static_cast<float>(i)} * cell_size;
                for (int k = 0; k < sltd::board_size; k++)
                {
                    if (!(candidates & (1 << k)))
                        continue;
                    const int kx = k % sltd::box_height, ky = k / sltd::box_width;
                    const auto candidate_offset =
                        SkVector{static_cast<float>(kx) + 0.5f, static_cast<float>(ky) + 0.5f} * candidate_region_size;
                    const auto pos = inner_offset + cell_offset + candidate_offset - bound.center();
                    region->drawString(std::to_string(k + 1).c_str(), pos.x(), pos.y(), font, paint);
                }
            }
    }
} // namespace slvs
