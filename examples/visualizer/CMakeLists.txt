set(TGT_NAME "solitude-visualizer")

add_executable(${TGT_NAME})
target_sources(${TGT_NAME} PRIVATE
	"main.cpp"
	"elements/board.h"
	"elements/board.cpp"
	"elements/constants.h"
	"elements/constants.cpp"
	"elements/style.h"
	"elements/style.cpp"
	"gui/canvas_view.h"
	"gui/canvas_view.cpp"
	"gui/color_literals.h"
	"gui/window.h"
	"gui/window.cpp"
)
target_set_output_dirs(${TGT_NAME})
target_set_cxx_std(${TGT_NAME})

# Get glad with FetchContent.
# vcpkg only provides compat profile and making an overlay port for glad is cumbersome.
# Since this is an application, not a library, using FetchContent should be fine.
include(FetchContent)
FetchContent_Declare(glad GIT_REPOSITORY https://github.com/Dav1dde/glad.git)
FetchContent_GetProperties(glad)
if (NOT glad_POPULATED)
    FetchContent_Populate(glad)
    set(GLAD_PROFILE "core")
    set(GLAD_API "gl=3.2")
    set(GLAD_GENERATOR "c")
    add_subdirectory(${glad_SOURCE_DIR} ${glad_BINARY_DIR})
endif ()

find_package(SDL2 CONFIG REQUIRED)
find_package(unofficial-skia CONFIG REQUIRED)
target_link_libraries(${TGT_NAME} PRIVATE
	project_options
	solitude::solitude
	glad
	$<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
	unofficial::skia::skia
)
