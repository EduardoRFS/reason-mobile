#! /bin/bash

TOOLCHAIN_USER="${TOOLCHAIN_USER:reason}"

mkdir -p .esy
docker run --rm -it \
  --mount type=bind,source="$(pwd)",target=/app --network host \
  --cap-add=SYS_PTRACE --privileged=true --security-opt seccomp=unconfined \
  eduardorfs/reason-mobile:latest su $TOOLCHAIN_USER -l
