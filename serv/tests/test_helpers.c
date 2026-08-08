#include <assert.h>
#include <stdint.h>

#include "proto.h"

int main(void)
{
  static const uint32_t values[] = {0, 1, 0x123456, 0xffffff};

  for (unsigned int i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
    uint8_t encoded[ADDR_SIZE] = {0};
    put_be24(values[i], encoded);
    assert(get_be24(encoded) == values[i]);
  }

  uint8_t bytes[ADDR_SIZE] = {0xab, 0xcd, 0xef};
  assert(get_be24(bytes) == 0xabcdef);
  return 0;
}
