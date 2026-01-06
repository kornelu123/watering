#!/bin/sh

set -e

sudo dd if=build/combined.uf2 of=/dev/sda oflag=sync status=progress
