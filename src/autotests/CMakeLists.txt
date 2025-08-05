include(ECMMarkAsTest)

find_package(Qt6Test ${QT_MIN_VERSION} QUIET REQUIRED)

set(kate_wakatime_client_tests_SRCS clienttest.cpp ../wakatime.h ../wakatime.cpp)
set(kate_wakatime_config_tests_SRCS configtest.cpp ../wakatimeconfig.cpp ../wakatimeconfig.h)

function(create_test test_name test_srcs)
  add_executable(${test_name} ${test_srcs})
  target_compile_definitions(${test_name} PRIVATE TESTING)
  target_include_directories(${test_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
  if(COVERAGE AND CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(${test_name} PRIVATE --coverage)
    target_link_libraries(${test_name} PRIVATE gcov)
  endif()
  target_link_libraries(${test_name} PRIVATE Qt6::Test)
  add_test(NAME ${test_name} COMMAND ${test_name})
  ecm_mark_as_test(${test_name})
endfunction()

create_test(kate-wakatime-client-test "${kate_wakatime_client_tests_SRCS}")
create_test(kate-wakatime-config-test "${kate_wakatime_config_tests_SRCS}")
target_link_libraries(kate-wakatime-config-test PRIVATE KF6::I18n KF6::TextEditor)
