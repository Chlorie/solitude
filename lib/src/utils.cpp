#include "solitude/utils.h"

#include <bit>
#include <clu/random.h>

namespace sltd
{
    CandidateMask get_random_bit(const CandidateMask bits) noexcept
    {
        const int max = std::popcount(bits);
        if (max == 1)
            return bits;
        const int num = clu::randint(1, max);
        int index = num - 1;
        while (true)
        {
            const int count = std::popcount(static_cast<CandidateMask>(bits << (15 - index)));
            if (count == num)
                return static_cast<CandidateMask>(1 << index);
            index += num - count;
        }
    }
} // namespace sltd
