
qt_add_resources(SERVER_RESOURCES_FILE
    ../resources/server/server.qrc
)

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
    ${SERVER_RESOURCES_FILE}
    src/video/VideoSender.cpp
    src/video/VideoSender.h
    src/video/UdpVideoSender.h
    src/video/UdpVideoSender.cpp
    
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
