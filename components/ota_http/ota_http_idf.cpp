

#ifdef USE_ESP_IDF

#include "ota_http_idf.h"
#include "esphome/core/defines.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/components/network/util.h"
#include "esphome/components/md5/md5.h"

#include <string.h>
#include <sys/param.h>
#include <stdlib.h>
#include <ctype.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
#include "esp_crt_bundle.h"
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "esp_http_client.h"

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048
static const char *TAG = "HTTP_CLIENT";

namespace esphome {
namespace ota_http {

int OtaHttpIDF::http_init() {
  esp_http_client_config_t config = {.url = this->url_.c_str(),
                                     .method = HTTP_METHOD_GET,
                                     .timeout_ms = (int) this->timeout_,
                                     .buffer_size = MAX_HTTP_RECV_BUFFER};
  this->client_ = esp_http_client_init(&config);
  esp_err_t err;
  if ((err = esp_http_client_open(this->client_, 0)) != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
    return 0;
  }
  this->body_length = esp_http_client_fetch_headers(this->client_);

  ESP_LOGD(TAG, "body_length: %d , esp_http_client_get_content_length: %d", this->body_length,
           esp_http_client_get_content_length(this->client_));

  if (this->body_length <= 0) {
    ESP_LOGE(TAG, "Incorrect file size (%d) reported by http server (http status: %d). Aborting", this->body_length,
             err);
    return -1;
  }
  return 1;
}

size_t OtaHttpIDF::http_read(uint8_t *buf, const size_t max_len) {
  size_t bufsize = std::min(max_len, this->body_length - this->bytes_read);
  int read_len = esp_http_client_read(this->client_, (char *) buf, bufsize);
  if (read_len <= 0) {
    ESP_LOGE(TAG, "Error read data");
  } else {
    this->bytes_read += bufsize;
    buf[bufsize] = '\0';  // not fed to ota
  }
  // ESP_LOGVV(TAG, "Read %d bytes, %d remainings", read_len, this->body_length - this->bytes_read);

  return (size_t) read_len;  // FIXME size_t dosen't allow < 0
}

void OtaHttpIDF::http_end() {
  esp_http_client_close(this->client_);
  esp_http_client_cleanup(this->client_);
}

}  // namespace ota_http
}  // namespace esphome

#endif  // USE_ESP_IDF
