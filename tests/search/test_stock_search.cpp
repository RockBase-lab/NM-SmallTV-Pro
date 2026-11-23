#include <Arduino.h>
#include <ArduinoJson.h>
#include <cassert>
#include <string>
unsigned long millis() { return 0; }
#include "features/ticker/StockSearch.h"
struct Input {
  std::string data; size_t position=0;
  size_t readBytes(char* out,size_t n) {size_t count=std::min(n,data.size()-position);memcpy(out,data.data()+position,count);position+=count;return count;}
};
int main() {
  using namespace stocksearch;
  assert(!validQuery("") && !validQuery("ab\ncd"));
  assert(validQuery("Apple") && validQuery("三星"));
  assert(!validQuery(std::string(81,'a').c_str()));
  assert(encode("A&B 三") == "A%26B%20%E4%B8%89");
  Input input{"123456"};BoundedReader<Input> reader(input,4,4000);
  char bytes[9];assert(reader.readBytes(bytes,9)==4);assert(reader.read()==-1);
  JsonDocument upstream,output;
  deserializeJson(upstream,R"({"quotes":[{"symbol":"AAPL","shortname":"Apple","exchDisp":"NASDAQ","quoteType":"EQUITY"},{"symbol":"SPY","longname":"S&P 500","quoteType":"ETF"},{"symbol":"BTC-USD","quoteType":"CRYPTOCURRENCY"},{"symbol":"","quoteType":"EQUITY"},{"symbol":"ABCDEFGHIJKLMNOPQRSTUVWX","quoteType":"EQUITY"}]})");
  normalize(upstream["quotes"].as<JsonArrayConst>(),output["results"].to<JsonArray>());
  assert(output["results"].size()==2);
  assert(output["results"][0]["source"]=="yahoo");
  assert(output["results"][0]["market"]=="NASDAQ");
  assert(output["results"][1]["name"]=="S&P 500");
}
