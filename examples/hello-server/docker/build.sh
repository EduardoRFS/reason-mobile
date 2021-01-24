#! /bin/sh

cd $(dirname $0)/..
esy
node ../generate/dist/cli.js linux.musl.x86_64
esy @linux.musl.x86_64
CCOPT="-static" esy @linux.musl.x86_64 not-esy-setup dune build -x linux.musl.x86_64 @install
cp -f _esy/linux.musl.x86_64/build/install/default.linux.musl.x86_64/bin/hello-server docker

cd docker

chmod 777 hello-server
esy @linux.musl.x86_64 x86_64-unknown-linux-musl-strip hello-server

chmod 500 hello-server
docker build -t hello-server .

