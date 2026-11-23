# NM-TV-154 全彩图片库与模式修复

## 本次更新

- 使用 SmallTV Ultra 原图生成 10 个 240×240 RGB JPEG 图片包，版本 2.0.0，避免旧的低色数资源。动画使用合法的布尔循环参数；受 192 KiB 文件系统限制，三个动画各保留两帧。
- Pictures 页面把 `layers[0].loop` 转换为“动画包版本过旧，请安装新版全彩图片库”，设备屏幕提示 `Update picture library`。
- General 的单应用、定时应用和轮播勾选统一按固件功能生成，包含 Usage、Plane Radar、Home Assistant。
- Carousel 只显示勾选项；“下一屏”只切换勾选项并继续轮播，“返回首页”从首个勾选项重新计时。空列表显示设置提示。
- Timer 按有效计划运行，支持跨午夜；触摸浏览 15 秒后恢复计划。未同步时间时等待时钟，无有效计划时提示设置。
- 保存模式后重启保持选择。Detailed weather 是出厂默认；保存设置不会把其他模式意外改成 Timer。

## 更新已有设备

先通过设备网页 Firmware → Upload 上传对应 OTA 文件。当前 `192.168.124.14` 为 WireGuard 版本，应使用 `smalltv-nm-tv-154-wg-ota.bin`。标准版使用无 `-wg` 的 OTA 文件。

**仅上传 OTA 不会更新图片库。** 固件重启后，解压图片库压缩包，在其目录运行以下命令（Python 3，无需额外依赖）：

```powershell
python update_nm_tv_154_pictures.py 192.168.124.14
python update_nm_tv_154_pictures.py 192.168.124.14 --apply
```

第一条只检查，第二条实际更新。工具先验证全部本地包的 SHA256、RGB JPEG 格式和循环参数，再临时切换 Weather，释放空间并替换十个内置图片包。自定义图片、Classic 时钟和其他配置保留；完成后恢复原模式、主题和图片选择。

设备启用了管理密码时增加 `--username 用户名`，并通过环境变量 `SMALLTV_PASSWORD` 提供密码。

更新中断时保留生成的 `nm-tv-154-picture-update-state.json`，在同一目录重跑 `--apply`；工具使用该文件恢复更新前的选择。请勿删除此状态文件或同时运行多个更新。全程保持设备通电。

## 文件说明

- `*-ota.bin`：网页 OTA，保留文件系统及配置。
- `*-factory.bin`：USB 初次安装或分区迁移，包含全彩图片库，会覆盖文件系统及配置；已有正常设备请使用 OTA 加图片库工具。
- `*-debug.elf`：对应固件的崩溃回溯符号。
- `SHA256SUMS.txt`：交付文件校验值。
- `pictures-preview.png`：从实际打包的 JPEG 解码得到的预览。

## 本地重新构建

网页有改动后可使用 `python tools/gzip_webui.py --small`（桌面端安装 `zopfli==0.4.3`）生成更小的标准 gzip，设备无需额外解码库；`--check` 校验解压内容与 HTML 一致。双版本均需通过 `tools/ci_checks.py size` 的 8192 字节最小 OTA 余量检查。

打包命令：`python tools/package_nm_tv_154_update.py dist/nm-tv-154-pictures-modes-20261120`，输出目录必须尚不存在。
