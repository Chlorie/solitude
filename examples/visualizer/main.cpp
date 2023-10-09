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
        canvas->clear(style_.palette.background);
        {
            const float inset = std::min(canvas.size().width(), canvas.size().height()) * 0.1f;
            auto view = canvas;
            view.subregion(view.rect().makeInset(inset, inset));
            std::vector<slvs::CandidateHighlight> highlights;
            for (int i = 0; i < sltd::cell_count; i++)
                if (!board_.filled[i])
                    for (const auto j : sltd::set_bit_indices(board_.cells[i]))
                    {
                        const bool correct = (1 << j) == solved_board_.cells[i];
                        highlights.push_back({
                            .cell = i,
                            .candidate = j,
                            .color = correct ? slvs::Palette::chosen : slvs::Palette::eliminated //
                        });
                    }
            slvs::draw_candidate_highlights(view, highlights, style_);
            slvs::draw_sudoku_grid(view, style_);
            slvs::draw_filled_numbers(view, board_, board_.filled, style_);
            slvs::draw_candidates(view, board_, style_);
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
