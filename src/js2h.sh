#!/bin/sh

echo '#define HINTS_JS "' | tr -d '\n'
cat $1 | \
    tr '\n\r\t' ' ' | \
    sed -e 's:/\*[^*]*\*/::g' \
    -e 's|[ ]\{2,\}| |g' \
    -e "s|[ ]\{0,\}\([-!?<>:=(){};+\&\"',\|]\)[ ]\{0,\}|\1|g" \
    -e 's|"+"||g' \
    -e 's|\\x20| |g' \
    -e 's|\\|\\\\|g' \
    -e 's|"|\\"|g' \
    -e 's|HINT_CSS|" HINT_CSS "|' \
    -e '$s/$/"/'
echo ""
