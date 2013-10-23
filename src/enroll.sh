#!/bin/bash

sed -i -e '/^#define TOTP_SECRET/d'  config.h

SEED=$(dd if=/dev/random bs=1 count=20 2>/dev/null | xxd -c 20 -g 2 -i)
TEXTSEED=$(echo -n "$SEED" | sed -e 's/[, ].0x//g')


echo "#define TOTP_SECRET {$SEED } " >> config.h
echo $TEXTSEED
