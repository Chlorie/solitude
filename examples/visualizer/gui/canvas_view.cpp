#include "canvas_view.h"

namespace slvs
{
    CanvasView& CanvasView::operator=(const CanvasView& other) noexcept
    {
        if (&other != this)
        {
            restore();
            canvas_ = other.canvas_;
            canvas_->save();
            save_count_ = canvas_->getSaveCount();
            size_ = other.size_;
        }
        return *this;
    }

    CanvasView::CanvasView(CanvasView&& other) noexcept:
        canvas_(std::exchange(other.canvas_, nullptr)), save_count_(other.save_count_), size_(other.size_)
    {
    }

    CanvasView& CanvasView::operator=(CanvasView&& other) noexcept
    {
        if (&other != this)
        {
            restore();
            canvas_ = std::exchange(other.canvas_, nullptr);
            save_count_ = other.save_count_;
            size_ = other.size_;
        }
        return *this;
    }

    void CanvasView::swap(CanvasView& other) noexcept
    {
        using std::swap;
        swap(canvas_, other.canvas_);
        swap(save_count_, other.save_count_);
        swap(size_, other.size_);
    }

    CanvasView CanvasView::from_size(SkCanvas& canvas, const SkSize size) { return CanvasView(canvas, size); }

    CanvasView CanvasView::from_subregion(SkCanvas& canvas, const SkRect rect)
    {
        CanvasView res(canvas, {rect.width(), rect.height()});
        canvas.translate(rect.x(), rect.y());
        return res;
    }

    CanvasView CanvasView::from_resize(SkCanvas& canvas, const SkSize old_size, const SkSize new_size)
    {
        CanvasView res = from_size(canvas, old_size);
        res.resize(new_size);
        return res;
    }

    void CanvasView::restore() noexcept
    {
        if (canvas_)
            canvas_->restoreToCount(save_count_);
    }

    void CanvasView::resize(const SkSize new_size)
    {
        canvas_->scale(size_.width() / new_size.width(), size_.height() / new_size.height());
        size_ = new_size;
    }

    void CanvasView::best_fit_centered(const SkSize size)
    {
        const float old_aspect = size_.width() / size_.height();
        const float new_aspect = size.width() / size.height();
        const bool fit_width = new_aspect > old_aspect;
        const float scale = fit_width ? size_.width() / size.width() : size_.height() / size.height();
        canvas_->scale(scale, scale);
        if (fit_width)
            canvas_->translate(0, (size_.height() / scale - size.height()) / 2);
        else
            canvas_->translate((size_.width() / scale - size.width()) / 2, 0);
        size_ = size;
    }

    void CanvasView::subregion(const SkRect rect)
    {
        canvas_->translate(rect.x(), rect.y());
        size_ = {rect.width(), rect.height()};
    }

    CanvasView::CanvasView(SkCanvas& canvas, const SkSize size) noexcept: canvas_(&canvas), size_(size)
    {
        save_count_ = canvas.getSaveCount();
        canvas.save();
    }
} // namespace slvs
