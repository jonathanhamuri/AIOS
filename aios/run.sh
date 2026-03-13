#!/bin/bash
rm -f /tmp/serial.log
qemu-system-x86_64 -cdrom aios.iso -display none -serial file:/tmp/serial.log -no-reboot &
QPID=$!
sleep 5
kill $QPID 2>/dev/null
echo "=== SERIAL OUTPUT ==="
cat /tmp/serial.log
