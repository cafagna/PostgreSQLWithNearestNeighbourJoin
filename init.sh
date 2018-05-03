#!/bin/sh
./server/bin/initdb -D data
sleep 2
./server/bin/pg_ctl -D data -o "-F -i -p 5495" start
sleep 2
./server/bin/createdb -p 5495 myDB
sleep 2
./server/bin/psql -p 5495 -d myDB -f sql.txt
sleep 2
./server/bin/pg_ctl -D data -o "-F -p 5495" stop
