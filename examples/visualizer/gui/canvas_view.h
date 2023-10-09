#pragma once

#include <skia/core/SkCanvas.h>
#include <skia/core/SkRect.h>

namespace slvs
{
    class CanvasView
    {
    public:
        ~CanvasView() noexcept { restore(); }
        CanvasView() noexcept = default;
        CanvasView(const CanvasView& other): CanvasView(*other.canvas_, other.size_) {}
        CanvasView& operator=(const CanvasView& other) noexcept;
        CanvasView(CanvasView&& other) noexcept;
        CanvasView& operator=(CanvasView&& other) noexcept;
        void swap(CanvasView& other) noexcept;
        friend void swap(CanvasView& lhs, CanvasView& rhs) noexcept { lhs.swap(rhs); }

        static CanvasView from_size(SkCanvas& canvas, SkSize size);
        static CanvasView from_subregion(SkCanvas& canvas, SkRect rect);
        static CanvasView from_resize(SkCanvas& canvas, SkSize old_size, SkSize new_size);

        [[nodiscard]] explicit operator bool() const noexcept { return canvas_ != nullptr; }
        [[nodiscard]] SkCanvas& operator*() const noexcept { return *canvas_; }
        [[nodiscard]] SkCanvas* operator->() const noexcept { return canvas_; }
        [[nodiscard]] SkSize size() const noexcept { return size_; }
        [[nodiscard]] SkRect rect() const noexcept { return SkRect::MakeSize(size_); }

        void restore() noexcept;
        void resize(SkSize new_size);
        void best_fit_centered(SkSize size);
        void subregion(SkRect rect);

    private:
        SkCanvas* canvas_ = nullptr;
        int save_count_ = 0;
        SkSize size_{};

        CanvasView(SkCanvas& canvas, SkSize size) noexcept;
    };
} // namespace slvs
