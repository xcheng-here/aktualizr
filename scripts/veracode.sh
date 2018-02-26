#!/bin/bash
set -euo pipefail
set -x

cd "$(dirname "$(readlink -f $0)")/.."

declare -r outdir="$1"
declare -r img='aktualizr/fedora:latest'

rm -rf "$outdir"
mkdir -p "$outdir"

docker build -t "$img" -f Dockerfile.fedora .
docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/src" -w /src "$img"

cp build/src/aktualizr "$outdir"

container=$(docker create "$img")
declare -r container

trap "docker rm -f $container" EXIT

shared=$(docker run --rm -it -v $PWD:/src/ -w /src "$img" ldd build/src/aktualizr | grep = | cut -d '>' -f 2 | cut -d '(' -f 1 | cut -d / -f 3)

# these are the external files needed by veracode
#for so in 'libarchive.so' 'libssl.so' 'libsqlite3.so' 'libostree-1.so' 'libsodium.so'; do
for so in $shared; do
  docker cp -L "$container:/lib64/$so" "$outdir/$so"
done
