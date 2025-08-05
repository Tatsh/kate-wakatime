find_package(KF6 ${KF_DEP_VERSION} REQUIRED COMPONENTS I18n TextEditor CoreAddons)

set(ktexteditor_wakatime_SRCS wakatimeconfig.cpp wakatimeconfig.h wakatimeplugin.cpp
                              wakatimeplugin.h wakatime.cpp wakatime.h)
ki18n_wrap_ui(ktexteditor_wakatime_SRCS configdialog.ui)
qt6_add_resources(ktexteditor_wakatime_SRCS plugin.qrc)
kcoreaddons_add_plugin(ktexteditor_wakatime INSTALL_NAMESPACE "kf6/ktexteditor" SOURCES
                       ${ktexteditor_wakatime_SRCS})
target_link_libraries(ktexteditor_wakatime KF6::I18n KF6::TextEditor KF6::CoreAddons)

set(WAKATIME_ICONS
    ${CMAKE_CURRENT_SOURCE_DIR}/../icons/16-apps-wakatime.png
    ${CMAKE_CURRENT_SOURCE_DIR}/../icons/22-apps-wakatime.png
    ${CMAKE_CURRENT_SOURCE_DIR}/../icons/32-apps-wakatime.png
    ${CMAKE_CURRENT_SOURCE_DIR}/../icons/48-apps-wakatime.png
    ${CMAKE_CURRENT_SOURCE_DIR}/../icons/64-apps-wakatime.png
    ${CMAKE_CURRENT_SOURCE_DIR}/../icons/128-apps-wakatime.png
    ${CMAKE_CURRENT_SOURCE_DIR}/../icons/256-apps-wakatime.png
    ${CMAKE_CURRENT_SOURCE_DIR}/../icons/512-apps-wakatime.png
    ${CMAKE_CURRENT_SOURCE_DIR}/../icons/sc-apps-wakatime.svg)

ecm_install_icons(ICONS ${WAKATIME_ICONS} DESTINATION ${KDE_INSTALL_ICONDIR} THEME hicolor)

if(BUILD_TESTING)
  enable_testing()
  add_subdirectory(autotests)
endif()
