#! /bin/bash

TOOLCHAIN_USER="${TOOLCHAIN_USER:reason}"

./update.sh
docker build -f Dockerfile.base -t reason-mobile:base context
docker build --build-arg TOOLCHAIN_USER=reason -t reason-mobile:latest - < Dockerfile