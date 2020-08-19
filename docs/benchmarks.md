# Benchmarks

All benchmarks were run on a Intel 7th gen cpu @4.0GHz with 32KB of L1 cache, and compiled with clang 10 with `-O3`.

[source](../test/entity_benchmark.cpp)

### Iterate `N` entities and unpack `M` components

| Number of Entities | 1 Component | 2 Components | 4 Components |
| ------------------ | ----------- | ------------ | ------------ |
| 256                | 0.002 ms    | 0.004 ms     | 0.008 ms     |
| 512                | 0.004 ms    | 0.008 ms     | 0.016 ms     |
| 4k                 | 0.034 ms    | 0.063 ms     | 0.127 ms     |
| 32k                | 0.265 ms    | 0.507 ms     | 1.02 ms      |
| 250k               | 2.16 ms     | 4.14 ms      | 8.24 ms      |
| 1M                 | 9.24 ms     | 18.0 ms      | 35.6 ms      |

```cpp
for (auto entity : world->view<Components...>()) {
    auto &a = world->unpack<A>(entity);
    // ...
}
```

### Iterate `N` entities and unpack `M` components (lambda version)

| Number of Entities | 1 Component | 2 Components |
| ------------------ | ----------- | ------------ |
| 256                | 0.002 ms    | 0.005 ms     |
| 512                | 0.005 ms    | 0.009 ms     |
| 4k                 | 0.038 ms    | 0.073 ms     |
| 32k                | 0.307 ms    | 0.592 ms     |
| 250k               | 2.45 ms     | 4.70 ms      |
| 1M                 | 9.83 ms     | 18.7 ms      |

```cpp
world->each<A, B>([](A &a, B &b) {
    // ...
});
```

### Create `N` entities and pack 1 component

| Number of Entities | Time     |
| ------------------ | -------- |
| 256                | 0.022 ms |
| 512                | 0.043 ms |
| 4k                 | 0.345 ms |
| 32k                | 2.80 ms  |

```cpp
for (int64_t i = 0; i < N; ++i) {
    world->pack(world->make_entity(), A{});
}
```

### Emit 2 events to `N` listeners

| Number of Listeners | Time     |
| ------------------- | -------- |
| 256                 | 0.004 ms |
| 512                 | 0.008 ms |
| 4k                  | 0.068 ms |
| 32k                 | 0.590 ms |
| 250k                | 4.35 ms  |
| 1M                  | 17.3 ms  |
