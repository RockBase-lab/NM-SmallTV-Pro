# NM-TV-154 usability update / 易用性更新

2026-11-20，基于 3.12.0。完整中英文 PDF 和 Markdown 手册位于 manuals；产品复盘记录已修问题、前置条件与验证范围。

## 更新内容

修正共享地点同步、合法零坐标、已支持城市时区选择、天气未设置位置提示、默认 Carousel 选择、固定天气布局快捷操作、网络模式／图片操作提示、完整备份恢复空秘密字段，以及临时网络信息页退出时的提示恢复。已有明确保存的模式和轮播选择保留。

This release aligns setup, saving, display modes, shared weather location and backup restoration. Existing saved modes and explicit Carousel choices are preserved. See the English manual and product review in manuals for the workflow and limits.

## 安装 / Installation

- 日常升级：System → Manual update (OTA)，上传 `smalltv-nm-tv-154-wg-ota.bin`（含 WireGuard）或 `smalltv-nm-tv-154-ota.bin`（标准版），保持供电并等待重启。
- OTA 仅适用于当前双 1,966,080 字节槽／192 KiB LittleFS 分区。旧分区迁移及救援使用匹配的 factory 文件，经 USB／UART 安装；可能覆盖配置与文件，先导出备份。
- Factory 文件不得上传至网页 OTA。Debug ELF 用于解析本版本串口回溯，不是升级文件。
- 本包保留 v2 全彩图片库。已是 v2 的设备无需重复替换；旧图片不会被应用 OTA 改写。完整手册包含图片工具使用方法。

For a routine update on the current layout, upload the matching app OTA file under System. Keep power connected. Never upload a factory image to OTA. Factory images are for board-specific USB/UART installation or partition migration and can overwrite settings/files. Keep settings exports private and back up package sources separately.

## 验证 / Validation

Both firmware targets pass build and minimum 8,192-byte OTA headroom checks. Web UI, native mode scheduling, complete backup import and weather regressions pass. The WG app is 1,957,776 bytes, leaving 8,304 bytes in its slot. Printable manuals include 14 matching chapters; consult the review report for device verification and remaining limits.

192.168.124.14 已完成本轮 WG OTA。COM3 启动完成，60 秒采样未见意外重启／panic；配置、网页和十个 v2 图片包核对通过，串口已释放。Physical panel colors and long-term stability still require observation on the device.

`SHA256SUMS.txt` lists every packaged file except the checksum list itself. The picture-library ZIP is provided for legacy library replacement; use the manuals in this release as the current product instructions.
