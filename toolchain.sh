#! /bin/bash

TOOLCHAIN_USER="${TOOLCHAIN_USER:reason}"

mkdir -p .esy
docker run --rm -it \
  --mount type=bind,source="$(pwd)",target=/app \
  reason-mobile:latest su reason -l
