#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "cloud.h"
#include "logger.h"

CURL *cloud_point;

struct resp_data {
    char *memory;
    size_t size;
};

struct resp_data data_resp;

static size_t get_resp_cb(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct resp_data *mem = (struct resp_data *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if(!ptr) {
        fprintf(stderr, "Not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    log_info("Response:\n%s\n", mem->memory);

    return realsize;
}

int
init_cloud_conn(void)
{
  log_info("Initialising connection with %s endpoint\n", ENDPOINT_HTTPS);
  CURLcode result;

  data_resp.memory = malloc(1025);
  data_resp.size = 1024;

  result = curl_global_init(CURL_GLOBAL_ALL);

  if (result != CURLE_OK)
  {
    return (int)result;
  }

  cloud_point = curl_easy_init();
  if (cloud_point) {
    curl_easy_setopt(cloud_point, CURLOPT_URL, HEALTH_ENDPOINT);
    curl_easy_setopt(cloud_point, CURLOPT_HTTPGET, 1L);

    struct curl_slist *headers = NULL;

    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(cloud_point, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(cloud_point, CURLOPT_WRITEFUNCTION, get_resp_cb);
    curl_easy_setopt(cloud_point, CURLOPT_WRITEDATA, (void *)data_resp.memory);

    result = curl_easy_perform(cloud_point);

    if (result != CURLE_OK)
    {
      log_err("curl_easy_perform() failed: %s\n",
            curl_easy_strerror(result));
    }
  }

  return (int)result;
}

void
deinit_cloud_conn(void)
{
  curl_easy_cleanup(cloud_point);
  curl_global_cleanup();
}


// TODO: make more generic function, based probably on some defines, for now it's fine
int
cloud_post_telemetry(const char *device_name, uint16_t soil_moisture, uint8_t water_lvl,
                     uint8_t battery_lvl, uint32_t uptime)
{
  CURLcode result = CURLE_OK;

  struct curl_httppost *post = NULL;

  char json_data[256];

  sprintf(json_data, "{"
      "\"device_id\": \"%s\","
      "\"soil_moisture\": %u,"
      "\"battery_level\": %u,"
      "\"water_tank_ok\": %u,"
      "\"timestamp\": %u"
  "}", device_name, soil_moisture, battery_lvl, water_lvl, uptime);

  log_info("Posting: %s\n", json_data);

  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "Content-Type: application/json");
  
  curl_easy_setopt(cloud_point, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(cloud_point, CURLOPT_VERBOSE, 1L);
  curl_easy_setopt(cloud_point, CURLOPT_URL, TELEMETRY_ENDPOINT);
  curl_easy_setopt(cloud_point, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(cloud_point, CURLOPT_POSTFIELDS, json_data);

  result = curl_easy_perform(cloud_point);

  return result;
}
