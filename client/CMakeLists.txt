
qt_add_resources(RESOURCES_FILE
    ../resources/client/client.qrc
)

qt_add_executable(client
    main.cpp
    src/network/RemoteSession.cpp
    src/network/RemoteSession.h
    src/network/DiscoveryClient.cpp
    src/network/DiscoveryClient.h

    src/ui/mainwindow.cpp
    src/ui/mainwindow.h
    src/ui/mainwindow.ui
    src/ui/RemoteViewWidget.cpp
    src/ui/RemoteViewWidget.h
    
    src/file/FileTransferClient.cpp
    src/file/FileTransferClient.h
    src/file/FileTransferWorker.h
    src/file/FileTransferWorker.cpp
    
    src/decode/FrameDecodeWorker.h
    src/decode/FrameDecodeWorker.cpp

    src/video/VideoFrameParser.h
    src/video/VideoFrameParser.cpp

    ${RESOURCES_FILE}
    src/video/UdpVideoReceiver.h
    src/video/UdpVideoReceiver.cpp

)

target_link_libraries(client
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Network
        remotelink_common
)

target_include_directories(client
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
