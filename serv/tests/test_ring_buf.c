#include <assert.h>
#include <stdint.h>

#include "ring_buf.h"

DECLARE_RINGBUF_W_STRUCT(number, uint32_t);
DEFINE_RINGBUF(number, uint32_t);

int main(void)
{
  number_ringbuf_init();
  assert(number_ringbuf_pop() == NULL);

  for (uint32_t i = 0; i < RINGBUF_STRUCT_COUNT - 1; ++i) {
    assert(number_ringbuf_push(&i) == 0);
  }

  uint32_t overflow = UINT32_MAX;
  assert(number_ringbuf_push(&overflow) == -1);

  for (uint32_t i = 0; i < RINGBUF_STRUCT_COUNT - 1; ++i) {
    uint32_t *value = number_ringbuf_pop();
    assert(value != NULL);
    assert(*value == i);
  }
  assert(number_ringbuf_pop() == NULL);

  /* Exercise index wraparound after the buffer has been drained. */
  for (uint32_t i = 100; i < 110; ++i) {
    assert(number_ringbuf_push(&i) == 0);
    assert(*number_ringbuf_pop() == i);
  }

  return 0;
}
