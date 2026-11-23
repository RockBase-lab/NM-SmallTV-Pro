import os
from pathlib import Path
import re
import subprocess
import tempfile


source = Path("src/features/ticker/StockClient.cpp").read_text(encoding="utf-8")


def function(name):
    match = re.search(r"^static [^\n]+\b" + name + r"\([^\n]+\) \{[\s\S]*?^\}", source, re.M)
    assert match, name
    return match.group()


fixture = r'''
#include "Settings.h"
#include "features/ticker/StockData.h"
#include <cassert>
#include <cmath>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
using Stream = std::istringstream;
using std::isnan;
static unsigned millis() { return 1000; }
static bool g_abortFetch = false;
struct NetClient {};
static NetClient* platformMakeSecureClient(int, void*, int, bool) { return new NetClient; }
static std::vector<String> urls;
static std::vector<String> responses;
static unsigned responseIndex = 0;
#define HTTP_CODE_OK 200
struct HTTPClient {
  void setTimeout(unsigned) {}
  void setReuse(bool) {}
  void useHTTP10(bool) {}
  void setUserAgent(const char*) {}
  void addHeader(const char*, const char*) {}
  bool begin(NetClient&, const String& url) { urls.push_back(url); return true; }
  int GET() { return 200; }
  String getString() { assert(responseIndex < responses.size()); return responses[responseIndex++]; }
  void end() {}
};
static const char* lookupKoreanName(const char*) { return "Samsung"; }
static bool fetchNaverMinute(const Settings&, const TickerSettings&, const char*, StockData&) { return false; }
'''

checks = r'''
int main() {
  assert(naverNumber("1,234.5") == 1234.5f);
  assert(naverNumber("-1,234.5") == -1234.5f);
  assert(naverNumber("0") == 0 && naverNumber("") == 0);
  TickerSettings ticker;
  ticker.range = "1mo";
  ticker.points = 20;
  assert(!strcmp(naverTimeframe(ticker.range), "day"));
  assert(naverCount(ticker.range) == 20);
  assert(buildYahooUrl(ticker, YAHOO_CHART_HOST1, "NVDA") ==
         "https://query1.finance.yahoo.com/v8/finance/chart/NVDA?range=1mo&interval=1d");
  StockData stock;
  stock.clear();
  strlcpy(stock.symbol, "NVDA", sizeof(stock.symbol));
  Stream yahoo(R"({"chart":{"result":[{"meta":{"regularMarketPrice":145,"chartPreviousClose":140,"currency":"USD","shortName":"NVIDIA"},"indicators":{"quote":[{"open":[130,131,132,133],"high":[132,133,134,144],"low":[129,130,131,132],"close":[131,null,133,143]}]}}],"error":null}})");
  assert(parseYahoo(ticker, stock, yahoo));
  assert(stock.valid && !stock.error && stock.price == 145);
  assert(stock.sparkCount == 3 && stock.candleCount == 3);
  assert(stock.spark[0] == 131 && stock.spark[2] == 145);
  assert(stock.candles[2].high == 145 && stock.candles[2].close == 145);
  assert(stock.change == 5 && stock.hasChange && !strcmp(stock.currency, "$"));
  Stream badYahoo(R"({"chart":{"result":null,"error":{"code":"Not Found"}}})");
  assert(!parseYahoo(ticker, stock, badYahoo));
  ticker.points = 2;
  Stream sparse(R"({"chart":{"result":[{"meta":{"regularMarketPrice":20},"indicators":{"quote":[{"close":[10,11,12,13,14,15]}]}}]}})");
  assert(parseYahoo(ticker, stock, sparse));
  assert(stock.sparkCount == 2 && stock.spark[0] == 10 && stock.spark[1] == 20);

  Settings settings;
  settings.httpTimeout = 5000;
  ticker.points = 20;
  stock.clear();
  stock.userNamed = true;
  strlcpy(stock.name, "Samsung", sizeof(stock.name));
  responses.push_back(R"({"closePrice":"72,500","compareToPreviousClosePrice":"-1,500","fluctuationsRatio":"-2.03","stockName":"Samsung"})");
  responses.push_back(R"(<protocol><chartdata><item data="20261118|74000|75000|73500|74000|12345" /><item data="20261119|73000|73500|72000|72800|23456" /></chartdata></protocol>)");
  assert(fetchNaver(settings, ticker, "005930", stock));
  assert(urls.size() == 2);
  assert(urls[0] == "https://m.stock.naver.com/api/stock/005930/basic");
  assert(urls[1] == "https://fchart.stock.naver.com/sise.nhn?symbol=005930&timeframe=day&count=20&requestType=0");
  assert(stock.price == 72500 && stock.change == -1500 && stock.prevClose == 74000);
  assert(stock.sparkCount == 2 && stock.candleCount == 2);
  assert(stock.spark[0] == 74000 && stock.spark[1] == 72500);
  assert(stock.valid && !stock.error && !strcmp(stock.currency, "KRW "));
  puts("Yahoo and Naver quote/chart parsing and default requests passed");
}
'''

functions = "\n".join(function(name) for name in (
    "urlEncode", "yahooInterval", "buildYahooUrl", "yahooCurrency",
    "naverTimeframe", "naverCount", "naverNumber", "fetchNaver", "parseYahoo"))
with tempfile.TemporaryDirectory() as directory:
    test_source = Path(directory) / "providers.cpp"
    executable = Path(directory) / "providers"
    test_source.write_text(fixture + functions + checks, encoding="utf-8")
    subprocess.run([
        os.environ.get("CXX", "c++"), "-std=c++11", "-Wall", "-Wextra", "-Werror",
        "-Wno-unused-parameter", "-DSMALLTV_ESP8266", "-DARDUINOJSON_USE_DOUBLE=0",
        "-DARDUINOJSON_ENABLE_ARDUINO_STRING=1", "-Itests/fuzz/shim", "-Isrc",
        "-I" + os.environ.get("THEME_JSON_INCLUDE", ".pio/libdeps/smalltv_esp32_8mb/ArduinoJson/src"),
        str(test_source), "-o", str(executable)], check=True)
    subprocess.run([str(executable)], check=True)

assert 'settingsApplyJson(s, doc.as<JsonObjectConst>(), true)' in Path("src/Settings.cpp").read_text(encoding="utf-8")
