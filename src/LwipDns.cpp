#include "Target.h"

#if defined(SMALLTV_FAMILY_ESP32)
#include <esp_netif.h>

extern "C" void __real_dns_clear_cache();

static esp_err_t clearDnsCache(void*) {
  __real_dns_clear_cache();
  return ESP_OK;
}

extern "C" void __wrap_dns_clear_cache() {
  esp_netif_tcpip_exec(clearDnsCache, nullptr);
}
#endif
