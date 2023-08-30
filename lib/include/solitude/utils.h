#pragma once

#include "board.h"

namespace sltd
{
    SOLITUDE_API CandidateMask get_random_bit(CandidateMask bits) noexcept;

    constexpr CandidateMask get_rightmost_set_bit(const CandidateMask bits) noexcept { return bits & -bits; }

    template <typename T>
        requires std::is_unsigned_v<T>
    class SetBitsEnumerator
    {
    public:
        struct Sentinel
        {
        };

        class Iterator
        {
        public:
            constexpr explicit Iterator(const T value): remain_(value) { ++*this; }

            constexpr friend bool operator==(const Iterator it, Sentinel) noexcept { return it.idx_ >= 8 * sizeof(T); }

            constexpr Iterator& operator++() noexcept
            {
                const int step = std::countr_zero(remain_) + 1;
                remain_ >>= step;
                idx_ += step;
                return *this;
            }

            constexpr Iterator operator++(int) noexcept
            {
                Iterator res = *this;
                ++*this;
                return res;
            }

            constexpr int operator*() const noexcept { return idx_; }

        private:
            T remain_;
            int idx_ = -1;
        };

        constexpr explicit SetBitsEnumerator(const T value) noexcept: value_(value) {}
        constexpr Iterator begin() const noexcept { return Iterator(value_); }
        constexpr Sentinel end() const noexcept { return Sentinel{}; }

    private:
        T value_;
    };

    template <typename T>
        requires std::is_unsigned_v<T>
    constexpr auto set_bit_indices(const T value) noexcept
    {
        return SetBitsEnumerator<T>(value);
    }
} // namespace sltd
