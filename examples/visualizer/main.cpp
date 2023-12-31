#include <solitude/generator.h>
#include <solitude/utils.h>

#include "gui/window.h"
#include "gui/canvas_view.h"
#include "gui/color_literals.h"
#include "elements/board.h"

using namespace slvs::literals;

class Window final : public slvs::WindowBase
{
public:
    Window(): WindowBase("Solitude Visualizer", {960, 540})
    {
        style_.palette = slvs::dark_palette();
        generate_new_puzzle();
    }

protected:
    void on_draw(const slvs::CanvasView& canvas) override
    {
        using namespace clu::flag_enum_operators;
        canvas->clear(style_.palette.background);
        {
            const float inset = std::min(canvas.size().width(), canvas.size().height()) * 0.1f;
            auto view = canvas;
            view.subregion(view.rect().makeInset(inset, inset));
            std::vector<slvs::CandidateHighlight> candidates;
            std::vector<slvs::CellHighlight> cells;
            for (int i = 0; i < sltd::cell_count; i++)
                if (!board_.filled[i])
                {
                    candidates.push_back({
                        .cell = i,
                        .candidate = std::countr_zero(solved_board_.cells[i]),
                        .colors = slvs::Palette::chosen //
                    });
                    // cells.push_back({
                    //     .cell = i,
                    //     .colors = slvs::Palette::eliminated | slvs::Palette::grey | slvs::Palette::alternative |
                    //         slvs::Palette::chosen | slvs::Palette::special | slvs::Palette::extra //
                    // });
                }
            const slvs::Arrow arrows[]{{
                .from_cell = candidates[0].cell,
                .to_cell = candidates[2].cell,
                .from_candidate = candidates[0].candidate,
                .to_candidate = candidates[2].candidate,
                .style = slvs::Arrow::Style::dashed,
                .bend = slvs::Arrow::BendDirection::right //
            }};
            slvs::draw_cell_highlights(view, cells, style_);
            slvs::draw_candidate_highlights(view, candidates, style_);
            slvs::draw_sudoku_grid(view, style_);
            slvs::draw_filled_numbers(view, board_, board_.filled, style_);
            slvs::draw_candidates(view, board_, style_);
            slvs::draw_arrows(view, arrows, style_);
        }
    }

    void on_key_pressed(const SDL_Keysym key) override
    {
        if (key.mod != 0)
            return;
        switch (key.sym)
        {
            case SDLK_t:
                use_dark_theme_ = !use_dark_theme_;
                style_.palette = use_dark_theme_ ? slvs::dark_palette() : slvs::light_palette();
                break;
            case SDLK_r: generate_new_puzzle(); break;
            case SDLK_ESCAPE: close(); break;
            default: break;
        }
    }

private:
    bool use_dark_theme_ = true;
    slvs::Style style_;
    sltd::Board board_;
    sltd::Board solved_board_;

    void generate_new_puzzle()
    {
        board_ = sltd::generate_minimal_puzzle(sltd::SymmetryType::centrosymmetric);
        solved_board_ = board_;
        solved_board_.brute_force_solve(1);
    }
};

int main()
{
    Window().run();
    return 0;
}
