#!/bin/bash -ex

# cd to kagome source root
cd "$(dirname "$0")/../../.."

BUILD_DIR=${BUILD_DIR:-$(pwd)/build}

BUILD_TYPE="${BUILD_TYPE:?BUILD_TYPE variable is not defined}"

if [ "$BUILD_TYPE" != "Debug" ] && [ "$BUILD_TYPE" != "Release" ]; then
  echo "Invalid build type $BUILD_TYPE, should be either Debug or Release"
  exit 1
fi

VERSION="${VERSION:?VERSION variable is not defined}"
# For github action we need remove ref prefix
if [ "$VERSION" = "refs/heads/master" ]; then
  VERSION=latest
elif [[ "$VERSION"  == refs/tags/* ]]; then
  VERSION="${VERSION#refs/tags/}"
else
  VERSION=$VERSION
fi

if [ "$BUILD_TYPE" = "Debug" ]; then
  VERSION="${VERSION}-debug"
fi

TAG="soramitsu/kagome:$VERSION"

CTX_DIR=${BUILD_DIR}/docker_context

# Cleanup docker context
rm -Rf ${CTX_DIR}
mkdir -p ${CTX_DIR}

# Copy binaries
cp -a ${BUILD_DIR}/node/kagome_full_syncing/kagome_full_syncing ${CTX_DIR}/
cp -a ${BUILD_DIR}/node/kagome_validating/kagome_validating  ${CTX_DIR}/


if [ "$BUILD_TYPE" = "Release" ]; then
  strip ${CTX_DIR}/kagome_full_syncing
  strip ${CTX_DIR}/kagome_validating

  docker build -t $TAG -f housekeeping/docker/kagome/minideb-release.Dockerfile ${CTX_DIR}

elif [ "$BUILD_TYPE" = "Debug" ]; then
  docker build -t $TAG -f housekeeping/docker/kagome/minideb-debug.Dockerfile ${CTX_DIR}

else
  echo "Unknown build type: $BUILD_TYPE"
  exit 1
fi

docker push $TAG

rm -R ${CTX_DIR}
