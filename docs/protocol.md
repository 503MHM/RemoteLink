# 自定义 TCP/UDP 通信协议说明

## 1. 协议概述

RemoteLink 使用两套通信方式：

- UDP：用于局域网设备发现。
- TCP：用于远程画面、鼠标键盘控制、文件传输、心跳。

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

TCP 是字节流协议，可能出现粘包和半包。因此 RemoteLink 定义了固定包头。

所有 TCP 消息格式如下：

```text
+------------+---------+----------+------------+---------+
| magic      | type    | length   | sequence   | payload |
+------------+---------+----------+------------+---------+
| quint32    | quint16 | quint32  | quint32    | bytes   |
+------------+---------+----------+------------+---------+
```

字段说明：

| 字段 | 类型 | 字节数 | 说明 |
| --- | --- | ---: | --- |
| magic | quint32 | 4 | 固定魔数，值为 `0x524C4B31`，ASCII 含义是 `RLK1` |
| type | quint16 | 2 | 消息类型，对应 `MessageType` |
| length | quint32 | 4 | payload 字节数 |
| sequence | quint32 | 4 | 包序号 |
| payload | bytes | length | 具体业务数据 |

包头总长度为 14 字节。

当前编码使用 `QDataStream::BigEndian`，因此包头字段按大端字节序写入。

## 4. TCP 消息类型

| 类型名 | 数值 | 方向 | 说明 |
| --- | ---: | --- | --- |
| Discovery | 1 | 未用于 TCP | 发现类型枚举保留 |
| DiscoveryReply | 2 | 未用于 TCP | 发现响应枚举保留 |
| FrameData | 10 | server -> client | 远程屏幕 JPEG 帧 |
| MouseMove | 20 | client -> server | 鼠标移动 |
| MouseClick | 21 | 保留 | 当前未使用 |
| MouseWheel | 22 | client -> server | 鼠标滚轮 |
| MousePress | 23 | client -> server | 鼠标按下 |
| MouseRelease | 24 | client -> server | 鼠标释放 |
| FileBegin | 30 | client -> server | 文件传输开始 |
| FileData | 31 | client -> server | 文件数据块 |
| FileEnd | 32 | client -> server | 文件传输结束 |
| Heartbeat | 40 | client -> server | 连接后心跳，server 收到后启动截图 |
| KeyEvent | 50 | client -> server | 键盘事件 |

## 5. 各消息 payload 格式

### 5.1 Heartbeat

方向：

```text
client -> server
```

payload 当前是普通字节串：

```text
hello from client
```

server 收到 `Heartbeat` 后，触发截图线程开始按定时器采集屏幕。

### 5.2 FrameData

方向：

```text
server -> client
```

payload 是一帧 JPEG 图片的原始字节：

```text
JPEG binary bytes
```

client 收到后，将 payload 投递给解码线程。解码成功后显示到远程画面控件。

### 5.3 MouseMove

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

server 根据自己的屏幕尺寸换算实际坐标。

### 5.4 MousePress / MouseRelease

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

### 5.5 MouseWheel

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

### 5.6 KeyEvent

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

### 5.7 FileBegin

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

### 5.8 FileData

方向：

```text
client -> server
```

payload 是文件原始字节块：

```text
file chunk binary bytes
```

当前 client 每次读取的文件块大小为 256 KB。

### 5.9 FileEnd

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

## 6. TCP 解包流程

`PacketCodec` 内部维护一个缓冲区：

1. socket 收到数据后调用 `appendData()` 追加到缓冲区。
2. `takePacket()` 检查缓冲区是否至少有 14 字节包头。
3. 读取 magic、type、length、sequence。
4. 如果 magic 不匹配，丢弃 1 字节并继续寻找下一个可能包头。
5. 如果缓冲区未收到完整 payload，返回空。
6. 如果完整，取出 payload，返回一个 `Packet`。

业务层通常用循环读取：

```cpp
while (auto packet = m_codec.takePacket()) {
    // handle packet
}
```

## 7. 发送背压

### 7.1 远程画面发送

server 发送屏幕帧前会检查 TCP 写缓冲：

```text
bytesToWrite() > 2 MB 时丢弃当前帧
```

这可以防止网络较慢时堆积大量过期画面。

### 7.2 文件发送

client 文件发送使用高低水位：

| 水位 | 当前值 | 作用 |
| --- | ---: | --- |
| 高水位 | 2 MB | 写缓冲超过该值时暂停读取更多文件块 |
| 低水位 | 512 KB | 写缓冲低于该值后通知 worker 继续读取 |

低水位判断发生在 `RemoteSession` 内部，因为 `RemoteSession` 和 `QTcpSocket` 属于同一线程。

## 8. 当前协议限制

- TCP 包头中有 payload 长度，但当前未设置最大包大小限制。
- TCP 通信当前未加密，也没有身份认证。
- UDP 发现只适合可信局域网环境。
- 文件传输当前没有断点续传。
- 屏幕帧当前是整屏 JPEG，不是区域增量传输。
