#pragma once

#include "board.h"

namespace sltd
{
    SOLITUDE_API CandidateMask get_random_bit(CandidateMask bits) noexcept;

    constexpr CandidateMask get_rightmost_set_bit(const CandidateMask bits) noexcept { return bits & -bits; }
}
