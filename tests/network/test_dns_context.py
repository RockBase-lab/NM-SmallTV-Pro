import argparse
import configparser
import os
from pathlib import Path
import subprocess
import tempfile


parser = argparse.ArgumentParser()
parser.add_argument("--baseline", action="store_true")
args = parser.parse_args()

header = r'''
#pragma once
using esp_err_t = int;
constexpr esp_err_t ESP_OK = 0;
using esp_netif_callback_fn = esp_err_t (*)(void*);
esp_err_t esp_netif_tcpip_exec(esp_netif_callback_fn callback, void* context);
'''
network = r'''
#include "esp_netif.h"
#include <cassert>
bool coreHeld = false, dispatchFailed = false;
unsigned clears = 0, ntpCallbacks = 0;
static void ntpDnsFound() {
  assert(coreHeld && "Required to lock TCPIP core functionality!");
  ++ntpCallbacks;
}
extern "C" void dns_clear_cache() {
  ++clears;
  ntpDnsFound();
}
esp_err_t esp_netif_tcpip_exec(esp_netif_callback_fn callback, void* context) {
  if (dispatchFailed) return -1;
  bool alreadyHeld = coreHeld;
  coreHeld = true;
  esp_err_t result = callback(context);
  coreHeld = alreadyHeld;
  return result;
}
'''
caller = r'''
#include <cassert>
extern bool coreHeld, dispatchFailed;
extern unsigned clears, ntpCallbacks;
extern "C" void dns_clear_cache();
int main() {
  dns_clear_cache();
  assert(clears == 1 && ntpCallbacks == 1 && !coreHeld);
  coreHeld = true;
  dns_clear_cache();
  assert(clears == 2 && ntpCallbacks == 2 && coreHeld);
  coreHeld = false;
  dispatchFailed = true;
  dns_clear_cache();
  assert(clears == 2 && ntpCallbacks == 2 && !coreHeld);
}
'''
with tempfile.TemporaryDirectory() as directory:
    folder = Path(directory)
    (folder / "esp_netif.h").write_text(header)
    (folder / "network.cpp").write_text(network)
    (folder / "caller.cpp").write_text(caller)
    for target in ("SMALLTV_ESP32", "SMALLTV_ESP32C2"):
        executable = folder / "dns-test"
        command = [os.environ.get("CXX", "c++"), "-std=c++11", "-Wall", "-Wextra", "-Werror", "-Isrc", "-I" + directory, "-D" + target, str(folder / "network.cpp"), str(folder / "caller.cpp")]
        if not args.baseline:
            command += ["src/LwipDns.cpp", "-Wl,--wrap=dns_clear_cache"]
        subprocess.run(command + ["-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True)
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++11", "-Wall", "-Wextra", "-Werror", "-Isrc", "-DSMALLTV_ESP8266", "-c", "src/LwipDns.cpp", "-o", str(folder / "esp8266.o")], check=True)

config = configparser.ConfigParser(interpolation=None)
config.read("platformio.ini", encoding="utf-8")
assert "-Wl,--wrap=dns_clear_cache" in config["esp32_base"]["build_flags"]
for environment in ("smalltv_esp32", "smalltv_esp32_8mb", "smalltv_c2"):
    assert "${esp32_base.build_flags}" in config["env:" + environment]["build_flags"]
assert config["env:smalltv_esp32_wg"]["extends"] == "env:smalltv_esp32"
assert "${env:smalltv_esp32.build_flags}" in config["env:smalltv_esp32_wg"]["build_flags"]
print("DNS cache/NTP callback TCPIP context tests passed")
