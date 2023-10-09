#include "style.h"
#include "../gui/color_literals.h"

namespace slvs
{
    const Palette& dark_palette() noexcept
    {
        static const Palette palette{
            .background = 0x171717_rgb,
            .foreground = 0xf0f0f0_rgb,
            .non_given_filled_numbers = 0xa7c7f0_rgb,
            .arrows = 0xe02222_rgb //
        };
        return palette;
    }

    const Palette& light_palette() noexcept
    {
        static Palette palette{
            .background = 0xf0f0f0_rgb,
            .foreground = 0x000000_rgb,
            .non_given_filled_numbers = 0x60a0f0_rgb,
            .arrows = 0xa01717_rgb //
        };
        return palette;
    }
} // namespace slvs
