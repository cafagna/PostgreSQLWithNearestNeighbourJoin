#!/bin/sh
./server/bin/pg_ctl -D data -o "-F -i -p 5495" stop
./server/bin/pg_ctl -D data -o "-F -i -p 5495" start
sleep 2
./server/bin/psql  -p 5495 myDB
