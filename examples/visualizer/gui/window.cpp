#include "window.h"

#include "include/core/SkImage.h"
#include "include/gpu/GrBackendSurface.h"

#include <fmt/format.h>
#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <skia/core/SkSurface.h>
#include <skia/core/SkColorSpace.h>
#include <skia/gpu/GrDirectContext.h>
#include <skia/gpu/GrBackendSurface.h>

namespace slvs
{
    namespace
    {
        [[noreturn]] void sdl_hard_error()
        {
            const char* error = SDL_GetError();
            fmt::println(stderr, "{}", error);
            SDL_ClearError();
            throw std::runtime_error(error);
        }

        [[noreturn]] void hard_error(const std::string& msg)
        {
            fmt::println(stderr, "{}", msg);
            throw std::runtime_error(msg);
        }

        struct SdlLifetime
        {
            SdlLifetime()
            {
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
                SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
                SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
                SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
                SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
                SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
                SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
                SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
                SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
                SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
                SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1");
                if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
                    sdl_hard_error();
            }

            ~SdlLifetime() noexcept { SDL_Quit(); }
        };

        template <auto Deleter>
        consteval auto lambdaify()
        {
            return [](auto obj) noexcept { Deleter(obj); };
        }

        template <auto Deleter>
        using Lambdaified = decltype(lambdaify<Deleter>());
    } // namespace

    class WindowBase::Impl final
    {
    public:
        Impl(WindowBase* parent, const clu::c_str_view title, const SkISize size): parent_(parent), size_(size)
        {
            static SdlLifetime sdl;
            setup_window(title);
            setup_skia_gl_interface();
            setup_surface();
        }

        void run()
        {
            while (!quitting_)
            {
                handle_events();
                SkCanvas& canvas = *surface_->getCanvas();
                canvas.clear(SK_ColorTRANSPARENT);
                parent_->on_draw(CanvasView::from_resize(canvas, SkSize::Make(px_size_), SkSize::Make(size_)));
                canvas.flush();
                SDL_GL_SwapWindow(window_.get());
            }
        }

        void close() { quitting_ = true; }

    private:
        WindowBase* parent_;
        SkISize size_;
        SkISize px_size_;
        std::unique_ptr<SDL_Window, Lambdaified<SDL_DestroyWindow>> window_;
        std::unique_ptr<std::remove_pointer_t<SDL_GLContext>, Lambdaified<SDL_GL_DeleteContext>> gl_ctx_;
        sk_sp<GrDirectContext> skia_ctx_;
        sk_sp<SkSurface> surface_;
        bool quitting_ = false;

        void setup_window(const clu::c_str_view title)
        {
            window_.reset(SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, //
                size_.width(), size_.height(), SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI));
            if (!window_)
                sdl_hard_error();
            gl_ctx_.reset(SDL_GL_CreateContext(window_.get()));
            if (!gl_ctx_)
                sdl_hard_error();
            gladLoadGLLoader(SDL_GL_GetProcAddress);
            if (SDL_GL_MakeCurrent(window_.get(), gl_ctx_.get()))
                sdl_hard_error();
            glEnable(GL_FRAMEBUFFER_SRGB);
        }

        void setup_skia_gl_interface()
        {
            skia_ctx_ = GrDirectContext::MakeGL();
            if (!skia_ctx_)
                hard_error("Failed to create skia GL context");
        }

        void setup_surface()
        {
            int width, height;
            SDL_GetWindowSizeInPixels(window_.get(), &width, &height);
            px_size_ = {width, height};

            GrGLint buffer;
            glGetIntegerv(GL_FRAMEBUFFER_BINDING, &buffer);
            glViewport(0, 0, width, height);
            const GrBackendRenderTarget render_target(width, height, 0, 8, //
                {.fFBOID = static_cast<GrGLuint>(buffer), .fFormat = GL_RGB8});
            surface_ = SkSurface::MakeFromBackendRenderTarget(skia_ctx_.get(), render_target,
                kBottomLeft_GrSurfaceOrigin, kRGB_888x_SkColorType, SkColorSpace::MakeSRGBLinear(), nullptr);
            if (!surface_)
                hard_error("Failed to create skia surface");
        }

        void handle_events()
        {
            SDL_Event ev;
            while (SDL_PollEvent(&ev))
                switch (ev.type)
                {
                    case SDL_WINDOWEVENT:
                    {
                        switch (ev.window.event)
                        {
                            case SDL_WINDOWEVENT_RESIZED:
                                size_ = {ev.window.data1, ev.window.data2};
                                setup_surface();
                                break;
                            default: break;
                        }
                        break;
                    }
                    case SDL_KEYDOWN: parent_->on_key_pressed(ev.key.keysym); break;
                    case SDL_QUIT: quitting_ = true; break;
                    default: break;
                }
        }
    };

    WindowBase::WindowBase(const clu::c_str_view title, const SkISize size):
        impl_(std::make_unique<Impl>(this, title, size))
    {
    }

    WindowBase::~WindowBase() noexcept = default;

    void WindowBase::run() { impl_->run(); }

    void WindowBase::close() { impl_->close(); }
} // namespace slvs
