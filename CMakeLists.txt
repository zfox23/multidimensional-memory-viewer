cmake_minimum_required(VERSION 3.25)
project(MultidimensionalMemoryViewer VERSION 0.1.0 LANGUAGES C CXX)

# ── Spatial Audio Framework ──────────────────────────────────────────────────
# SOURCE_SUBDIR skips SAF's top-level CMakeLists (which sets OSX arch flags and
# requires examples/tests) and configures only the framework library directly.
include(FetchContent)
if(NOT DEFINED SAF_PERFORMANCE_LIB)
    if(APPLE)
        set(SAF_PERFORMANCE_LIB "SAF_USE_APPLE_ACCELERATE_LP64")
    else()
        set(SAF_PERFORMANCE_LIB "SAF_USE_OPEN_BLAS_AND_LAPACKE")
    endif()
endif()
FetchContent_Declare(
    SAF
    GIT_REPOSITORY https://github.com/leomccormack/Spatial_Audio_Framework.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  framework
)
FetchContent_MakeAvailable(SAF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.6 REQUIRED COMPONENTS Core Gui Quick QuickControls2 QuickDialogs2 Multimedia Concurrent ShaderTools)
qt_standard_project_setup(REQUIRES 6.6)
qt_policy(SET QTP0004 NEW)

find_package(PkgConfig REQUIRED)
pkg_check_modules(OPUSFILE REQUIRED opusfile)

add_subdirectory(src/core)
add_subdirectory(src/ui)

if(APPLE)
    add_subdirectory(src/platforms/macos)
endif()

qt_add_executable(mmv
    src/main.cpp
)

qt_add_qml_module(mmv
    URI "MMV"
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/ViewerScene.qml
        qml/AppToolBar.qml
        qml/ThumbnailStrip.qml
)

qt_add_shaders(mmv "mmv_shaders"
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    PREFIX "/"
    FILES
        shaders/hemisphere.vert
        shaders/hemisphere.frag
)

target_link_libraries(mmv PRIVATE
    mmv_core
    mmv_ui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::QuickDialogs2
)

if(APPLE)
    target_link_libraries(mmv PRIVATE mmv_platform_macos)
    set_target_properties(mmv PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Multidimensional Memory Viewer"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.zachfox.mmv"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    )
endif()

enable_testing()
add_subdirectory(tests)
