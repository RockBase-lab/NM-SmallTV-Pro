# NM-TV-154 默认股票和曲线

## 使用

在设备上通过触摸切换到 **Stock 1 / 行情 1**，即可查看默认股票；网页同名页面可查看报价、修改自选股和曲线设置，也可在 General 中将 Stock 1 加入应用轮播。设备仍以 Detailed weather 开机。

| 公司 | 代码 | 默认接口 | 币种 |
| --- | --- | --- | --- |
| 三星电子 | 005930 | Naver Finance | KRW |
| 英伟达 | NVDA | Yahoo Finance | USD |
| 苹果 | AAPL | Yahoo Finance | USD |

无需申请 API Key、填写 Webhook 或部署服务器。每 120 秒刷新，每家公司显示 10 秒；默认显示价格、涨跌和一个月折线图，最多 20 个点。默认涨跌计算基于曲线首点到最新价格，可在设置中改为单日涨跌。三星使用约 20 个交易日的日线；美股使用一个自然月的数据并降采样至 20 点。最新曲线点与当前报价保持一致，休市时仍显示最近数据。

## 接口

三星报价：

`https://m.stock.naver.com/api/stock/005930/basic`

三星曲线：

`https://fchart.stock.naver.com/sise.nhn?symbol=005930&timeframe=day&count=20&requestType=0`

英伟达和苹果报价及曲线（将 NVDA 替换为 AAPL）：

`https://query1.finance.yahoo.com/v8/finance/chart/NVDA?range=1mo&interval=1d`

Yahoo 首个主机失败时沿用现有 query2 主机重试。公共接口可能延迟、限流或调整；实际设备网络需要能访问对应域名。

## 升级兼容

- 新配置直接包含三家公司，Stock 2 和虚拟货币列表保持各自独立。
- 旧设备配置若 Stock 1 为空，在加载时自动填入默认列表并启用曲线。已有非空自选股、行情源和图表设置保留。
- 新版保存和导出配置包含 `stockDefaultsVersion=1`。之后主动删除全部股票并保存，重启仍保持为空；普通 API 保存不会反复填回默认股票。
- 保留 Detailed weather 默认开机页、spaceman 动画、Sunrise/IP 轮换及此前 DNS/TCPIP 修复。

## 验证

- 主机直接访问上述公开接口成功：英伟达和苹果各返回 23 个有效历史收盘点，三星返回 20 根日线，三者报价和币种字段可读取。
- 原生测试执行真实 Yahoo JSON 与 Naver JSON/XML 解析代码，覆盖缺失历史点、降采样、最新价格补入曲线、带逗号正负价格、小数和错误响应。配置测试覆盖默认值、旧版嵌套/平铺空列表、已有自选股、主动清空和保存恢复。
- 设置、天气启动和 DNS 回归以及股票相关网页/i18n 测试通过。尚未烧录设备，屏幕和设备侧联网效果待上机验证。

发布文件与 OTA 容量见 `dist/nm-tv-154-stocks-20261120/README.md`；只需上传对应 OTA 程序，无需重刷文件系统。

标准版程序为 1,912,288 字节，OTA 余量 53,792 字节；WireGuard 版为 1,957,888 字节，余量 8,192 字节，均通过项目规定的最小 8 KiB 余量检查。WG 版后续增加功能仍需持续检查容量。
