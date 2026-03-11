#!/bin/bash
set -e

cd /opt/loginserver

# Generate config.cfg from dist template if not present
if [ ! -f config.cfg ]; then
    cp config.cfg.dist config.cfg
fi

# Apply environment variable overrides to config
if [ -n "$QUERYMANAGER_PASSWORD" ]; then
    sed -i "s/^QueryManagerPassword.*/QueryManagerPassword = \"$QUERYMANAGER_PASSWORD\"/" config.cfg
fi
if [ -n "$GAME_WORLD_NAME" ]; then
    sed -i "s/^StatusWorld.*/StatusWorld = \"$GAME_WORLD_NAME\"/" config.cfg
fi

# Wait for querymanager to be reachable
echo "Waiting for querymanager on port 7173..."
for i in $(seq 1 60); do
    if nc -z 127.0.0.1 7173 2>/dev/null; then
        echo "querymanager is reachable."
        break
    fi
    sleep 1
done

echo "Starting login server..."
exec ./login
