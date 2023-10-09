#pragma once

#include <skia/core/SkColor.h>
#include <skia/core/SkFont.h>

namespace slvs
{
    struct Palette
    {
        enum Highlight
        {
            colorless, ///< Colorless highlight
            eliminated, ///< Eliminations
            chosen, ///< Chosen candidates
            alternative, ///< Other candidates (fins, other outcome of chains, etc.)
            special, ///< Special candidates (Start of a chain, etc.)
            extra ///< Extra hint color
        };

        SkColor4f background; ///< Background color
        SkColor4f foreground; ///< Grid, all normal text
        SkColor4f non_given_filled_numbers; ///< Numbers filled in (non-givens)
        SkColor4f highlight[6]; ///< Highlight colors
        SkColor4f arrows; ///< Arrows for chains
    };

    const Palette& dark_palette() noexcept;
    const Palette& light_palette() noexcept;

    struct Style
    {
        Palette palette = dark_palette();
        sk_sp<SkTypeface> number_typeface = nullptr;
    };
} // namespace slvs
