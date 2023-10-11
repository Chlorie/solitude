#include "style.h"
#include "../gui/color_literals.h"

namespace slvs
{
    namespace
    {
        SkColor4f blend(SkColor4f fg, SkColor4f bg, const float factor)
        {
            fg = fg * factor;
            bg = bg * (1 - factor);
            return {fg.fR + bg.fR, fg.fG + bg.fG, fg.fB + bg.fB, fg.fA + bg.fA};
        }

        Palette create_dark_palette() noexcept
        {
            Palette res{
                .background = 0x181818_rgb,
                .foreground = 0xf0f0f0_rgb,
                .non_given_filled_numbers = 0xa8c8f0_rgb,
                .candidate_highlight =
                    {
                        0x808080_rgb, // colorless
                        0x903040_rgb, // eliminated
                        0x307840_rgb, // chosen
                        0x4050a0_rgb, // alternative
                        0x804090_rgb, // special
                        0x805828_rgb, // extra
                    },
                .arrows = 0xe04848_rgb //
            };
            for (size_t i = 0; i < res.candidate_highlight.size(); i++)
                res.cell_highlight[i] = blend(res.candidate_highlight[i], res.background, 0.4f);
            return res;
        }

        Palette create_light_palette() noexcept
        {
            Palette res{
                .background = 0xf0f0f0_rgb,
                .foreground = 0x000000_rgb,
                .non_given_filled_numbers = 0x60a0f0_rgb,
                .candidate_highlight =
                    {
                        0x808080_rgb, // colorless
                        0xf0a0a8_rgb, // eliminated
                        0x98d890_rgb, // chosen
                        0xa8b8f0_rgb, // alternative
                        0xe0a8f0_rgb, // special
                        0xf0c8a0_rgb, // extra
                    },
                .arrows = 0xe02828_rgb //
            };
            for (size_t i = 0; i < res.candidate_highlight.size(); i++)
                res.cell_highlight[i] = blend(res.candidate_highlight[i], res.background, 0.3f);
            return res;
        }
    } // namespace

    const Palette& dark_palette() noexcept
    {
        static const Palette palette = create_dark_palette();
        return palette;
    }

    const Palette& light_palette() noexcept
    {
        static const Palette palette = create_light_palette();
        return palette;
    }
} // namespace slvs
