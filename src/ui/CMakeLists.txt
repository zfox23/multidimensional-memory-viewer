add_library(mmv_ui STATIC
    AppController.cpp
    ThumbnailModel.cpp
    MdmImageProvider.cpp
)

target_include_directories(mmv_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mmv_ui PUBLIC
    mmv_core
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Concurrent
)
