#pragma once

#include <memory>
#include <clu/c_str_view.h>
#include <clu/macros.h>
#include <skia/core/SkSize.h>
#include <SDL2/SDL_keyboard.h>

#include "canvas_view.h"

namespace slvs
{
    class WindowBase
    {
    public:
        CLU_IMMOVABLE_TYPE(WindowBase);
        WindowBase(clu::c_str_view title, SkISize size);
        virtual ~WindowBase() noexcept;
        void run();
        void close();

    protected:
        virtual void on_draw([[maybe_unused]] const CanvasView& canvas) {}
        virtual void on_key_pressed([[maybe_unused]] SDL_Keysym key) {}

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
} // namespace slvs
