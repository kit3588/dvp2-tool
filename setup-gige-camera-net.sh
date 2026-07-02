#!/bin/sh
# Configure RK3588 Ethernet ports for the DVP2 GigE camera subnet.

set -eu

CAMERA_IFACE="${CAMERA_IFACE:-end1}"
CAMERA_ADDR="${CAMERA_ADDR:-192.168.1.200/25}"

MODEBUS_IFACE="${MODEBUS_IFACE:-end0}"
MODEBUS_ADDR="${MODEBUS_ADDR:-192.168.1.100/25}"

configure_iface() {
	iface="$1"
	addr="$2"

	if [ ! -d "/sys/class/net/$iface" ]; then
		echo "skip: interface $iface not found"
		return 0
	fi

	ip link set "$iface" up

	if ip addr show dev "$iface" | grep -q " $addr"; then
		echo "ok: $iface already has $addr"
	else
		ip addr add "$addr" dev "$iface" 2>/dev/null || true
		echo "ok: configured $iface $addr"
	fi
}

configure_iface "$CAMERA_IFACE" "$CAMERA_ADDR"
configure_iface "$MODEBUS_IFACE" "$MODEBUS_ADDR"

# Configure camera IP via gige-ip tool (first-time setup).
# Skipped if camera is already at CAMERA_IP or if gige-ip is not found.
GIGE_IP_TOOL="${GIGE_IP_TOOL:-$(dirname $0)/bin/gige-ip}"
CAMERA_IP="${CAMERA_IP:-192.168.1.222}"
CAMERA_MASK="${CAMERA_MASK:-255.255.255.128}"
CAMERA_GW="${CAMERA_GW:-192.168.1.129}"

configure_camera_ip() {
    if [ ! -x "$GIGE_IP_TOOL" ]; then
        echo "skip: gige-ip tool not found at $GIGE_IP_TOOL"
        return 0
    fi

    # Give camera a moment to become visible after link up
    sleep 2

    # Enumerate cameras
    output=$("$GIGE_IP_TOOL" list 2>&1) || true
    sn=$(echo "$output" | awk '/SN:/{print $2; exit}')

    if [ -z "$sn" ]; then
        echo "skip: no camera found"
        return 0
    fi

    current_ip=$(echo "$output" | awk '/IP:/{print $2; exit}')

    if [ "$current_ip" = "$CAMERA_IP" ]; then
        echo "ok: camera $sn already at $CAMERA_IP"
        return 0
    fi

    echo "configuring camera $sn: $current_ip -> $CAMERA_IP"
    "$GIGE_IP_TOOL" set "$sn" --ip "$CAMERA_IP" --mask "$CAMERA_MASK" --gw "$CAMERA_GW" --mode PERSISTENT || \
        echo "warn: gige-ip set failed (camera may need power cycle to apply)"
}

configure_camera_ip
