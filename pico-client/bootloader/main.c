#include "RP2040.h"

#include "crc.c"
#include "slot.h"

#include "hardware/resets.h"
#include "hardware/regs/m0plus.h"
#include "pico/binary_info.h"

bi_decl(bi_program_version_string("1.01"));

int main(void);

//****************************************************************************
// This is normally provided as part of pico_stdlib so we have to provide it
// here if not we're not using it.
void exit(int ret)
{
  (void)ret;
  while(true)
    tight_loop_contents();
}

//****************************************************************************
// Replace the standard 'atexit' with an empty version to avoid pulling in
// additional code that we don't need anyway.
int atexit(void *a, void (*f)(void*), void *d)
{
  (void)a;
  (void)f;
  (void)d;
  return 0;
}

void jump_to_app(watering_slot_t *slot)
{
  asm volatile (
      "mov r0, %[start]\n"
      "ldr r1, =%[vtable]\n"
      "str r0, [r1]\n"
      "ldmia r0, {r0, r1}\n"
      "msr msp, r0\n"
      "bx r1\n"
      :
      : [start] "r" ((uint32_t )slot + 0x100), [vtable] "X" (PPB_BASE + M0PLUS_VTOR_OFFSET)
      :
      );
}

int main(void)
{
  unreset_block_wait(RESETS_RESET_DMA_BITS);

  if (validate_crc(slots[2])) {

    reset_block(RESETS_RESET_DMA_BITS);

    jump_to_app(slots[2]);
  }
}
