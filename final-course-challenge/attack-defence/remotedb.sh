#!/bin/sh

PORT=10000
ROOT=/opt/remotedb
BIN=$ROOT/remotedb
FLAG=$ROOT/flag.txt
DB=$ROOT/db

case "$1" in
	start)
		$BIN -d -f $DB -l $FLAG -p $PORT
		;;
	stop)
		kill $(cat /var/run/remotedb.pid)
		;;
	*)
		echo "Usage: $0 {start|stop}"
esac
