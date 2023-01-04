#!/bin/sh
util/size2bin n64.sfs disk.size.bin
util/size2bin original.vmlinux.32 vmlinux.size.bin
n64tool -h /n64_toolchain/mips64-elf/lib/header -t "Linux               " -o original.repack.z64 original.bl -s 1048568B disk.size.bin -s 1048572B vmlinux.size.bin -s 1M original.vmlinux.32 -s $(util/size2bin original.vmlinux.32)B n64.sfs
chksum64 original.repack.z64
