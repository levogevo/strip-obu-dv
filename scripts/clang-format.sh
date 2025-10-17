#!/usr/bin/env bash

base="$(dirname "$(readlink -f "$0")")/.."

inotifywait -m \
	-e close_write \
	-e moved_to \
	--format '%w%f' \
	--include '.*\.c$' \
	"${base}" |
	while read -r file; do
		tmp="${file}-tmp"
		clang-format "${file}" >"${tmp}"
		if ! cmp "${file}" "${tmp}"; then
			mv "${tmp}" "${file}"
		fi
		test -f "${tmp}" && rm "${tmp}"
	done
