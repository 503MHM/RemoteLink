# RemoteLink TCP/UDP 通信协议说明

## 1. 协议概述

RemoteLink 使用两类通信方式：

- UDP：用于局域网设备发现。
- TCP：用于远程画面传输、鼠标键盘控制、文件传输、心跳保活和主动断开通知。

UDP 发现消息直接使用 JSON。TCP 业务消息使用自定义二进制包格式，由 `PacketCodec` 负责编码和解码。

默认端口：

| 名称 | 默认值 | 说明 |
| --- | ---: | --- |
| TCP 服务端口 | 45454 | client 连接 server 的 TCP 端口 |
| UDP 发现端口 | 45455 | client 广播发现、server 监听发现请求 |

## 2. UDP 发现协议

### 2.1 client 发现请求

client 向局域网广播 JSON：

```json
{
  "type": "DISCOVERY",
  "clientName": "RemoteLink Client"
}
```

发送目标端口为 `DiscoveryPort`，当前默认是 `45455`。

### 2.2 server 发现响应

server 收到发现请求后，向请求来源地址和端口回复 JSON：

```json
{
  "type": "DISCOVERY_REPLY",
  "deviceName": "server-host-name",
  "tcpPort": 45454,
  "version": "1.0"
}
```

client 收到响应后，将设备名称、IP、TCP 端口加入设备列表。

## 3. TCP 包格式

TCP 是字节流协议，可能出现粘包、半包和数据错位。RemoteLink 定义固定包头来划分业务消息。

所有 TCP 消息格式如下：

```text
+------------+---------+----------+------------+---------+
| magic      | type    | sessionId | length     | sequence | payload |
+------------+---------+----------+------------+----------+---------+
| quint32    | quint16 | quint32  | quint32    | quint32  | bytes   |
+------------+---------+----------+------------+---------+
```

字段说明：

| 字段 | 类型 | 字节数 | 说明 |
| --- | --- | ---: | --- |
| magic | quint32 | 4 | 固定魔数，值为 `0x524C4B31`，ASCII 含义是 `RLK1` |
| type | quint16 | 2 | 消息类型，对应 `MessageType` |
| sessionId | quint32 | 4 | 会话标识，server 新连接后生成，client 连接后通过心跳响应获取 |
| length | quint32 | 4 | payload 字节数 |
| sequence | quint32 | 4 | 包序号 |
| payload | bytes | length | 具体业务数据 |

包头总长度为 18 字节。

当前编码使用 `QDataStream::BigEndian`，因此包头字段按大端字节序写入。

## 4. TCP 消息类型

| 类型名 | 数值 | 方向 | 说明 |
| --- | ---: | --- | --- |
| Discovery | 101 | 未用于 TCP | 发现类型枚举保留 |
| DiscoveryReply | 102 | 未用于 TCP | 发现响应枚举保留 |
| FrameData | 301 | server -> client | 远程屏幕视频帧，payload 内含视频帧头和 JPEG 数据 |
| MouseMove | 401 | client -> server | 鼠标移动 |
| MouseClick | 402 | 保留 | 当前未使用 |
| MouseWheel | 403 | client -> server | 鼠标滚轮 |
| MousePress | 404 | client -> server | 鼠标按下 |
| MouseRelease | 405 | client -> server | 鼠标释放 |
| FileBegin | 501 | client -> server | 文件传输开始 |
| FileData | 502 | client -> server | 文件分片数据，payload 内含分片头和 chunk |
| FileEnd | 503 | client -> server | 文件传输结束 |
| Heartbeat | 201 | client -> server | 客户端心跳请求 |
| HeartbeatAck | 202 | server -> client | 服务端心跳响应 |
| KeyEvent | 406 | client -> server | 键盘事件 |
| ServerKick | 601 | server -> client | 服务端主动断开通知，client 收到后不自动重连 |

## 5. 连接保活与重连

### 5.1 心跳流程

client 连接成功后会立即发送一次 `Heartbeat`，之后每 5 秒发送一次。首个心跳的 `sessionId` 可能为 0，server 会在响应里返回当前会话的 `sessionId`，client 收到后保存。

server 收到 `Heartbeat` 后：

1. 如果当前还没有启动屏幕采集，则启动采集线程。
2. 回复 `HeartbeatAck`。

client 收到 `HeartbeatAck` 后刷新最后响应时间。如果超过约 15 秒没有收到心跳响应，则认为连接异常，主动断开 socket 并进入自动重连流程。

### 5.2 自动重连

自动重连由 client 负责，server 只负责持续监听 TCP 端口。

当前策略：

```text
1. client 连接 server 成功。
2. client 定时发送 Heartbeat。
3. 超过 15 秒未收到 HeartbeatAck 时认为连接异常。
4. client 断开当前 socket。
5. 如果不是手动断开，则每 3 秒尝试重连一次。
6. 最多重连 5 次。
7. 重连成功后停止重连定时器，重置重连次数，并重新启动心跳。
```

### 5.3 主动断开通知

server 主动断开客户端或停止服务时，会先发送 `ServerKick`，再断开 socket。

client 收到 `ServerKick` 后：

1. 设置本地手动断开标记。
2. 停止心跳定时器。
3. 停止重连定时器。
4. 断开当前 socket。

这样可以区分：

| 场景 | 是否自动重连 |
| --- | --- |
| 网络异常断开 | 是 |
| 心跳超时 | 是 |
| client 用户主动断开 | 否 |
| server 主动断开连接 | 否 |
| server 停止服务 | 否 |

## 6. 各消息 payload 格式

### 6.1 Heartbeat

方向：

```text
client -> server
```

payload 当前是普通字节串：

```text
hello from client
```

server 收到后回复 `HeartbeatAck`。

### 6.2 HeartbeatAck

方向：

```text
server -> client
```

payload 当前为空：

```text
empty
```

payload 中带有 server 当前会话的 `sessionId`。client 收到后刷新最后心跳响应时间，并保存 `sessionId`。

### 6.3 ServerKick

方向：

```text
server -> client
```

payload 当前为空：

```text
empty
```

该消息表示 server 主动要求断开连接。client 收到后不会自动重连。

### 6.4 FrameData

方向：

```text
server -> client
```

payload 结构如下：

```text
VideoFrameHeader + JPEG binary bytes
```

`VideoFrameHeader` 当前字段如下：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| frameId | quint32 | 视频帧序号 |
| codec | quint16 | 编码类型，当前为 `Jpeg` |
| flags | quint16 | 预留标志位 |
| timestampMs | quint64 | 采集时间戳 |
| dataSize | quint32 | 后面 JPEG 数据长度 |

client 收到后先解析视频帧头，再把 JPEG 数据投递给解码线程。解码成功后显示到远程画面控件。

### 6.5 MouseMove

方向：

```text
client -> server
```

payload 是 compact JSON：

```json
{
  "xRatio": 0.5,
  "yRatio": 0.5
}
```

字段说明：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| xRatio | number | 鼠标在远程画面中的横向比例，范围 0.0 到 1.0 |
| yRatio | number | 鼠标在远程画面中的纵向比例，范围 0.0 到 1.0 |

server 根据自身屏幕尺寸换算实际坐标。

### 6.6 MousePress / MouseRelease

方向：

```text
client -> server
```

payload 是 JSON：

```json
{
  "xRatio": 0.5,
  "yRatio": 0.5,
  "button": "left"
}
```

`button` 可取值：

| 值 | 说明 |
| --- | --- |
| left | 鼠标左键 |
| right | 鼠标右键 |
| middle | 鼠标中键 |

### 6.7 MouseWheel

方向：

```text
client -> server
```

payload 是 compact JSON：

```json
{
  "xRatio": 0.5,
  "yRatio": 0.5,
  "deltaY": 120
}
```

字段说明：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| xRatio | number | 鼠标横向比例 |
| yRatio | number | 鼠标纵向比例 |
| deltaY | integer | Qt wheel 纵向滚动增量 |

### 6.8 KeyEvent

方向：

```text
client -> server
```

payload 是 compact JSON：

```json
{
  "key": 65,
  "modifiers": 0,
  "pressed": true,
  "text": "a"
}
```

字段说明：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| key | integer | Qt key 值 |
| modifiers | integer | Qt 键盘修饰键状态 |
| pressed | boolean | `true` 表示按下，`false` 表示释放 |
| text | string | Qt 事件中的文本 |

server 当前主要根据 `key` 转换为 Windows virtual-key code，并通过 `SendInput` 执行。

### 6.9 FileBegin

方向：

```text
client -> server
```

payload 是 JSON：

```json
{
  "fileName": "example.txt",
  "fileSize": 1024,
  "fileId": "uuid-without-braces"
}
```

server 收到后会在程序目录下创建 `downloads` 目录，并打开目标文件准备写入。

### 6.10 FileData

方向：

```text
client -> server
```

payload 结构如下：

```text
FileDataHeader + file chunk binary bytes
```

当前 client 每次读取的文件块大小为 256 KB。

`FileDataHeader` 当前字段如下：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| offset | quint64 | 当前分片在文件中的起始偏移 |
| chunkIndex | quint32 | 分片序号，从 0 开始 |
| dataSize | quint32 | 后面紧跟的 chunk 长度 |

### 6.11 FileEnd

方向：

```text
client -> server
```

payload 是 JSON：

```json
{
  "fileId": "uuid-without-braces"
}
```

server 收到后校验 `fileId`，关闭文件，并比较已接收大小和 `fileSize` 是否一致。

## 7. TCP 解包流程

`PacketCodec` 内部维护一个缓冲区：

1. socket 收到数据后调用 `appendData()` 追加到缓冲区。
2. `takePacket()` 检查缓冲区是否至少有 18 字节包头。
3. 读取 magic、type、sessionId、length、sequence。
4. 如果 magic 不匹配，丢弃 1 字节并继续寻找下一个可能包头。
5. 如果 payload 长度超过上限，丢弃 1 字节并继续。
6. 如果缓冲区未收到完整 payload，返回空。
7. 如果数据完整，取出 payload，返回一个 `Packet`。
8. 从缓冲区移除已经消费的完整包。

业务层通常用循环读取：

```cpp
while (auto packet = m_codec.takePacket()) {
    // handle packet
}
```

## 8. 发送背压

### 8.1 远程画面发送

server 发送屏幕帧前会检查 TCP 写缓冲：

```text
bytesToWrite() > 2 MB 时丢弃当前帧
```

这可以防止网络较慢时堆积大量过期画面，使远程控制尽量保持低延迟。

### 8.2 文件发送

client 文件发送使用高低水位控制：

| 水位 | 当前值 | 作用 |
| --- | ---: | --- |
| 高水位 | 2 MB | 写缓冲超过该值时暂停读取更多文件块 |
| 低水位 | 512 KB | 写缓冲低于该值后通知 worker 继续读取 |

低水位判断发生在 `RemoteSession` 内部，因为 `RemoteSession` 和 `QTcpSocket` 属于同一线程。

## 9. 当前协议限制

- TCP 包头中有 payload 长度限制，当前上限为 4 MB。
- TCP 通信当前未加密，也没有身份认证。
- UDP 发现只适合可信局域网环境。
- 文件传输当前没有断点续传。
- 屏幕帧当前是整屏 JPEG，不是区域增量传输。
- 当前控制、心跳、屏幕帧和文件传输共用一条 TCP 连接，后续可以进一步拆分控制连接和画面连接。
