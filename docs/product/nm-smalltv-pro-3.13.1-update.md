# NMMiner SmallTV Pro 3.13.1 — NM-TV-154

2026-11-23

软件名称为 **NMMiner SmallTV Pro**，固件标识为 `nm-smalltv-pro`，项目地址为 https://github.com/RockBase-lab/NM-SmallTV-Pro 。网页、启动标题、系统信息、GitHub 更新源和当前文档已统一。版本从 3.13.0 升至 3.13.1。GitHub 发布资产采用 `nm-smalltv-pro-firmware*` 名称，CI 与固件查询保持一致。本地生成发布包不代表已向 GitHub 推送或发布 Release。

## CRASH 排查与恢复

本轮实机读回上次重启原因为 `Exception/panic`。旧版在 panic 后暂停应用显示并保持网页服务，屏幕因此停留在 CRASH；“OTA flash to fix”不等于检测到 OTA 文件损坏。旧版未保留 ESP32 异常寄存器且没有 core dump 分区，当时串口未在捕获，无法从现存数据确认是哪一条代码触发。

已在 COM3 115200 下重启恢复运行，观察 382 秒并进行 25 次只读股票检索，未再次出现 panic。用户确认崩溃前没有操作，因此不能将搜索压力测试结果当作后台问题已修复的证据。此次升级增强诊断和恢复说明，尚未确认原始 panic 的根因。

- CRASH 页改为引导查看网页诊断及 System → Reboot；网页状态明确标记安全模式。
- NM-TV-154 使用 Arduino 已有的 panic 回调，保留原因、可用 PC、最多 12 个回溯地址、近似时长、执行阶段及固件版本至 RTC 内存；正常重启后 Last crash 仍可读，完全断电会丢失。回调没有提供的异常原因码或故障地址不会伪造。
- abort/assert 单独记录，避免把 IDF 故意触发的非法指令误认作原始异常；历史记录不会被当作后来 watchdog 的当前地址。
- 原有分区、设置、图片库和模块功能保留。没有新增公开的强制崩溃接口。

## 默认密码

网页：默认用户名 `admin`，默认密码为空，出厂不启用登录保护。保存两次新密码后启用 HTTP Digest。密码输入框留空表示保留当前密码，并非清除。

NM-TV-154 配网热点：`SmallTV-Setup`，默认 WiFi 密码 `12345678`，与网页管理员密码无关。升级保留用户已有设置。

## 默认地点

NM-TV-154 出厂地点设为中国成都（纬度 30.5728，经度 104.0668），时区为 `Asia/Shanghai`，规则 `CST-8`。升级不会自动覆盖已保存的位置；本次按用户要求将实机此前未设置的位置补为成都。雷达独立中心保持原设置，选择跟随设备时才使用设备地点。

字体与 spaceman 帧数据采用无损压缩调整，解码结果逐像素比对原资源；本次没有为容量缩减色彩或移除功能。

## 安装

当前设备已使用本项目 NM-TV-154 分区：普通版用 `nm-smalltv-pro-nm-tv-154-ota.bin`，WireGuard 版用 `nm-smalltv-pro-nm-tv-154-wg-ota.bin`，在 System 的 Manual update 上传。不要把 factory 镜像当普通 OTA 上传。当前全彩 v2 图片库无需重复安装。发布包包含中英文第三版手册、校验摘要与调试 ELF。
