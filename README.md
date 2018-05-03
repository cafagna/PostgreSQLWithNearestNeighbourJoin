# PostgreSQLWithNearestNeighbourJoin
PostgreSQL 9.3 including my implementation of the Nearest Neighbour Join operator.

For each outer tuple r, find the inner tuple s that belongs to the same group G, and holds either equality on E or similarity on T. Three different queriy types are provided (see point 4)


1) Check out the code and compile it:
```
make && make install
```

2) Create the DB and some test data:
```
./init.sh
```


3) Start the DB Server and a client:
```
./start.sh
```


4) Run any of the following SQL queries:
```
select * from rt rnnj st equal on E nn by G using T;
```
_For each outer tuple r, find the inner tuple s that belongs to the same group G, and holds either equality on E or similarity on T. The last produced join matches are temporarly stored in a memory buffer, in case they are also a match for the next outer tuples._


```
select * from rt rnnj st equal on E nn by G using T backtrack;
```
_No semantic difference with the first query, but it refetches m times an inner tuple that is join match for m different outer tuples._


```
select * from rt rnnj st equal on E nn by G using T max M;
select * from rt rnnj st equal on E nn by G using T min M;
select * from rt rnnj st equal on E nn by G using T avg M;
```
_It aggregates ties using min, max, avg in case multiple nearest neighbours are found._
