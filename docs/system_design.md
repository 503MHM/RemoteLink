# 系统整体架构设计文档

## 1. 项目概述

RemoteLink 是一个基于 Qt 的局域网远程控制项目，分为 client 和 server 两个程序。

- server 运行在被控制端，负责局域网发现响应、TCP 服务监听、屏幕采集、远程鼠标键盘执行、文件接收。
- client 运行在控制端，负责发现 server、建立 TCP 连接、接收 UDP 画面、发送鼠标键盘事件、发送文件。
- common 模块提供 TCP 自定义包格式、UDP 视频分片格式、端口常量，以及视频和文件 payload 定义。

当前项目主要面向 Windows。server 端屏幕采集使用 DXGI Desktop Duplication，鼠标键盘控制使用 Windows API。

## 2. 目录结构

```text
RemoteLink
├── client
│   ├── main.cpp
│   └── src
│       ├── decode              # JPEG 帧解码 worker
│       ├── file                # client 文件发送
│       ├── network             # TCP 会话、UDP 发现
│       └── ui                  # client 主窗口、远程画面控件
├── server
│   ├── main.cpp
│   └── src
│       ├── capture             # DXGI 截图、截图 worker
│       ├── control             # 鼠标键盘控制
│       ├── file                # server 文件接收
│       ├── network             # TCP server、UDP 发现响应、server worker
│       └── ui                  # server 主窗口
├── common
│   └── protocol                # 自定义协议、视频/文件 payload 头
├── resources                   # QSS、图标资源
└── docs                        # 项目文档
```

## 3. 进程角色

### 3.1 server

server 是被控制端。它启动服务后：

1. 通过 UDP 监听局域网发现请求。
2. 通过 TCP 监听 client 连接。
3. 收到 client 心跳后启动屏幕采集。
4. 将屏幕 JPEG 帧通过 UDP 分片发送给 client。
5. 接收 client 发来的鼠标、键盘、文件消息。
6. 将接收到的文件保存到 server 程序目录下的 `downloads` 目录。

### 3.2 client

client 是控制端。它启动后：

1. 通过 UDP 广播搜索 server。
2. 用户选择设备并建立 TCP 连接。
3. 连接成功后发送心跳。
4. 接收 server 的屏幕帧，解码并显示。
5. 将远程画面控件上的鼠标键盘输入发送给 server。
6. 支持选择本地文件发送给 server。

## 4. 核心模块

### 4.1 common/protocol

common 模块定义了 TCP 自定义协议。

- `MessageType`：消息类型枚举。
- `Packet`：统一 TCP 数据包结构。
- `PacketCodec`：负责粘包、拆包、编码、解码。
- `NetworkConstants`：默认 TCP/UDP 端口。
- `VideoFramePayload`：完整视频帧的头部与 JPEG 数据结构。
- `UdpVideoPacket`：视频 UDP 数据报的分片头及编解码函数。
- `FileTransferPayload`：`FileData` 的文件分片 payload 结构。

所有 TCP 业务消息都经过 `PacketCodec` 编码和解码。UDP 发现消息不走 `PacketCodec`，而是直接发送 JSON。

### 4.2 server UI 模块

server 主窗口负责：

- 启动/停止服务。
- 显示本机名称、本机 IP、连接状态、连接时长。
- 显示文件接收进度。
- 显示运行日志。

UI 线程不直接运行 TCP 服务。`MainWindow` 会创建 `ServerWorker`，并将它移动到独立的 `serverThread`。

### 4.3 server 网络模块

server 网络模块主要由三个类组成：

- `DiscoveryResponder`：UDP 发现响应，监听 client 的发现广播并回复 server 信息。
- `ServerWorker`：运行在 server 后台线程中，负责创建和管理 `RemoteServer`。
- `RemoteServer`：TCP 服务对象，负责连接、心跳、控制和文件消息；同时配置 UDP 视频目标地址。

当前 server 限制为单 client 连接。如果已经有 client 连接，新的连接会被拒绝。

### 4.4 server 截图模块

server 截图模块包括：

- `ScreenCapture`：基于 DXGI Desktop Duplication 获取 `QImage`，并提供 JPEG 编码。
- `CaptureWorker`：运行在截图线程中，通过 `QTimer` 定时采集和编码，并每秒统计采集/编码耗时和 JPEG 大小。

client 连接后发送心跳，server 收到心跳后启动截图定时器。当前截图间隔为 16 ms，理论目标接近 60 FPS，但实际帧率受采集、JPEG 编码、网络和 socket 缓冲影响。

视频帧经 `VideoFrameBuilder` 添加帧头，再由 `UdpVideoSender` 分片：

```text
UDP datagrams
└── VideoFrameHeader + JPEG bytes
```

`VideoFrameBuilder` 负责帧号和帧头，`UdpVideoSender` 负责分片发送，并每秒输出发送帧数、数据报数、字节吞吐和发送失败数。

### 4.5 server 控制模块

server 控制模块负责执行 client 发来的输入事件：

- `MouseController`：处理鼠标移动、按下、释放、滚轮。
- `KeyboardController`：处理键盘按下和释放。

client 发送的是相对坐标比例，server 根据自己的屏幕尺寸换算为实际坐标。

### 4.6 server 文件接收模块

`FileReceiver` 运行在 server TCP 线程中，处理三类文件消息：

- `FileBegin`：创建保存文件，初始化文件 ID、文件大小、接收进度。
- `FileData`：解析 `FileDataHeader` 后写入文件块。
- `FileEnd`：校验文件 ID 和总大小，关闭文件。

文件默认保存到：

```text
server 程序目录/downloads
```

### 4.7 client UI 模块

client 主窗口负责：

- 搜索 server。
- 建立/断开 TCP 连接。
- 显示远程画面。
- 显示连接状态、FPS、文件发送进度。
- 选择并发送文件。

远程画面控件是 `RemoteViewWidget`。它继承自 `QLabel`，负责捕获鼠标和键盘事件，并将本地控件坐标转换为相对比例。

### 4.8 client 网络模块

client 网络模块包括：

- `DiscoveryClient`：UDP 广播搜索 server。
- `RemoteSession`：TCP client 会话，负责连接 server、心跳和发送控制/文件消息。
- `UdpVideoReceiver`：绑定视频 UDP 端口，校验会话并重组最新完整帧。
- `VideoFrameParser`：解析重组后的完整视频 payload，提取 JPEG。

`RemoteSession`、`UdpVideoReceiver` 和 `VideoFrameParser` 当前运行在 UI 线程中。JPEG 数据随后交给独立解码线程。

### 4.9 client 解码模块

client 使用独立线程解码 JPEG：

- `FrameDecodeWorker` 运行在 `frameDecodeThread` 中。
- UI 收到 JPEG 数据后，通过 queued invoke 调用 `decodeFrame()`。
- 解码完成后发送 `frameDecoded(QImage)` 回 UI 线程显示。

client 做了“只保留最新帧”的处理：

```text
如果解码线程忙，新来的 JPEG 覆盖 pendingJpegData
解码完成后只继续处理最新 pending JPEG
```

这样可以避免 client 解码队列堆积大量过期帧。

### 4.10 client 文件发送模块

client 文件发送由两个类组成：

- `FileTransferClient`：运行在 UI 线程中，协调 socket 发送和 worker 生命周期。
- `FileTransferWorker`：运行在文件发送线程中，负责读取本地文件块。

文件发送流程：

1. worker 打开文件并发出 `fileBeginReady`。
2. `FileTransferClient` 通过 `RemoteSession::sendPacket()` 发送 `FileBegin`。
3. worker 每次读取 256 KB 文件块。
4. worker 将 `FileDataHeader + chunk` 编成 payload。
5. `FileTransferClient` 发送 `FileData`。
6. worker 读到文件末尾后发出 `fileEndReady`。
7. `FileTransferClient` 发送 `FileEnd`。

当前文件发送有水位控制：

- 高水位：2 MB。socket 写缓冲超过高水位时暂停读取新块。
- 低水位：512 KB。`RemoteSession` 检测到写缓冲低于低水位后，通知 worker 继续读取。

## 5. 多线程模型

### 5.1 server 线程

```text
server 进程
├── UI 主线程
│   ├── MainWindow
│   └── DiscoveryResponder
│
└── serverThread
    ├── ServerWorker
    ├── RemoteServer
    ├── QTcpServer / QTcpSocket
    ├── FileReceiver
    └── captureThread
        └── CaptureWorker + ScreenCapture + QTimer
```

说明：

- UI 主线程只负责界面和 UDP 发现响应。
- TCP 监听、控制收包、文件接收和 UDP 视频发送运行在 `serverThread`。
- 屏幕采集运行在 `captureThread`。
- `CaptureWorker::frameReady` 跨线程回到 `RemoteServer` 后，由 `UdpVideoSender` 发送视频分片。

### 5.2 client 线程

```text
client 进程
├── UI 主线程
│   ├── MainWindow
│   ├── RemoteSession + QTcpSocket
│   ├── DiscoveryClient
│   └── FileTransferClient
│
├── frameDecodeThread
│   └── FrameDecodeWorker
│
└── fileTransferWorkerThread
    └── FileTransferWorker
```

说明：

- client 的 TCP socket 当前在 UI 线程。
- JPEG 解码放在独立线程。
- 文件读取放在独立线程。
- socket 写入仍由 `RemoteSession` 在 UI 线程完成。
- 文件 worker 不直接访问 socket，而是通过信号通知主线程发送。

## 6. 主要数据流

### 6.1 设备发现

```text
client DiscoveryClient
    └── UDP broadcast DISCOVERY
server DiscoveryResponder
    └── UDP reply DISCOVERY_REPLY
client MainWindow
    └── 将设备加入下拉框
```

### 6.2 远程画面

```text
client RemoteSession
    └── TCP Heartbeat
server RemoteServer
    └── startCaptureRequested
server CaptureWorker
    └── DXGI capture + JPEG encode
server RemoteServer
    └── VideoFrameBuilder
        └── UdpVideoSender 分片
client UdpVideoReceiver
    └── VideoFrameParser
client FrameDecodeWorker
    └── JPEG decode
client MainWindow
    └── QLabel 显示 QImage/QPixmap
```

### 6.3 鼠标键盘控制

```text
client RemoteViewWidget
    └── 捕获鼠标键盘事件
client RemoteSession
    └── TCP MouseMove / MousePress / MouseRelease / MouseWheel / KeyEvent
server RemoteServer
    └── PacketCodec 解包
server MouseController / KeyboardController
    └── Windows API 执行输入
```

### 6.4 文件传输

```text
client FileTransferWorker
    └── 读取文件块
client FileTransferClient
    └── 通过 RemoteSession 发送 FileBegin / FileData / FileEnd
server RemoteServer
    └── 分发到 FileReceiver
server FileReceiver
    └── 写入 downloads 目录
```

## 7. 当前设计特点

- server 将 UI 线程和 TCP 服务线程分离，避免高频网络和文件写入影响界面。
- server 将屏幕采集单独放入截图线程，避免 DXGI/JPEG 编码阻塞 TCP 事件循环。
- client 将 JPEG 解码放入独立线程，降低 UI 卡顿概率。
- client 解码侧只保留最新帧，避免过期画面堆积。
- 文件发送支持 socket 写缓冲水位控制，避免一次性塞入过多文件数据。
- TCP 使用统一包头解决粘包和半包问题，文件消息携带文件 payload 头；视频使用独立 UDP 分片头。

## 8. 已知限制和后续优化方向

- server 目前只支持单 client 连接。
- TCP 协议层已加入最大 payload 长度限制，当前上限为 4 MB。
- server 文件接收当前运行在 TCP 线程，大文件写盘较慢时可能影响控制事件处理。
- 屏幕传输当前是整屏 JPEG over UDP，尚未使用 DXGI dirty rects 和 move rects。
- client 的 TCP socket 当前仍在 UI 线程。当前重活已拆到解码线程和文件线程，后续如果 UI 收包压力变大，可考虑将 `RemoteSession` 移到独立网络线程。
- 当前控制、心跳和文件传输共用 TCP，屏幕帧已经通过独立 UDP 通道传输。
- 当前控制和截图实现主要面向 Windows。

## 9. 重构进度

当前按“协议定义 -> 通道拆分 -> UDP 视频 -> 工程化 -> 编码升级 -> 界面完善”的顺序推进。

| 阶段 | 状态 | 说明 |
| --- | --- | --- |
| 1. 协议收口 | 已完成定义层和编解码层 | TCP 包头包含 `sessionId` 和长度限制；消息类型按实际 TCP 业务保留；视频和文件 payload 各自定义结构。`ProtocolErrorCode` 已定义，但还没有统一映射到所有业务错误路径。 |
| 2. 通道拆分 | 已完成 | TCP 用于心跳、输入控制和文件；UDP 用于发现和视频。 |
| 3. UDP 视频 | 已完成基础实现 | JPEG over UDP，支持分片、会话校验、重组、超时/新帧替换和最新帧策略。当前无 ACK、重传或 FEC。 |
| 4. 工程化 | 进行中 | 已有自动重连、分阶段性能日志和文件吞吐统计；配对码/认证未实现，网络丢包率尚无可靠端到端统计。 |
| 5. 编码升级 | 未开始 | 后续评估 FFmpeg H.264；不把编码升级作为当前必需项。 |
| 6. 界面完善 | 未开始 | 当前沿用 Widgets，性能和日志主要通过现有状态显示及运行日志输出。 |

因此当前项目完成到第 3 阶段，正在补第 4 阶段工程化能力。下一阶段优先完善 UDP 发送错误诊断、端到端丢包/积压统计和认证，再评估编码与界面工作。
