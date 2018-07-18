#!/bin/sh

# set DEVELOPMENT to yes if you want to be able to get cmdline
# access to the console
# never check in with DEVELOPMENT set to yes !!!!
DEVELOPMENT=no

echo "You reached a dead end."
echo "It is not possible to login to Anvil via the console"

echo "DEVELOPMENT=$DEVELOPMENT"
if [ "$DEVELOPMENT" = "yes" ]; then
	echo "But as this is a development build we'll make an exception"
	exec /bin/ash --login
fi
# wait until stdout is flushed
sleep 1
