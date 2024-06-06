## With caching

```
Running 10s test @ http://localhost:8080
  10 threads and 10 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     1.89ms    2.86ms  62.65ms   96.90%
    Req/Sec   625.68    157.83     2.23k    79.72%
  Latency Distribution
     50%    1.43ms
     75%    2.14ms
     90%    3.06ms
     99%    9.14ms
  62362 requests in 10.10s, 16.27MB read
  Socket errors: connect 0, read 9314, write 0, timeout 0
  Non-2xx or 3xx responses: 53048
Requests/sec:   6174.37
Transfer/sec:      1.61MB
```

## Without caching

```
Running 10s test @ http://localhost:8080
  10 threads and 10 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     1.99ms    1.84ms  28.48ms   89.48%
    Req/Sec   579.77    192.55     1.33k    73.61%
  Latency Distribution
     50%    1.47ms
     75%    2.48ms
     90%    3.92ms
     99%    9.40ms
  54982 requests in 10.09s, 15.10MB read
  Socket errors: connect 0, read 17371, write 0, timeout 0
  Non-2xx or 3xx responses: 40610
Requests/sec:   5745.32
Transfer/sec:      1.50MB
```