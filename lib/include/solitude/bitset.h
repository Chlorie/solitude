#pragma once

#include <cstdint>
#include <cstring>
#include <bit>
#include <span>
#include <clu/type_traits.h>

namespace sltd
{
    template <int Size>
    class Bitset
    {
    public:
        using ElementType = clu::conditional_t<(Size <= 8), std::uint8_t,
            clu::conditional_t<(Size <= 16), std::uint16_t,
                clu::conditional_t<(Size <= 32), std::uint32_t, std::uint64_t>>>;
        static constexpr int array_size = (Size - 1) / (8 * sizeof(ElementType)) + 1;

        class Reference
        {
        public:
            constexpr Reference(Bitset& self, const int i) noexcept: self_(&self), idx_(i) {}

            constexpr Reference& operator=(const bool b) noexcept
            {
                self_->set(idx_, b);
                return *this;
            }

            constexpr Reference& operator=(const Reference other) noexcept
            {
                self_->set(idx_, static_cast<bool>(other));
                return *this;
            }

            constexpr operator bool() const noexcept { return self_->test(idx_); }
            constexpr bool operator~() const noexcept { return !self_->test(idx_); }
            constexpr void flip() const noexcept { self_->flip(idx_); }

        private:
            Bitset* self_;
            int idx_;
        };

        class SetBitEnumerator
        {
        public:
            struct Sentinel
            {
            };

            class Iterator
            {
            public:
                constexpr explicit Iterator(const Bitset& self) noexcept: self_(&self) { ++*this; }

                constexpr friend bool operator==(const Iterator it, Sentinel) noexcept { return it.idx_ >= Size; }

                constexpr Iterator& operator++() noexcept
                {
                    idx_++;
                    int i = idx_ / element_width;
                    const int remainder = idx_ % element_width;
                    if (const ElementType masked_element = self_->data_[i] & (all_bits_set_mask << remainder))
                    {
                        idx_ = i * element_width + std::countr_zero(masked_element);
                        return *this;
                    }
                    for (++i; i < array_size; ++i)
                        if (self_->data_[i] != 0)
                        {
                            idx_ = i * element_width + std::countr_zero(self_->data_[i]);
                            return *this;
                        }
                    idx_ = Size;
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
                const Bitset* self_;
                int idx_ = -1;
            };

            constexpr explicit SetBitEnumerator(const Bitset& self) noexcept: self_(&self) {}
            constexpr Iterator begin() const noexcept { return Iterator(*self_); }
            constexpr Sentinel end() const noexcept { return Sentinel{}; }

        private:
            const Bitset* self_;
        };

        constexpr Bitset() noexcept = default;

        template <typename Uint>
            requires std::is_unsigned_v<Uint>
        constexpr explicit Bitset(const Uint value) noexcept: //
            data_{static_cast<ElementType>(value & last_element_mask)}
        {
        }

        constexpr friend bool operator==(const Bitset&, const Bitset&) noexcept = default;

        constexpr Reference operator[](const int i) noexcept { return Reference(*this, i); }
        constexpr bool operator[](const int i) const noexcept { return test(i); }

        constexpr bool test(const int i) const noexcept
        {
            return static_cast<bool>(data_[i / element_width] & (ElementType{1} << (i % element_width)));
        }

        constexpr bool all() const noexcept
        {
            for (int i = 0; i < array_size - 1; i++)
                if (data_[i] != all_bits_set_mask)
                    return false;
            return data_[array_size - 1] == last_element_mask;
        }

        constexpr bool any() const noexcept
        {
            for (const auto e : data_)
                if (e != 0)
                    return true;
            return false;
        }

        constexpr bool none() const noexcept { return !any(); }

        constexpr int count() const noexcept
        {
            int res = 0;
            for (const auto e : data_)
                res += std::popcount(e);
            return res;
        }

        constexpr auto set_bit_indices() const noexcept { return SetBitEnumerator(*this); }

        constexpr static int size() noexcept { return Size; }

        constexpr Bitset& operator&=(const Bitset& other) noexcept
        {
            for (int i = 0; i < array_size; i++)
                data_[i] &= other.data_[i];
            return *this;
        }

        constexpr Bitset& operator|=(const Bitset& other) noexcept
        {
            for (int i = 0; i < array_size; i++)
                data_[i] |= other.data_[i];
            return *this;
        }

        constexpr Bitset& operator^=(const Bitset& other) noexcept
        {
            for (int i = 0; i < array_size; i++)
                data_[i] ^= other.data_[i];
            return *this;
        }

        constexpr Bitset operator~() const noexcept { return Bitset(*this).flip(); }

        constexpr Bitset& operator<<=(int x) noexcept
        {
            if (const int element_shift = x / element_width; element_shift != 0)
            {
                for (int i = array_size - 1; i >= 0; --i)
                    data_[i] = i < element_shift ? 0 : data_[i - element_shift];
            }
            if ((x %= element_width) != 0)
            {
                for (int i = array_size - 1; i > 0; --i)
                    data_[i] = (data_[i] << x) | (data_[i - 1] >> (element_width - x));
                data_[0] <<= x;
            }
            data_[array_size - 1] &= last_element_mask;
            return *this;
        }

        constexpr Bitset& operator>>=(int x) noexcept
        {
            if (const int element_shift = x / element_width; element_shift != 0)
            {
                for (int i = 0; i < array_size; i++)
                    data_[i] = i + element_shift >= array_size ? 0 : data_[i + element_shift];
            }
            if ((x %= element_width) != 0)
            {
                for (int i = 0; i < array_size - 1; i++)
                    data_[i] = (data_[i] >> x) | (data_[i + 1] << (element_width - x));
                data_[array_size - 1] >>= x;
            }
            return *this;
        }

        constexpr Bitset operator<<(const int x) noexcept { return Bitset(*this) <<= x; }

        constexpr Bitset operator>>(const int x) noexcept { return Bitset(*this) >>= x; }

        constexpr friend Bitset operator&(const Bitset& lhs, const Bitset& rhs) noexcept { return Bitset(lhs) &= rhs; }
        constexpr friend Bitset operator|(const Bitset& lhs, const Bitset& rhs) noexcept { return Bitset(lhs) |= rhs; }
        constexpr friend Bitset operator^(const Bitset& lhs, const Bitset& rhs) noexcept { return Bitset(lhs) ^= rhs; }

        constexpr Bitset& set() noexcept
        {
            if (std::is_constant_evaluated())
                for (auto& e : data_)
                    e = all_bits_set_mask;
            else
                std::memset(data_, 0xff, sizeof(data_));
            data_[array_size - 1] = last_element_mask;
            return *this;
        }

        constexpr Bitset& set(const int i) noexcept
        {
            data_[i / element_width] |= ElementType{1} << (i % element_width);
            return *this;
        }

        constexpr Bitset& set(const int i, const bool b) noexcept { return b ? set(i) : reset(i); }

        constexpr Bitset& reset() noexcept
        {
            if (std::is_constant_evaluated())
                for (auto& e : data_)
                    e = 0;
            else
                std::memset(data_, 0, sizeof(data_));
            return *this;
        }

        constexpr Bitset& reset(const int i) noexcept
        {
            data_[i / element_width] &= ~(ElementType{1} << (i % element_width));
            return *this;
        }

        constexpr Bitset& flip() noexcept
        {
            for (auto& e : data_)
                e = ~e;
            data_[array_size - 1] &= last_element_mask;
            return *this;
        }

        constexpr Bitset& flip(const int i) noexcept
        {
            data_[i / element_width] ^= ElementType{1} << (i % element_width);
            return *this;
        }

        constexpr ElementType& get_element(const int i) noexcept { return data_[i]; }
        constexpr ElementType get_element(const int i) const noexcept { return data_[i]; }

        constexpr std::span<ElementType, array_size> data() noexcept { return data_; }
        constexpr std::span<const ElementType, array_size> data() const noexcept { return data_; }

    private:
        static constexpr int element_width = 8 * sizeof(ElementType);
        static constexpr ElementType all_bits_set_mask = ~ElementType{};
        static constexpr ElementType last_element_mask = all_bits_set_mask >> (element_width - Size % element_width);

        ElementType data_[array_size]{};
    };
} // namespace sltd
