#pragma once

#include <stdint.h>

#include <curl/curl.h>

#ifndef ENDPOINT_HTTPS
  #define ENDPOINT_HTTPS "https://watering-app.duckdns.org"
#endif

#define HEALTH_ENDPOINT ENDPOINT_HTTPS"/health"
#define TELEMETRY_ENDPOINT ENDPOINT_HTTPS"/api/v1/telemetry"

extern CURL *cloud_point;

int init_cloud_conn(void);
void deinit_cloud_conn(void);
int cloud_post_telemetry(const char *device_name, uint16_t soil_moisture, uint8_t water_lvl,
                     uint8_t battery_lvl, uint32_t uptime);
