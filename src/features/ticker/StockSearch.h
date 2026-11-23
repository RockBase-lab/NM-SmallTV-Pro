#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

namespace stocksearch {
inline bool validQuery(const char* query) {
  size_t n = strlen(query);
  if (!n || n > 80) return false;
  for (size_t i=0; i<n; ++i)
    if ((unsigned char)query[i] < 32 || (unsigned char)query[i] == 127) return false;
  return true;
}
inline String encode(const char* query) {
  String out;
  const char* hex = "0123456789ABCDEF";
  for (const unsigned char* p=(const unsigned char*)query; *p; ++p) {
    if ((*p>='a'&&*p<='z') || (*p>='A'&&*p<='Z') || (*p>='0'&&*p<='9') || *p=='-' || *p=='.' || *p=='_' || *p=='~') out += (char)*p;
    else { out += '%'; out += hex[*p>>4]; out += hex[*p&15]; }
  }
  return out;
}
template<class T> class BoundedReader {
  T& input;
  size_t remaining;
  uint32_t deadline;
public:
  BoundedReader(T& stream, size_t limit, uint32_t timeout): input(stream), remaining(limit), deadline(millis()+timeout) {}
  int read() {
    if (!remaining || (int32_t)(millis()-deadline)>=0) return -1;
    char c;
    if (input.readBytes(&c,1)!=1) return -1;
    --remaining;
    return (unsigned char)c;
  }
  size_t readBytes(char* buffer, size_t count) {
    size_t n=0; int c;
    while (n<count && (c=read())>=0) buffer[n++]=(char)c;
    return n;
  }
};
inline void normalize(JsonArrayConst quotes, JsonArray results) {
  for (JsonObjectConst quote : quotes) {
    const char* symbol=quote["symbol"] | "";
    const char* type=quote["quoteType"] | "";
    if (!symbol[0] || strlen(symbol)>=24 || (strcmp(type,"EQUITY") && strcmp(type,"ETF"))) continue;
    JsonObject item=results.add<JsonObject>();
    item["symbol"]=symbol;
    item["name"]=quote["shortname"] | (quote["longname"] | symbol);
    item["market"]=quote["exchDisp"] | (quote["exchange"] | "Yahoo Finance");
    item["source"]="yahoo";
    if (results.size()>=8) break;
  }
}
}
