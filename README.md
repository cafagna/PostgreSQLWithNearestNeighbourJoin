# PostgreSQLWithNearestNeighbourJoin
PostgreSQL 9.3 including my implementation of the Nearest Neighbour Join operator.

For each outer tuple _r_, find the inner tuple _s_ that belongs to the same group __G__, and holds either equality on __E__ or similarity on __T__.

When to use this operator? When an equijoin on __G__ and __E__ provides too few results because the data is not dense enough. The idea is then to compute an equijoin on __G__ and __E__, and, for all the outer tuples for which an equijoin match is not found, compute a nearest neighbour search in the inner relation using the values of __T__. You might think to __G__ as the attributes that are strictly required to be equal, and to __E__ as the attributes that can be sacrificed when looking for a match.   
Three different queriy types are provided (see point 4)


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
select * from rt r rnnj st s equal on E nn by G using T;
```
_For each outer tuple r, find the inner tuple s that belongs to the same group __G__, and holds either equality on __E__ or similarity on __T__. The last produced join matches are temporarly stored in a memory buffer, in case they are also a match for the next outer tuples._


```
select * from rt r rnnj st s equal on E nn by G using T backtrack;
```
_No semantic difference with the first query, but it refetches m times an inner tuple that is join match for m different outer tuples._


```
select * from rt r rnnj st s equal on E nn by G using T max M;
select * from rt r rnnj st s equal on E nn by G using T min M;
select * from rt r rnnj st s equal on E nn by G using T avg M;
```
_It aggregates ties using min, max, avg in case multiple nearest neighbours are found._
