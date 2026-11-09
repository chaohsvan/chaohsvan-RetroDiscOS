add_executable(retrodisc_smoke test_smoke.cpp)
target_link_libraries(retrodisc_smoke PRIVATE retrodisc_core)
target_include_directories(retrodisc_smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME smoke COMMAND retrodisc_smoke)
