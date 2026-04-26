add_library(mmv_core STATIC
    mdm/MdmScanner.cpp
    audio/AmbisonicDecoder.cpp
    audio/AudioEngine.cpp
    audio/OpusReader.cpp
    rendering/TextureLoader.cpp
    ${saf_SOURCE_DIR}/examples/src/ambi_bin/ambi_bin.c
    ${saf_SOURCE_DIR}/examples/src/ambi_bin/ambi_bin_internal.c
)

target_include_directories(mmv_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(mmv_core PRIVATE
    ${OPUSFILE_INCLUDE_DIRS}
    ${saf_SOURCE_DIR}/examples/include
    ${saf_SOURCE_DIR}/examples/src/ambi_bin
)

target_link_libraries(mmv_core PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Multimedia
    Qt6::Concurrent
    saf
)
target_link_directories(mmv_core PUBLIC ${OPUSFILE_LIBRARY_DIRS})
target_link_libraries(mmv_core PUBLIC ${OPUSFILE_LIBRARIES})
