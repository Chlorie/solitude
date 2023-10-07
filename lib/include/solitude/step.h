#pragma once

#include <string>
#include <concepts>
#include <memory>
#include <clu/macros.h>

#include "board.h"

SOLITUDE_SUPPRESS_EXPORT_WARNING

namespace sltd
{
    // clang-format off
    template <typename T>
    concept Step = requires(const T& step, Board& board)
    {
        { T::name } -> std::same_as<std::string_view>;
        { step.description() } -> std::same_as<std::string>;
        step.apply_to(board);
    };
    // clang-format on

    class SOLITUDE_API AnyStep final
    {
    public:
        AnyStep() noexcept = default;

        template <Step T>
        explicit AnyStep(T step): ptr_(std::make_unique<Implementation<T>>(std::move(step)))
        {
        }

        CLU_DEFAULT_MOVE_MEMBERS(AnyStep);

        AnyStep(const AnyStep& other): ptr_(other.ptr_->clone()) {}

        AnyStep& operator=(const AnyStep& other)
        {
            AnyStep(other).swap(*this);
            return *this;
        }

        void swap(AnyStep& other) noexcept { ptr_.swap(other.ptr_); }
        SOLITUDE_API friend void swap(AnyStep& lhs, AnyStep& rhs) noexcept { lhs.swap(rhs); }

        [[nodiscard]] explicit operator bool() const noexcept { return ptr_ != nullptr; }

        template <typename T>
        [[nodiscard]] const T* get_if() const noexcept
        {
            return dynamic_cast<const T*>(ptr_.get());
        }

        [[nodiscard]] std::string_view name() const noexcept { return ptr_->name(); }
        [[nodiscard]] std::string description() const { return ptr_->description(); }
        void apply_to(Board& board) const { ptr_->apply_to(board); }

    private:
        class SOLITUDE_API Model
        {
        public:
            Model() noexcept = default;
            virtual ~Model() noexcept = default;
            CLU_IMMOVABLE_TYPE(Model);
            virtual std::string_view name() const noexcept = 0;
            virtual std::string description() const = 0;
            virtual void apply_to(Board& board) const = 0;
            virtual std::unique_ptr<Model> clone() const = 0;
        };

        template <typename T>
        class Implementation final : public Model
        {
        public:
            explicit Implementation(T step): step_(std::move(step)) {}
            std::string_view name() const noexcept override { return T::name; }
            std::string description() const override { return step_.description(); }
            void apply_to(Board& board) const override { step_.apply_to(board); }
            std::unique_ptr<Model> clone() const override { return std::make_unique<Implementation>(step_); }

        private:
            T step_;
        };

        std::unique_ptr<Model> ptr_;
    };
} // namespace sltd

SOLITUDE_RESTORE_EXPORT_WARNING
