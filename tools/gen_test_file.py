#!/bin/python3

file_bytes = bytes([0xAA, 0x55]*256*1024)

with open('test_file.bin', 'wb') as file:
    file.write(file_bytes)
