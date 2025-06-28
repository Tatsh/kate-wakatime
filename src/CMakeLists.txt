find_package(KF6 ${KF_DEP_VERSION} REQUIRED COMPONENTS I18n TextEditor CoreAddons)

set(ktexteditor_wakatime_SRCS wakatimeplugin.cpp)
ki18n_wrap_ui(ktexteditor_wakatime_SRCS configdialog.ui)
qt6_add_resources(ktexteditor_wakatime_SRCS plugin.qrc)
kcoreaddons_add_plugin(ktexteditor_wakatime INSTALL_NAMESPACE "kf6/ktexteditor" SOURCES
                       ${ktexteditor_wakatime_SRCS})
target_link_libraries(ktexteditor_wakatime KF6::I18n KF6::TextEditor KF6::CoreAddons)

set(WAKATIME_ICONS
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/512-apps-wakatime.png
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/16-apps-wakatime.png
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/22-apps-wakatime.png
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/sc-apps-wakatime.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/128-apps-wakatime.png
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/256-apps-wakatime.png
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/32-apps-wakatime.png
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/48-apps-wakatime.png
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/64-apps-wakatime.png)

ecm_install_icons(ICONS ${WAKATIME_ICONS} DESTINATION ${KDE_INSTALL_ICONDIR} THEME hicolor)

if(BUILD_TESTS)
  enable_testing()
  include(FetchContent)
  FetchContent_Declare(googletest
                       URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip)
  FetchContent_MakeAvailable(googletest)
  add_executable(${CMAKE_PROJECT_NAME}-tests test.cpp)
  target_compile_options(${CMAKE_PROJECT_NAME}-tests PRIVATE ${DEBUG_FLAGS})
  if(CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(${CMAKE_PROJECT_NAME}-tests PRIVATE --coverage)
  endif()
  target_link_libraries(${CMAKE_PROJECT_NAME}-tests PRIVATE name ${SHARED_DEPS} GTest::gtest_main
                                                            GTest::gmock gcov)
  include(GoogleTest)
  gtest_discover_tests(${CMAKE_PROJECT_NAME}-tests)
endif()
