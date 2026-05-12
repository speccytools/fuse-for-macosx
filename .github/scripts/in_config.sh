#!/bin/bash
echo -n "Verify configure line '$1' .. "
lines1=(`cat "./configure.out" | grep -c "$1"`)
if [[ "$lines1" -lt "1" ]]; then
  echo "NOT FOUND"
  exit 1
fi
echo "OK"
