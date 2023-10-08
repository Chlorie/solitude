#pragma once

#include <memory>
#include <clu/c_str_view.h>
#include <clu/macros.h>
#include <skia/core/SkSize.h>
#include <skia/core/SkCanvas.h>

namespace slvs::gui
{
    class WindowBase
    {
    public:
        CLU_IMMOVABLE_TYPE(WindowBase);
        WindowBase(clu::c_str_view title, SkISize size);
        virtual ~WindowBase() noexcept;
        void run();

    protected:
        virtual void on_draw([[maybe_unused]] SkCanvas& canvas) {}

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
} // namespace slvs::gui
