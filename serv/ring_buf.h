#pragma once

#include <string.h>

#define RINGBUF_STRUCT_COUNT 64

#define DECLARE_RINGBUF_W_STRUCT(NAME, STORE_STRUCT) \
  typedef struct NAME##_ring_buf { \
    uint8_t cur; \
    uint8_t first; \
    STORE_STRUCT data[RINGBUF_STRUCT_COUNT]; \
  } NAME##_ring_buf_t; \
    \
  extern NAME##_ring_buf_t NAME##_buf; \
  STORE_STRUCT *NAME##_ringbuf_pop(void); \
  int NAME##_ringbuf_push(STORE_STRUCT *data); \
  void NAME##_ringbuf_init(void);

#define DEFINE_RINGBUF(NAME, STORE_STRUCT) \
  NAME##_ring_buf_t NAME##_buf; \
    \
  STORE_STRUCT *NAME##_ringbuf_pop(void) \
  { \
    if (NAME##_buf.cur == NAME##_buf.first) { \
      return NULL; \
    } \
    \
    return &NAME##_buf.data[NAME##_buf.first++]; \
  } \
    \
  int NAME##_ringbuf_push(STORE_STRUCT *data) \
  { \
    uint8_t next_cur = (NAME##_buf.cur + 1) % RINGBUF_STRUCT_COUNT; \
    if (next_cur == NAME##_buf.first) { \
      return -1; \
    } \
    \
    NAME##_buf.cur = next_cur; \
    memcpy(&(NAME##_buf.data[NAME##_buf.cur]), data, sizeof(STORE_STRUCT)); \
    return 0; \
  } \
    \
  void NAME##_ringbuf_init(void) \
  { \
    NAME##_buf.cur = 0; \
    NAME##_buf.first = 0; \
  }
