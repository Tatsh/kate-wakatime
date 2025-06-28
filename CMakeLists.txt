cmake_minimum_required(VERSION 3.31)

project(
  ktexteditor_wakatime
  VERSION 1.5.1
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

# Sane flags from Kate project.
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

add_subdirectory(src)
