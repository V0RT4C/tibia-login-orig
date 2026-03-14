#!/bin/sh
set -e

cd /opt/loginserver

# Wait for querymanager to be reachable
QM_HOST="${SENJA_QM_HOST:-127.0.0.1}"
QM_PORT="${SENJA_QM_PORT:-7173}"
echo "Waiting for querymanager on $QM_HOST:$QM_PORT..."
i=0
while [ "$i" -lt 60 ]; do
    if nc -z "$QM_HOST" "$QM_PORT" 2>/dev/null; then
        echo "querymanager is reachable."
        break
    fi
    sleep 1
    i=$((i + 1))
done

echo "Starting login server..."
exec ./login
