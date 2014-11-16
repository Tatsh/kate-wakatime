project(ktexteditor_wakatime)

find_package(KDE4 REQUIRED)
find_package(QJSON)
include(KDE4Defaults)
include_directories(${KDE4_INCLUDES} ${QT_INCLUDES})
add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

set(ktexteditor_wakatime_SRCS wakatimeplugin.cpp)

kde4_add_plugin(ktexteditor_wakatime ${ktexteditor_wakatime_SRCS})

target_link_libraries(ktexteditor_wakatime ${KDE4_KDEUI_LIBS} ${KDE4_KTEXTEDITOR_LIBS} kateinterfaces ${QJSON_LIBRARIES})

install(TARGETS ktexteditor_wakatime DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES ktexteditor_wakatime.desktop DESTINATION ${SERVICES_INSTALL_DIR})
