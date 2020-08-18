# Benchmarks

All benchmarks were run on a Intel 7th gen cpu @4.0GHz with 32KB of L1 cache, and compiled with clang 10 with `-O3`.

[source](../test/entity_benchmark.cpp)

### Iterate `N` entities and unpack `M` components

| Number of Entities | 1 Component | 2 Components | 4 Components |
| ------------------ | ----------- | ------------ | ------------ |
| 256                | 0.004 ms    | 0.008 ms     | 0.016 ms     |
| 512                | 0.008 ms    | 0.016 ms     | 0.032 ms     |
| 4k                 | 0.065 ms    | 0.128 ms     | 0.254 ms     |
| 32k                | 0.607 ms    | 1.15 ms      | 2.19 ms      |
| 250k               | 5.15 ms     | 9.39 ms      | 17.8 ms      |
| 1M                 | 23.3 ms     | 41.1 ms      | 81.0 ms      |

```cpp
for (auto entity : world->view<Components...>()) {
    auto &a = world->unpack<A>(entity);
    // ...
}
```

### Iterate `N` entities and unpack `M` components (lambda version)

| Number of Entities | 1 Component | 2 Components |
| ------------------ | ----------- | ------------ |
| 256                | 0.004 ms    | 0.008 ms     |
| 512                | 0.008 ms    | 0.016 ms     |
| 4k                 | 0.064 ms    | 0.128 ms     |
| 32k                | 0.607 ms    | 1.15 ms      |
| 250k               | 4.99 ms     | 9.27 ms      |
| 1M                 | 21.43 ms    | 37.1 ms      |

```cpp
world->each<A, B>([](A &a, B &b) {
    // ...
});
```

### Create `N` entities and pack 1 component

| Number of Entities | Time     |
| ------------------ | -------- |
| 256                | 0.038 ms |
| 512                | 0.075 ms |
| 4k                 | 0.619 ms |
| 32k                | 5.12 ms  |

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
