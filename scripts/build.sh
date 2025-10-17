#!/usr/bin/env bash

# submodule check
test -f "${PWD}/obuparse/Makefile" || git submodule update --init --recursive

# set defaults
CMAKE_FLAGS=('-DCMAKE_BUILD_TYPE=Release')
PREPEND=('bash' '-c')

# check for arguments
for arg in "$@"; do
	case "${arg}" in
	'asan')
		CMAKE_FLAGS+=('-DASAN=ON')
		;;
	'ubsan')
		CMAKE_FLAGS+=('-DUBSAN=ON')
		;;
	'valgrind')
		PREPEND=(
			'valgrind'
			'--track-fds=all'
			'--trace-children=yes'
			'--expensive-definedness-checks=yes'
		)
		;;
	'perf')
		PREPEND=('flamegraph' '--' "${PREPEND[@]}")
		;;
	'time')
		if command -v hyperfine >/dev/null 2>&1; then
			PREPEND=('hyperfine' '-N')
		else
			PREPEND+=('time')
		fi
		;;
	esac
done

# build
buildDir="${PWD}/build"
test -d "${buildDir}" && rm -rf "${buildDir}"
mkdir "${buildDir}"
(
	cmake \
		"${CMAKE_FLAGS[@]}" \
		-B "${buildDir}" || exit 1
	cmake \
		--build "${buildDir}" || exit 1
) >"${buildDir}"/build-log 2>&1

test $? -eq 0 || {
	cat "${buildDir}"/build-log
	exit 1
}

bin="${buildDir}/strip-obu-dv"

# test
TESTS=(
	# test no dv content
	"${bin} -i test-data/no-dv.obu"
	# test dv content
	"${bin} -i test-data/dv.obu -o test-data/dv-removed.obu"
	# test stripped dv output
	"${bin} -i test-data/dv-removed.obu"
)

# call first time just to prepare the binary
# windows/macos has slow start for first run
"${bin}" >/dev/null 2>&1

for test in "${TESTS[@]}"; do
	if [[ ${PREPEND[0]} == 'valgrind' ]]; then
		# intentional word splitting
		# shellcheck disable=SC2086
		"${PREPEND[@]}" ${test} || exit 1
	else
		"${PREPEND[@]}" "${test}" || exit 1
	fi
done
