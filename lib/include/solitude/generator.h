#pragma once

#include "export.h"
#include "board.h"

namespace sltd
{
    enum class SymmetryType
    {
        none,
        centrosymmetric,
        axisymmetric
    };

    SOLITUDE_API Board generate_minimal_puzzle(SymmetryType symmetry);
}
