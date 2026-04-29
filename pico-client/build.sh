#!/bin/sh

if [[ -z ${WIFI_SSID} ]]; then
  echo "Set WIFI_SSID env variable as WIFI name";
  echo "f.e.: export WIFI_SSID=wifi_pigeon"
  exit -1
fi

if [[ -z ${WIFI_PASS} ]]; then
  echo "Set WIFI_PASS env variable as WIFI password";
  echo "f.e.: export WIFI_PASS=super_duper_pass"
  exit -1
fi

if [[ -z ${TEST_TCP_SERVER_IP} ]]; then
  echo "Set TEST_TCP_SERVER_IP env variable as IP of tcp server (if running on your machine, then your machine ip"
  echo "f.e.: export TEST_TCP_SERVER_IP=2.1.3.7"
  exit -1
fi

if [[ -z ${DISPLAYER_IP} ]]; then
  echo "Set DISPLAYER_IP env variable as IP of the e-ink display"
  echo "f.e.: export DISPLAYER_IP=192.168.4.1"
  exit -1
fi

if [[ -z ${CLOUD_IP} ]]; then
  echo "Set CLOUD_IP env variable as IP of the cloud server"
  echo "f.e.: export CLOUD_IP=192.168.1.1"
  exit -1
fi

if [[ -z ${ACTIVE_PUMPS_MASK} ]]; then
  echo "Set ACTIVE_PUMPS_MASK env variable (e.g., 0xFF for all 8 pumps, 0x07 for first 3 pumps)"
  echo "f.e.: export ACTIVE_PUMPS_MASK=0x07"
  exit -1
fi

cmake -B build -S . -DCMAKE_EXPORT_COMPILE_COMMANDS=Y -GNinja -DPICO_BOARD=pico_w \
  -DCMAKE_BUILD_TYPE=Release \
  -DPICO_SDK_PATH=${PICO_SDK_PATH} -DPICO_EXTRAS_PATH=${PICO_EXTRAS_PATH} \
  -DWIFI_SSID="${WIFI_SSID}" -DWIFI_PASS="${WIFI_PASS}" \
  -DTEST_TCP_SERVER_IP="${TEST_TCP_SERVER_IP}" \
  -DDISPLAYER_IP="${DISPLAYER_IP}" \
  -DCLOUD_IP="${CLOUD_IP}" \
    -DACTIVE_PUMPS_MASK="${ACTIVE_PUMPS_MASK}" \
  -DCRC_SEED=0xDEADBEEF \
