#define CRC_SEED 0x00000000

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "hardware/dma.h"

#include "crc.h"

uint32_t crc32(const void *data, size_t len, uint32_t crc)
{
    // Nothing else is running on the system, so it doesn't matter which
    // DMA channel we use
    static const uint8_t channel = 0;

    uint8_t dummy;

    dma_channel_config c = dma_channel_get_default_config(channel);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_sniff_enable(&c, true);

    dma_sniffer_enable(channel, 0x0F, true);
    dma_hw->sniff_data = crc;

    dma_channel_configure(
        channel,
        &c,
        &dummy,
        data,
        len,
        true
    );

    dma_channel_wait_for_finish_blocking(channel);

    return(dma_hw->sniff_data);
}
