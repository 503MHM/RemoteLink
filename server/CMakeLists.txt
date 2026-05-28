#[[cmake_minimum_required(VERSION 3.19)
project(server LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(server
    #WIN32 MACOSX_BUNDLE
    main.cpp
    src/ui/mainwindow.cpp
    src/ui/mainwindow.h
    src/ui/mainwindow.ui
)

target_link_libraries(server
    PRIVATE
        Qt::Core
        Qt::Widgets
)

install(TARGETS server
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET server
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
]]


qt_add_executable(server
    main.cpp
    src/network/RemoteServer.cpp
    src/network/RemoteServer.h
    src/ui/mainwindow.cpp
    src/ui/mainwindow.h
    src/ui/mainwindow.ui
    src/network/DiscoveryResponder.h
    src/network/DiscoveryResponder.cpp
    src/capture/ScreenCapture.cpp
    src/capture/ScreenCapture.h
    src/control/MouseController.cpp
    src/control/MouseController.h
    src/file/FileReceiver.h
    src/file/FileReceiver.cpp
    src/capture/CaptureWorker.h
    src/capture/CaptureWorker.cpp
    src/control/KeyboardController.h
    src/control/KeyboardController.cpp
    src/network/ServerWorker.h
    src/network/ServerWorker.cpp
    
)

target_link_libraries(server
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Network
        remotelink_common
        d3d11
        dxgi
)

target_include_directories(server
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
