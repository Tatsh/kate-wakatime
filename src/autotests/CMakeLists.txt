include(ECMMarkAsTest)

set(ktexteditor_wakatime_tests_SRCS ../wakatimeplugin.cpp ../wakatimeplugin.h basic.cpp)
ki18n_wrap_ui(ktexteditor_wakatime_tests_SRCS ../configdialog.ui)
qt6_add_resources(ktexteditor_wakatime_tests_SRCS ../plugin.qrc)

add_executable(test_wakatime ${ktexteditor_wakatime_tests_SRCS})
target_include_directories(test_wakatime PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_compile_options(test_wakatime PRIVATE -ggdb)
if(COVERAGE)
  target_compile_options(test_wakatime PRIVATE --coverage)
  target_link_libraries(test_wakatime PRIVATE gcov)
endif()

find_package(Qt6Test ${QT_MIN_VERSION} QUIET REQUIRED)

target_link_libraries(test_wakatime PRIVATE Qt6::Test KF6::I18n KF6::TextEditor KF6::CoreAddons)

add_test(NAME kate-test_wakatime COMMAND test_wakatime ${OFFSCREEN_QPA})
ecm_mark_as_test(test_wakatime)
