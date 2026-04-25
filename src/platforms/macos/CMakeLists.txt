add_library(mmv_platform_macos STATIC
    MacPlatform.mm
)

target_include_directories(mmv_platform_macos PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mmv_platform_macos PUBLIC Qt6::Core Qt6::Gui)
target_link_libraries(mmv_platform_macos PRIVATE
    "-framework AppKit"
    "-framework Foundation"
)
