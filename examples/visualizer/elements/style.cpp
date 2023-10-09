#include "style.h"
#include "../gui/color_literals.h"

namespace slvs
{
    const Palette& dark_palette() noexcept
    {
        static const Palette palette{
            .background = 0x181818_rgb,
            .foreground = 0xf0f0f0_rgb,
            .non_given_filled_numbers = 0xa8c8f0_rgb,
            .highlight =
                {
                    0x303030_rgb, // colorless
                    0xc05060_rgb, // eliminated
                    0x60b050_rgb, // chosen
                    0xa8c8f0_rgb, // alternative
                    0xf0a8f0_rgb, // special
                    0xf0c8a8_rgb, // extra
                },
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
            .highlight =
                {
                    0x303030_rgb, // colorless
                    0xf0b8c0_rgb, // eliminated
                    0xb0e0a8_rgb, // chosen
                    0xa8c8f0_rgb, // alternative
                    0xf0a8f0_rgb, // special
                    0xf0c8a8_rgb, // extra
                },
            .arrows = 0xa01818_rgb //
        };
        return palette;
    }
} // namespace slvs
