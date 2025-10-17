#!/usr/bin/env bash

if ! command -v ffmpeg >/dev/null 2>&1; then
	echo 'missing ffmpeg, aborting...'
	exit 1
fi

if ! command -v strip-obu-dv >/dev/null 2>&1; then
	echo 'missing strip-obu-dv, aborting...'
	exit 1
fi

usage() {
	echo 'wrong usage'
	echo "$0 -i input -o output"
	exit 1
}

input=''
output=''

for arg in "$@"; do
	case "${arg}" in
	'-i')
		shift
		input="$1"
		;;
	'-o')
		shift
		output="$2"
		;;
	esac
done

test "${input}" == '' && usage
test "${output}" == '' && usage

if [[ -f "${output}" ]]; then
	echo "refusing to overwrite file: ${output}"
	exit 1
fi

obuFile="${output}.obu"
obuNoDvFile="${output}.obu.noDV"

ffmpeg \
	-hide_banner \
	-i "${input}" \
	-map 0:v \
	-c copy \
	-f obu \
	"${obuFile}" || exit 1

strip-obu-dv \
	-i "${obuFile}" \
	-o "${obuNoDvFile}" || exit 1
rm "${obuFile}"

ffmpeg \
	-hide_banner \
	-i "${input}" \
	-i "${obuNoDvFile}" \
	-map 0 \
	-map -0:v \
	-map 1 \
	-c copy \
	"${output}" || exit 1
rm "${obuNoDvFile}"
