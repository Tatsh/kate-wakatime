cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(ktexteditor_wakatime)

set(QT_MAJOR_VERSION "6")
set(QT_MIN_VERSION "6.5.0")
set(KF5_DEP_VERSION "6.0.0")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_definitions(-DTRANSLATION_DOMAIN=\"katewakatime\")

find_package(ECM ${KF5_DEP_VERSION} REQUIRED NO_MODULE)
list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(ECMInstallIcons)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDEInstallDirs)
include(KDECMakeSettings)

# Sane flags from Kate project
add_definitions(
  -DQT_USE_QSTRINGBUILDER
  -DQT_NO_CAST_TO_ASCII
  -DQT_NO_CAST_FROM_ASCII
  -DQT_STRICT_ITERATORS
  -DQT_NO_URL_CAST_FROM_STRING
  -DQT_NO_CAST_FROM_BYTEARRAY
  -DQT_NO_SIGNALS_SLOTS_KEYWORDS
  -DQT_USE_FAST_OPERATOR_PLUS)

# Needed to find the moc file
include_directories(${CMAKE_CURRENT_BINARY_DIR})

find_package(Qt6Sql REQUIRED)
find_package(KF6 ${KF5_DEP_VERSION} REQUIRED COMPONENTS I18n TextEditor)

set(ktexteditor_wakatime_SRCS wakatimeplugin.cpp offlinequeue.cpp)

ki18n_wrap_ui(ktexteditor_wakatime_SRCS configdialog.ui)
qt6_add_resources(ktexteditor_wakatime_SRCS plugin.qrc)

add_library(ktexteditor_wakatime MODULE ${ktexteditor_wakatime_SRCS})

target_link_libraries(ktexteditor_wakatime KF6::I18n KF6::TextEditor Qt6::Sql)

install(TARGETS ktexteditor_wakatime
        DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)

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

ecm_install_icons(ICONS ${WAKATIME_ICONS} DESTINATION ${KDE_INSTALL_ICONDIR}
                  THEME hicolor)
