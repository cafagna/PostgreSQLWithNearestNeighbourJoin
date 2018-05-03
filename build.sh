#!/bin/sh
./server/bin/pg_ctl -D db -o "-F -i -p 5400" stop
make 
make install
./server/bin/pg_ctl -D db -o "-F -i -p 5400" start
