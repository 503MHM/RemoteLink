qt_add_library(remotelink_common STATIC
    protocol/MessageType.h
    protocol/Packet.h
    protocol/PacketCodec.h
    protocol/PacketCodec.cpp
    protocol/NetworkConstants.h
    protocol/VideoFramePayload.h
    protocol/VideoFramePayload.cpp
    protocol/FileTransferPayload.h
    protocol/FileTransferPayload.cpp
    protocol/UdpVideoPacket.cpp
    protocol/UdpVideoPacket.h
)

target_link_libraries(remotelink_common
    PUBLIC
        Qt::Core
        Qt::Network
)

target_include_directories(remotelink_common
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)