#!/bin/sh
PATH_TO_REPO=$1
PATH_COREDUMP=$2
make -C "$PATH_TO_REPO/platforms/esp8266" debug_coredump  \
  CONSOLE_LOG=$PWD/$PATH_COREDUMP ELF_FILE=$(ls $PWD/build/objs/*.elf)\
  BIN_FILE=$(ls $PWD/build/objs/*.bin)
