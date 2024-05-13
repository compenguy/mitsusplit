#!/bin/bash
set -e
SCRIPT_DIR="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"
SCRIPT_NAME="$(basename "$0")"
die() { echo "$*" 1>&2; exit 1; }

ESP_IDF_DIR="${HOME}/.local/share/esp/esp-idf"
ESP_PORT="/dev/ttyACM0"

usage() {
cat << EOF
Usage: ${SCRIPT_NAME} [build] [flash] [monitor]
General Arguments:
  -h, --help		explanation of command line argument and environment
            		variable options
Build Arguments:
  build	invoke idf.py build
  flash	invoke idf.py -p ${ESP_PORT} flash
  flash	invoke idf.py -p ${ESP_PORT} monitor
EOF
}

while [[ $# -gt 0 ]]; do
	case $1 in
		build)
			shift
			DO_BUILD="true"
			;;
		flash)
			shift
			DO_FLASH="true"
			;;
		monitor)
			shift
			DO_MONITOR="true"
			;;
		-h|--help)
			usage
			exit 0
			;;
		*)
			usage
			echo ""
			die "Unrecognized argument '$1'"
			;;
	esac
done

function init_env() {
	if [ -z "${ENV_INITED}" ]; then
		source "${ESP_IDF_DIR}"/export.sh
		ENV_INITED="true"
	fi
}

function test_port() {
	if [ ! -e "${ESP_PORT}" ]; then
		die "${ESP_PORT} is missing, but is required for the current operation"
	fi
}

function do_build() {
	init_env
	idf.py build
}

function do_flash() {
	test_port
	init_env
	idf.py -p "${ESP_PORT}" flash
}

function do_monitor() {
	test_port
	init_env
	idf.py -p "${ESP_PORT}" monitor
}
if [ "${DO_BUILD}" = "true" ]; then
	do_build
fi
if [ "${DO_FLASH}" = "true" ]; then
	do_flash
fi
if [ "${DO_MONITOR}" = "true" ]; then
	do_monitor
fi
