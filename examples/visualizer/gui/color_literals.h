#pragma once

#include <skia/core/SkColor.h>

namespace slvs::inline literals
{
    [[nodiscard]] constexpr SkColor operator""_rgba8(const unsigned long long value)
    {
        const auto u32 = static_cast<uint32_t>(value);
        const auto rgb = u32 >> 8, a = u32 & 0xff;
        return (a << 24) | rgb;
    }

    [[nodiscard]] constexpr SkColor operator""_rgb8(const unsigned long long value)
    {
        return static_cast<SkColor>((value & 0xffffff) | 0xff000000);
    }

    [[nodiscard]] inline SkColor4f operator""_rgba(const unsigned long long value)
    {
        return SkColor4f::FromColor(operator""_rgba8(value));
    }

    [[nodiscard]] inline SkColor4f operator""_rgb(const unsigned long long value)
    {
        return SkColor4f::FromColor(operator""_rgb8(value));
    }
} // namespace slvs::inline literals
