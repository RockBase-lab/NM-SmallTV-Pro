import os
from pathlib import Path
import re
import subprocess
import tempfile

source = Path('src/WebPortal.cpp').read_text(encoding='utf-8')
handler = re.search(r'static void handleImport\(\) \{[\s\S]*?\n\}', source).group()
fixture = r'''
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include "Settings.h"
#include "SettingsTransaction.h"
#include <ArduinoJson.h>
#include <cassert>
FSClass LittleFS;
EspClass ESP;
static Settings settings;
static Settings* S = &settings;
static bool authenticated = true, writable = true;
static unsigned rebootMs = 0;
struct Server {
  String payload;
  int status = 0;
  bool hasArg(const char*) { return payload.length() != 0; }
  String arg(const char*) { return payload; }
  void send(int code, const char*, const char*) { status = code; }
} server;
bool requireAuth() { return authenticated; }
void appRelease() {}
void sendParseError(DeserializationError) { server.status = 400; }
bool importSaveSettings(const Settings&) { return writable; }
void scheduleReboot(unsigned value) { rebootMs = value; }
#define saveSettings importSaveSettings
'''
checks = r'''
int main() {
  settings.setDefaults();
  settings.hostname = "current";
  settings.auth.pass = "later-password";
  settings.wg.privateKey = "later-private-key";
  settings.ha.brokerPass = "later-mqtt-password";
  for (const char* payload : {"[]", "{}", "{\"hostname\":\"backup\"}", "not json"}) {
    server.payload = payload;
    handleImport();
    assert(server.status == 400 && settings.hostname == "current" && rebootMs == 0);
  }
  Settings backup;
  backup.setDefaults();
  backup.hostname = "backup";
  backup.mode = MODE_RADAR;
  JsonDocument document;
  settingsToJson(backup, document.to<JsonObject>(), true);
  server.payload = "";
  serializeJson(document, server.payload);
  authenticated = false;
  handleImport();
  assert(settings.hostname == "current" && rebootMs == 0);
  authenticated = true;
  writable = false;
  handleImport();
  assert(server.status == 500 && settings.hostname == "current" && rebootMs == 0);
  assert(settings.auth.pass == "later-password" && settings.wg.privateKey == "later-private-key");
  writable = true;
  handleImport();
  assert(server.status == 200 && settings.hostname == "backup" && rebootMs == 800);
  assert(settings.mode == MODE_RADAR);
  assert(settings.auth.pass.length() == 0 && settings.wg.privateKey.length() == 0 && settings.ha.brokerPass.length() == 0);
}
'''
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory)/'import.cpp'
    binary = Path(directory)/'import'
    path.write_text(fixture + handler + checks)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++11', '-Wall', '-Wextra', '-Werror',
                    '-Wno-unused-parameter', '-DSMALLTV_ESP8266', '-DARDUINOJSON_USE_DOUBLE=0',
                    '-DARDUINOJSON_ENABLE_ARDUINO_STRING=1', '-Itests/fuzz/shim', '-Isrc',
                    '-Isrc/features/theme', '-I.pio/libdeps/smalltv_esp32_8mb/ArduinoJson/src',
                    str(path), 'src/Settings.cpp', '-lm', '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Actual import handler: validation, authentication, rollback and complete backup restoration passed')
