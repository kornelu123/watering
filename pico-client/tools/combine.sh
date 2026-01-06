#!/bin/bash
set -e

BOOTLOADER_OFFSET=$(($1 - 0x10000000))
SLOT0_OFFSET=$(($2 - 0x10000000))
SLOT1_OFFSET=$(($3 - 0x10000000))
SLOT2_OFFSET=$(($4 - 0x10000000))

# Input BIN files
BOOTLOADER_BIN="bootloader/bootloader.bin"
SLOT0_BIN="waterer/watering_slot0.bin"
SLOT1_BIN="waterer/watering_slot1.bin"
SLOT2_BIN="waterer/watering_slot2.bin"

# Output files
COMBINED_BIN="combined.bin"
COMBINED_UF2="combined.uf2"

# Flash size (2MB for Pico W)
FLASH_SIZE=$((2 * 1024 * 1024))

# Slot size (512KB each)
SLOT_SIZE=$((512 * 1024))

echo "=== Combining BIN files into single UF2 ==="

# Create empty flash image
echo "Creating empty flash image of ${FLASH_SIZE} bytes..."
dd if=/dev/zero of=${COMBINED_BIN} bs=1 count=${FLASH_SIZE} status=none

# ... wcześniejsza część skryptu ...

# Function to calculate CRC32 using IEEE 802.3 polynomial
calculate_crc32_python() {
    local bin_file=$1
    python3 -c "
import sys
import struct

def swap32(value):
    return struct.unpack('<I', struct.pack('>I', value))[0]

def calculate_crc32_ieee8023(data, initial=0x00000000):
    '''Calculate CRC32 using IEEE 802.3 polynomial'''
    crc = initial

    for value in data:
      crc += value

      crc &= 0xFFFFFFFF

    return swap32(crc & 0xFFFFFFFF)

# Read file
with open('$bin_file', 'rb') as f:
    data = f.read()

# Calculate CRC32
crc = calculate_crc32_ieee8023(data)
print(f'{crc:08x}')
"
}

calculate_and_append_crc() {
    local bin_file=$1
    local slot_name=$2

    # Get original size
    local original_size=$(stat -c%s ${bin_file})

    # Check if slot is empty or too small
    if [ ${original_size} -eq 0 ]; then
        echo "  Warning: ${slot_name} is empty, skipping CRC calculation"
        echo ${bin_file}
        return
    fi

    # Check if slot fits in 512KB minus 4 bytes for CRC
    if [ ${original_size} -gt $((SLOT_SIZE - 4)) ]; then
        echo "  Error: ${slot_name} is too large for slot (${original_size} > $((SLOT_SIZE - 4)))"
        exit 1
    fi

    # Create temporary file with slot data
    local temp_file=$(mktemp)

    # Copy original data
    dd if=${bin_file} of=${temp_file} bs=1 count=${original_size} status=none

    # Calculate CRC32 of the original data
    local crc_value
    if command -v crc32 &> /dev/null; then
        crc_value=$(calculate_crc32_python "${temp_file}")
    else
        echo "  Error: crc32 command not found. Install with: brew install crc32 (macOS) or apt-get install libarchive-zip-perl (Linux)"
        exit 1
    fi

    # Pad the rest of the slot with zeros if needed
    local current_size=$((original_size))
    if [ ${current_size} -lt ${SLOT_SIZE} ]; then
        local padding_size=$((SLOT_SIZE - current_size - 4))
        dd if=/dev/zero of=${temp_file} bs=1 seek=${current_size} count=${padding_size} conv=notrunc status=none
    fi

    bytes=$(printf "%b" "$(echo "$crc_value" | sed 's/../\\x&/g')")
    # Append CRC32 bytes to the end of slot
    echo -n ${bytes} >> ${temp_file}

    echo "${temp_file}"
}

# Function to extract binary from BIN, add CRC32 and place at offset
extract_and_place_with_crc() {
    local bin_file=$1
    local offset=$2
    local name=$3

    echo "Processing ${name} at offset ${offset}..."

    local original_size=$(stat -c%s ${bin_file})
    printf "  Original size: %x bytes\n" ${original_size}

    # Calculate CRC32 and append to slot
    crc_file=$(calculate_and_append_crc "${bin_file}" "${name}")

    # Place at offset (convert hex to decimal for dd)
    local offset_dec=$(printf "%d" ${offset})
    dd if="${crc_file}" of="${COMBINED_BIN}" bs=1 seek="${offset_dec}" conv=notrunc status=none

    # Cleanup temporary files
    rm -f ${crc_file}
}

# Special function for bootloader (no CRC needed)
extract_and_place() {
    local bin_file=$1
    local offset=$2
    local name=$3

    echo "Processing ${name} at offset ${offset}..."
    local size=$(stat -c%s ${bin_file})
    printf "  Size: %x bytes\n" ${size}

    local offset_dec=$(printf "%d" ${offset})
    dd if=${bin_file} of=${COMBINED_BIN} bs=1 seek=${offset_dec} conv=notrunc status=none
}

# Process bootloader (no CRC)
extract_and_place ${BOOTLOADER_BIN} ${BOOTLOADER_OFFSET} "bootloader"

# Process application slots with RC32
extract_and_place_with_crc ${SLOT0_BIN} ${SLOT0_OFFSET} "slot0"
extract_and_place_with_crc ${SLOT1_BIN} ${SLOT1_OFFSET} "slot1"
extract_and_place_with_crc ${SLOT2_BIN} ${SLOT2_OFFSET} "slot2"

# Convert to UF2 using picotool
echo "Converting to UF2..."
picotool uf2 convert ${COMBINED_BIN} ${COMBINED_UF2}

# Get file info
echo -e "\n=== Combined UF2 Info ==="
picotool info ${COMBINED_UF2}
