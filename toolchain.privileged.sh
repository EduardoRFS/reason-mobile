#! /bin/bash

TOOLCHAIN_USER="${TOOLCHAIN_USER:reason}"

docker run --rm -it \
  --mount type=bind,source="$(pwd)/app",target=/app \
  --network host \
  --cap-add=SYS_PTRACE --security-opt seccomp=unconfined \
  --name reason-mobile:latest su $TOOLCHAIN_USER -l
