# CutieCoCo Test Suite

# CPU emulation tests
add_executable(cpu_tests
    cpu_test_harness.cpp
    mc6809_tests.cpp
)

target_link_libraries(cpu_tests PRIVATE
    cutie-emulation
    Catch2::Catch2WithMain
)

# Integration tests (CocoEmulator API)
add_executable(integration_tests
    integration_tests.cpp
)

target_link_libraries(integration_tests PRIVATE
    cutie-emulation
    Catch2::Catch2WithMain
)

# Use Catch2's test discovery
include(Catch)
catch_discover_tests(cpu_tests)
catch_discover_tests(integration_tests)
