#!/bin/bash

make clean && make

#qemu-system-i386 -cdrom build/yegaos.iso -serial stdio
qemu-system-i386 -m 16M -machine pc,accel=kvm -cdrom build/yegaos.iso -serial stdio


