find_package(Qt6 REQUIRED COMPONENTS Test)

qt_add_executable(MdmScannerTest
    MdmScannerTest.cpp
)

target_link_libraries(MdmScannerTest PRIVATE
    mmv_core
    Qt6::Test
)

add_test(NAME MdmScannerTest COMMAND MdmScannerTest)
