#pragma once

#include <skia/core/SkColor.h>
#include <skia/core/SkFont.h>

namespace slvs
{
    struct Palette
    {
        SkColor4f background; ///< Background color
        SkColor4f foreground; ///< Grid, all normal text
        SkColor4f non_given_filled_numbers; ///< Numbers filled in (non-givens)
        SkColor4f arrows;
    };

    const Palette& dark_palette() noexcept;
    const Palette& light_palette() noexcept;

    struct Style
    {
        Palette palette = dark_palette();
        sk_sp<SkTypeface> number_typeface = nullptr;
    };
} // namespace slvs
