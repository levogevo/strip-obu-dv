#!/bin/bash

buildDir="$PWD/build"
test -d "${buildDir}" || mkdir "${buildDir}"
(
	cmake \
		--fresh \
		-DCMAKE_BUILD_TYPE=Release \
		-B "${buildDir}" || exit 1
	cmake \
		--build "${buildDir}" || exit 1
) > "${buildDir}"/build-log 2>&1
test $? -eq 0 || { cat "${buildDir}"/build-log ; exit 1 ; }

# test no dv content
"${buildDir}"/strip-obu-dv -i test-data/no-dv.obu || exit 1

# test dv content
"${buildDir}"/strip-obu-dv -i test-data/dv.obu -o test-data/dv-removed.obu || exit 1

# test stripped dv output
"${buildDir}"/strip-obu-dv -i test-data/dv-removed.obu || exit 1

# for chunkSize in $(seq 400000 100000 10000000); do
# 	"${buildDir}"/strip-obu-dv -i snow.obu -c "${chunkSize}" && break
# done

