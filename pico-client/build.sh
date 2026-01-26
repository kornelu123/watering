#!/bin/sh

if [[ -z ${WIFI_SSID} ]]; then
  echo "Set WIFI_SSID env variable as WIFI name";
  echo "f.e.: export WIFI_SSID=wifi_pigeon"
  exit -1
fi

if [[ -z ${WIFI_PASS} ]]; then
  echo "Set WIFI_SSID env variable as WIFI password";
  echo "f.e.: export WIFI_PASS=super_duper_pass"
  exit -1
fi

if [[ -z ${TEST_TCP_SERVER_IP} ]]; then
  echo "Set TEST_TCP_SERVER_IP env variable as IP of tcp server (if running on your machine, then your machine ip"
  echo "f.e.: export TEST_TCP_SERVER_IP=2.1.3.7"
  exit -1
fi

cmake -B build -S . -DCMAKE_EXPORT_COMPILE_COMMANDS=Y -GNinja -DPICO_BOARD=pico_w \
  -DCMAKE_BUILD_TYPE=Release \
  -DPICO_SDK_PATH=${PICO_SDK_PATH} -DPICO_EXTRAS_PATH=${PICO_EXTRAS_PATH} \
  -DWIFI_SSID="${WIFI_SSID}" -DWIFI_PASS="${WIFI_PASS}" \
  -DTEST_TCP_SERVER_IP="${TEST_TCP_SERVER_IP}" \
  -DCRC_SEED=0xDEADBEEF \
