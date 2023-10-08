#include <skia/core/SkFont.h>
#include <skia/core/SkPaint.h>

#include "window.h"

class Window final : public slvs::gui::WindowBase
{
public:
    Window(): WindowBase("Solitude Visualizer", {1280, 720}) {}

protected:
    void on_draw(SkCanvas& canvas) override
    {
        const SkFont font(nullptr, 12);
        canvas.drawRect(SkRect::MakeLTRB(0, 0, 1280, 18), SkPaint(SkColor4f{1, 0.5, 0.5, 0.5}));
        canvas.drawString("The quick brown fox jumps over the lazy dog.", 0, 12, font, SkPaint(SkColor4f{1, 1, 1, 1}));
    }
};

int main()
{
    Window().run();
    return 0;
}
