# Android platform support — placeholder for future implementation
add_library(mmv_platform_android STATIC)
target_include_directories(mmv_platform_android PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mmv_platform_android PUBLIC Qt6::Core Qt6::Gui)
