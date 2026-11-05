# RemoteLink 性能测试记录方法

## 测试条件

每次测试记录以下环境信息，避免把不同条件下的数据直接比较：

- CPU、GPU、内存、Windows 版本和显示器分辨率
- client/server 是同机、局域网有线还是 Wi-Fi，网卡型号与链路速率
- 构建类型、Qt/编译器版本、JPEG quality、UDP 分片大小
- 屏幕分辨率、测试时长和测试画面内容

建议使用持续变化的同一画面，先预热 20 秒，再连续记录 60 秒；每种配置至少测 3 次并报告中位数。分别测同机回环和真实局域网，不要混为一组。测试文件传输时使用同一个文件、同一磁盘和同一网络条件。

## 画面测试

JPEG quality 默认是 100。启动 server 前设置环境变量 `REMOTELINK_JPEG_QUALITY`，可直接测试 40、60、80、100，无需修改代码或重新构建；除该值外，固定分辨率、采集间隔 16 ms、UDP 分片大小 1400 字节。

PowerShell 示例：

```powershell
$env:REMOTELINK_JPEG_QUALITY = '40'
& '.\server.exe'
```

每轮测试前重启 server，并确认 `[capture]` 日志中的 `quality` 与预期一致。

记录每秒日志：

| 日志 | 指标 | 用途 |
| --- | --- | --- |
| server `[capture]` | `capture_fps`、`jpeg_fps` | DXGI 新帧频率及成功编码频率 |
| server `[capture]` | `capture_avg_ms/max_ms` | DXGI 获取与 CPU 图像拷贝耗时 |
| server `[capture]` | `jpeg_avg_ms/max_ms`、`jpeg_avg_kib` | JPEG 编码耗时和平均帧大小 |
| server `[udp-tx]` | `frames_per_sec` | 应用层完整写入所有分片的帧率 |
| server `[udp-tx]` | `datagrams_per_sec`、`mib_per_sec` | 应用层提交到 UDP socket 的分片数和吞吐 |
| client `[udp-rx]` | `datagrams_per_sec`、`complete_frames_per_sec` | 收到的数据报和完整重组帧频率 |
| client `[udp-rx]` | `replaced_frames`、`timed_out_frames`、`missing_fragments` | 未完成帧被新帧替换或超时的情况 |
| client `[decode]` | `fps`、`avg_ms/max_ms` | JPEG 解码频率和耗时 |
| client `[display]` | `fps`、`pending_frame_overwrites` | UI 最终刷新频率，以及解码忙时被覆盖的待处理帧 |
| server `[file-rx]` | `elapsed_sec`、`average_mib_per_sec` | server 收到并关闭文件后的实际传输耗时和平均吞吐 |

`udp-tx` 统计的是 `writeDatagram()` 成功写入，不是对端确认收到。`missing_fragments` 只统计被替换或超时帧中缺少的分片，不能直接当成网络丢包率。UDP 发送/接收的帧率差、完整帧率和未完成帧计数应联合判断。

### 当前桥接模式诊断记录

最近一轮 VMware 桥接测试中，服务端约 146 秒日志里出现 3 次 `writeDatagram()` 失败，错误为 `socketError=7`、`Unable to send a message`；客户端 `rejected_datagrams=0`、`timed_out_frames=0`、`decode failed=0`，`replaced_frames` 也极少。该轮数据没有显示持续性 UDP 接收丢帧，发送错误属于偶发事件。

同轮服务端 `capture_fps` 平均约 24.7，`capture_avg_ms` 平均约 22.0 ms；客户端完整帧率约 24.7 FPS，与服务端产帧基本一致。因此这轮主要性能限制在虚拟机端采集/编码链路，不是客户端解码或持续 UDP 丢包。此结论只适用于该轮测试环境，不应当作所有机器的固定性能结果。`socketError` 的具体平台原因仍需结合 VMware/Windows 网络状态进一步确认。

### 初步定位

- `capture_fps` 低：重点看 DXGI 获取/拷贝、屏幕更新频率和采集定时器。
- `jpeg_avg_ms` 高：JPEG 编码是主要成本；降低 quality 会缩小帧并减少编码时间，但影响画质。
- `jpeg_fps` 高而 `udp-tx frames_per_sec` 低：检查分片数、socket 发送错误及服务端发送耗时。
- `udp-tx` 明显高于 client `complete_frames_per_sec`，且替换/超时/缺片持续增长：瓶颈在 UDP 路径或接收重组阶段；这些差值包含接收端主动丢弃旧帧，不能单独解释为链路丢包。
- client 完整帧率高而 `[decode] fps` 低：JPEG 解码或 CPU 负载受限。
- `[decode] fps` 高而 `display fps` 低：检查 UI 线程、图像缩放和绘制成本。

## 文件传输测试

使用同一文件至少测试 3 次，计时从开始发送到 server 报告接收完成。记录文件字节数和耗时，并使用统一单位计算：

```text
吞吐量 (MB/s) = 文件字节数 / 1,000,000 / 秒数
```

同时记录磁盘类型、文件大小、网络方式和是否同机。简历应写清是 MB/s 还是 MiB/s，并写实际测量条件；不要把旧 main 分支的测试结果当作当前 UDP 版本的数据。
