add_library(mmv_core STATIC
    mdm/MdmScanner.cpp
    audio/AmbisonicDecoder.cpp
    audio/AudioEngine.cpp
    audio/OpusReader.cpp
    rendering/TextureLoader.cpp
)

target_include_directories(mmv_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(mmv_core PRIVATE ${OPUSFILE_INCLUDE_DIRS})

target_link_libraries(mmv_core PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Multimedia
    Qt6::Concurrent
)
target_link_directories(mmv_core PUBLIC ${OPUSFILE_LIBRARY_DIRS})
target_link_libraries(mmv_core PUBLIC ${OPUSFILE_LIBRARIES})
