cmake_minimum_required(VERSION 3.31)

project(
  ktexteditor_wakatime
  VERSION 1.3.10
  DESCRIPTION "Kate plugin to interface with WakaTime."
  HOMEPAGE_URL https://tatsh.github.io/kate-wakatime/)

set(QT_MAJOR_VERSION "6")
set(QT_MIN_VERSION "6.5.0")
set(KF_DEP_VERSION "6.0.0")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_definitions(-DTRANSLATION_DOMAIN=\"katewakatime\")

find_package(ECM ${KF_DEP_VERSION} REQUIRED NO_MODULE)
list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs6)
include(KDECMakeSettings)
include(ECMInstallIcons)
include(KDECompilerSettings NO_POLICY_SCOPE)

# Sane flags from Kate project
add_definitions(
  -DQT_NO_CAST_FROM_ASCII
  -DQT_NO_CAST_FROM_BYTEARRAY
  -DQT_NO_CAST_TO_ASCII
  -DQT_NO_SIGNALS_SLOTS_KEYWORDS
  -DQT_NO_URL_CAST_FROM_STRING
  -DQT_STRICT_ITERATORS
  -DQT_USE_FAST_OPERATOR_PLUS
  -DQT_USE_QSTRINGBUILDER
  -DVERSION=\"${CMAKE_PROJECT_VERSION}\")

# Needed to find the moc file.
include_directories(${CMAKE_CURRENT_BINARY_DIR})

find_package(Qt6Sql REQUIRED)
find_package(KF6 ${KF_DEP_VERSION} REQUIRED COMPONENTS I18n TextEditor CoreAddons)

set(ktexteditor_wakatime_SRCS wakatimeplugin.cpp)
ki18n_wrap_ui(ktexteditor_wakatime_SRCS configdialog.ui)
qt6_add_resources(ktexteditor_wakatime_SRCS plugin.qrc)
kcoreaddons_add_plugin(ktexteditor_wakatime INSTALL_NAMESPACE "kf6/ktexteditor" SOURCES
                       ${ktexteditor_wakatime_SRCS})
target_link_libraries(ktexteditor_wakatime KF6::I18n KF6::TextEditor KF6::CoreAddons Qt6::Sql)

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
