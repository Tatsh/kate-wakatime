project(ktexteditor_wakatime)

cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

add_definitions(-DTRANSLATION_DOMAIN=\"katewakatime\")

find_package(ECM 1.1.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDEInstallDirs)
include(KDECMakeSettings)

# Sane flags from Kate project
add_definitions(-DQT_USE_QSTRINGBUILDER
                -DQT_NO_CAST_TO_ASCII
                -DQT_NO_CAST_FROM_ASCII
                -DQT_STRICT_ITERATORS
                -DQT_NO_URL_CAST_FROM_STRING
                -DQT_NO_CAST_FROM_BYTEARRAY
                -DQT_NO_SIGNALS_SLOTS_KEYWORDS
                -DQT_USE_FAST_OPERATOR_PLUS)

# Needed to find the moc file
include_directories(${CMAKE_CURRENT_BINARY_DIR})

find_package(KF5TextEditor NO_MODULE)

set(ktexteditor_wakatime_SRCS wakatimeplugin.cpp)

ki18n_wrap_ui(ktexteditor_wakatime_SRCS configdialog.ui)
qt5_add_resources(ktexteditor_wakatime_SRCS plugin.qrc)

add_library(ktexteditor_wakatime MODULE ${ktexteditor_wakatime_SRCS})

kcoreaddons_desktop_to_json(ktexteditor_wakatime ktexteditor_wakatime.desktop
                            DEFAULT_SERVICE_TYPE)

target_link_libraries(ktexteditor_wakatime KF5::TextEditor)

install(TARGETS ktexteditor_wakatime
        DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)
