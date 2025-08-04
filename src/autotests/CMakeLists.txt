include(ECMMarkAsTest)

set(ktexteditor_wakatime_tests_SRCS basic.cpp)

add_executable(test_wakatime ${ktexteditor_wakatime_tests_SRCS})
target_include_directories(test_wakatime PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
if(COVERAGE AND CMAKE_COMPILER_IS_GNUCXX)
  target_compile_options(test_wakatime PRIVATE --coverage)
  target_link_libraries(test_wakatime PRIVATE gcov)
endif()

find_package(Qt6Test ${QT_MIN_VERSION} QUIET REQUIRED)

target_link_libraries(test_wakatime PRIVATE Qt6::Test)

set(OFFSCREEN_QPA)
if(NOT WIN32)
  set(OFFSCREEN_QPA "-platform" "offscreen")
endif()

add_test(NAME kate-test_wakatime COMMAND test_wakatime ${OFFSCREEN_QPA})
ecm_mark_as_test(test_wakatime)
