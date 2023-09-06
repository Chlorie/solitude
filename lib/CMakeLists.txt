add_library(solitude)
add_library(solitude::solitude ALIAS solitude)

set_target_properties(solitude PROPERTIES LINKER_LANGUAGE CXX)
target_set_output_dirs(solitude)

function (target_add_sources TGT PREFIX)
    set(srcs ${ARGN})
    list(TRANSFORM srcs PREPEND ${PREFIX})
    target_sources(${TGT} PRIVATE ${srcs})
endfunction ()

target_add_sources(solitude "include/solitude/"
    # Header files here
    "bitset.h"
    "board.h"
    "export.h"
    "generator.h"
    "strategies.h"
    "utils.h"
    "strategies/chains.h"
    "strategies/coloring.h"
    "strategies/fish.h"
    "strategies/intersection.h"
    "strategies/subsets.h"
    "strategies/sue_de_coq.h"
    "strategies/uniqueness.h"
    "strategies/wings.h"
)

target_add_sources(solitude "src/"
    # Source files here (relative to ./src/)
    "board.cpp"
    "generator.cpp"
    "utils.cpp"
    "strategies/chains.cpp"
    "strategies/coloring.cpp"
    "strategies/common.h"
    "strategies/common.cpp"
    "strategies/fish.cpp"
    "strategies/intersection.cpp"
    "strategies/subsets.cpp"
    "strategies/sue_de_coq.cpp"
    "strategies/uniqueness.cpp"
    "strategies/wings.cpp"
)

find_package(clu CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

target_set_options(solitude PRIVATE)
target_set_warnings(solitude PRIVATE)
target_set_cxx_std(solitude)

if (BUILD_SHARED_LIBS)
    target_compile_definitions(solitude
        PRIVATE SOLITUDE_EXPORT_SHARED
        PUBLIC SOLITUDE_BUILD_SHARED)
endif ()

target_include_directories(solitude
PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(solitude
PUBLIC
    clu::clu    
    fmt::fmt
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/solitudeConfigVersion.cmake"
    VERSION ${PACKAGE_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(TARGETS solitude
    EXPORT solitudeTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/solitudeConfig.cmake.in"
    "${PROJECT_BINARY_DIR}/solitudeConfig.cmake"
    INSTALL_DESTINATION lib/cmake/solitude
)

export(TARGETS solitude NAMESPACE solitude:: FILE solitudeConfig.cmake)
install(EXPORT solitudeTargets NAMESPACE solitude:: DESTINATION lib/cmake/solitude)
install(FILES "${PROJECT_BINARY_DIR}/solitudeConfigVersion.cmake"
                "${PROJECT_BINARY_DIR}/solitudeConfig.cmake"
        DESTINATION lib/cmake/solitude)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/lib/include/ DESTINATION include)
