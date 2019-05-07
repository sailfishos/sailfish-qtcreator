#!/bin/bash

NBSP=$'\xC2\xA0'
nonbreakables=(
    "Sailfish IDE"
    "Sailfish OS"
    "Sailfish SDK"
    "Qt Creator"
    )

expression=
for nonbreakable in "${nonbreakables[@]}"; do
    expression+="s/$nonbreakable/${nonbreakable// /$NBSP}/g;"
done

sed -i~ -e "$expression" "$(dirname "$0")"/*.json
