add_executable(retrodisc_smoke test_smoke.cpp)
target_link_libraries(retrodisc_smoke PRIVATE retrodisc_core)
add_test(NAME smoke COMMAND retrodisc_smoke)

add_executable(retrodisc_test_player_state test_player_state.cpp)
target_link_libraries(retrodisc_test_player_state PRIVATE retrodisc_core)
add_test(NAME player_state COMMAND retrodisc_test_player_state)

add_executable(retrodisc_test_event_bus test_event_bus.cpp)
target_link_libraries(retrodisc_test_event_bus PRIVATE retrodisc_core)
add_test(NAME event_bus COMMAND retrodisc_test_event_bus)

add_executable(retrodisc_test_mock_cd test_mock_cd.cpp)
target_link_libraries(retrodisc_test_mock_cd PRIVATE retrodisc_core)
add_test(NAME mock_cd COMMAND retrodisc_test_mock_cd)

target_include_directories(retrodisc_smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(retrodisc_test_player_state PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(retrodisc_test_event_bus PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(retrodisc_test_mock_cd PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(retrodisc_test_mock_audio test_mock_audio.cpp)
target_link_libraries(retrodisc_test_mock_audio PRIVATE retrodisc_core)
target_include_directories(retrodisc_test_mock_audio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME mock_audio COMMAND retrodisc_test_mock_audio)

add_executable(retrodisc_test_config test_config.cpp)
target_link_libraries(retrodisc_test_config PRIVATE retrodisc_core)
target_include_directories(retrodisc_test_config PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(retrodisc_test_config PRIVATE
  RETRODISC_CONFIG_DIR="${CMAKE_SOURCE_DIR}/config")
add_test(NAME config COMMAND retrodisc_test_config)

add_executable(retrodisc_test_key_mapping test_key_mapping.cpp)
target_link_libraries(retrodisc_test_key_mapping PRIVATE retrodisc_core)
target_include_directories(retrodisc_test_key_mapping PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME key_mapping COMMAND retrodisc_test_key_mapping)

add_executable(retrodisc_test_render test_render.cpp)
target_link_libraries(retrodisc_test_render PRIVATE retrodisc_core)
target_include_directories(retrodisc_test_render PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME render COMMAND retrodisc_test_render)

add_executable(retrodisc_test_integration test_integration.cpp)
target_link_libraries(retrodisc_test_integration PRIVATE retrodisc_core)
target_include_directories(retrodisc_test_integration PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME integration COMMAND retrodisc_test_integration)

add_executable(retrodisc_test_theme test_theme.cpp)
target_link_libraries(retrodisc_test_theme PRIVATE retrodisc_core)
target_include_directories(retrodisc_test_theme PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME theme COMMAND retrodisc_test_theme)

add_executable(retrodisc_test_brightness test_brightness.cpp)
target_link_libraries(retrodisc_test_brightness PRIVATE retrodisc_core)
target_include_directories(retrodisc_test_brightness PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME brightness COMMAND retrodisc_test_brightness)

add_executable(retrodisc_test_help test_help.cpp)
target_link_libraries(retrodisc_test_help PRIVATE retrodisc_core)
target_include_directories(retrodisc_test_help PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME help COMMAND retrodisc_test_help)

add_executable(retrodisc_test_service test_service.cpp)
target_link_libraries(retrodisc_test_service PRIVATE retrodisc_core)
target_include_directories(retrodisc_test_service PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME service COMMAND retrodisc_test_service)

add_executable(retrodisc_test_modes test_modes.cpp)
target_link_libraries(retrodisc_test_modes PRIVATE retrodisc_core)
target_include_directories(retrodisc_test_modes PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME modes COMMAND retrodisc_test_modes)

add_executable(retrodisc_test_resume test_resume.cpp)
target_link_libraries(retrodisc_test_resume PRIVATE retrodisc_core)
target_include_directories(retrodisc_test_resume PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME resume COMMAND retrodisc_test_resume)

add_executable(retrodisc_test_spectrum test_spectrum.cpp)
target_link_libraries(retrodisc_test_spectrum PRIVATE retrodisc_core)
target_include_directories(retrodisc_test_spectrum PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME spectrum COMMAND retrodisc_test_spectrum)

add_test(NAME system_units
         COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/test_units.sh ${CMAKE_SOURCE_DIR})

add_test(NAME deploy
         COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/test_deploy.sh ${CMAKE_SOURCE_DIR})
add_test(NAME permissions
         COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/test_permissions.sh ${CMAKE_SOURCE_DIR})

add_test(NAME cdda_spike
         COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/test_spike.sh ${CMAKE_SOURCE_DIR})

add_test(NAME hw_survey
         COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/test_survey.sh ${CMAKE_SOURCE_DIR})

add_executable(retrodisc_test_alsa test_alsa.cpp)
target_link_libraries(retrodisc_test_alsa PRIVATE retrodisc_core)
target_include_directories(retrodisc_test_alsa PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME alsa COMMAND retrodisc_test_alsa)

add_test(NAME power_storage
         COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/test_power.sh ${CMAKE_SOURCE_DIR})
