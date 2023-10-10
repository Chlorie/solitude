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
            extra, ///< Extra hint color

            flags_bit_size
        };

        SkColor4f background; ///< Background color
        SkColor4f foreground; ///< Grid, all normal text
        SkColor4f non_given_filled_numbers; ///< Numbers filled in (non-givens)
        std::array<SkColor4f, flags_bit_size> candidate_highlight; ///< Candidate highlight colors
        std::array<SkColor4f, flags_bit_size> cell_highlight; ///< Cell highlight colors
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
