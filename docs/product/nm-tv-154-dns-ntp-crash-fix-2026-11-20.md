# NM-TV-154 DNS / NTP TCPIP 断言修复

## 证据

用户刷入天气时钟更新后，在 `[boot] done` 之后重启，触发 `udp_new_ip_type: Required to lock TCPIP core functionality!`。

报告的 ELF SHA256 前缀 `5f9947377` 与本机 WireGuard 版完整 ELF SHA256 `5f9947377a5e0ade92e1d57255eadab0b0c6f9f2a6d8d2faeb17b5d9c8b249ea` 一致。使用同一 ELF 解码回溯确认：

```text
loop -> TickerMode::service -> stocksService -> HTTPClient::GET
 -> NetworkClientSecure::connect
 -> NetworkManager::hostByName (NetworkManager.cpp:84)
 -> dns_clear_cache -> dns_call_found
 -> sntp_dns_found -> sntp_try_next_server -> sntp_request
 -> dns_gethostbyname -> udp_new_ip_type -> assert
```

根因在固定 Arduino ESP32 3.3.11 网络库中：`hostByName()` 发现接口 IP 状态变化后，直接在 Arduino loop 任务里调用 raw lwIP `dns_clear_cache()`。清缓存会通知等待 DNS 的 SNTP 请求；回调重新创建 UDP PCB 时要求 TCPIP 上下文，因而触发断言。此前的固件编译和天气解析测试没有覆盖此网络库与 SNTP 的交互。

后续 `Core dump flash config is corrupted` / `No core dump partition found` 是崩溃记录失败，本机分区表没有 core dump 分区；它们不是这次重启的起因。

## 修复

项目内增加 `src/LwipDns.cpp`，通过链接选项 `--wrap=dns_clear_cache` 拦截跨目标文件的调用，使用 ESP-IDF 的同步 `esp_netif_tcpip_exec()` 在 TCPIP 上下文中调用真实清缓存函数。该 API 自行处理普通任务与已经处于 TCPIP 上下文的情况。调度失败时不回退为无锁调用。

该修复覆盖 ESP32/C2/8MB Pro 构建，WireGuard 版继承普通 ESP32 版链接选项。ESP8266 不启用此包装。不修改本机框架缓存，也不关闭 lwIP 线程安全断言，不关闭 NTP、行情或天气功能。

## 验证与实机检查

`python3 tests/network/test_dns_context.py --baseline` 使用无包装调用，在主机模拟 DNS 清缓存触发 NTP 回调，复现同类 TCPIP 锁断言。`python3 tests/network/test_dns_context.py` 编译实际包装代码，覆盖普通任务、已持有 TCPIP 上下文、调度失败，以及 ESP32/C2 构建宏与 ESP8266 空编译；测试加入 CI。修复后测试通过，原有时钟需求测试也通过。独立代码审查无新增问题。

最终两种固件均编译通过，OTA 余量检查通过：普通版 1,886,464 B，余量 79,616 B；WireGuard 版 1,931,968 B，余量 34,112 B。反汇编验证了实际 WireGuard 镜像的调用链：`NetworkManager::hostByName -> __wrap_dns_clear_cache -> esp_netif_tcpip_exec -> clearDnsCache -> dns_clear_cache`。新 WireGuard ELF SHA256 为 `737813d64c41ede6784590f88c9ab64ab5261bbf2b9eb03f946075ce82dca68c`。

修复版交付目录为 `dist/nm-tv-154-dns-fix-20261120/`，保留了对应 ELF。OTA 包用于网页升级；串口 recovery 包用于这次已安装 NM-TV-154 分区表的重启恢复，写入地址 `0x0`。recovery 从已验证的工厂合并镜像截取至应用末尾，共 1,997,504 B，包含相同引导程序、分区表、OTA 初始化数据和新应用；确认应用、分区表及 OTA 初始化数据逐字节匹配，结束位置早于 `0x3d0000` 的 LittleFS，不包含文件区。写入时无需整片擦除。

源码与主机测试不能替代实机验证。刷入修复版后应检查：启动不再重启，NTP 能完成校时，行情/天气 HTTPS 获取正常，Wi-Fi 重连后仍可更新。当前改动不需要擦除 Flash、重置设置或重写 LittleFS。
