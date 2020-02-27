#! /bin/bash

TOOLCHAIN_USER="${TOOLCHAIN_USER:reason}"

./update.sh
docker build -f Dockerfile.base -t eduardorfs/reason-mobile:base context
docker build --build-arg TOOLCHAIN_USER=reason -t eduardorfs/reason-mobile:latest - < Dockerfile