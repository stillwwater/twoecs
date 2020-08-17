#include "benchmark/benchmark.h"

#define TWO_ENTITY_MAX 32<<10
#include "../entity.h"

// Some dummy components
struct A { int data; };
struct B { int data; };
struct C { int data; };
struct D { int data; };

template <typename... Components>
static void make_entities(two::World *world, int n) {
    for (int i = 0; i < n; ++i) {
        auto entity = world->make_entity();
        (world->pack(entity, Components{}), ...);
    }
}

template <typename... Components>
static void BM_ViewCached(benchmark::State &state) {
    two::World world;
    make_entities<Components...>(&world, state.range(0));
    world.view<Components...>();

    for (auto _ : state) {
        auto entities = world.view<Components...>();
        benchmark::DoNotOptimize(entities.data());
        benchmark::ClobberMemory();
    }
}
BENCHMARK_TEMPLATE(BM_ViewCached, A)
    ->RangeMultiplier(2)->Range(512, 16<<10);

BENCHMARK_TEMPLATE(BM_ViewCached, A, B)
    ->RangeMultiplier(2)->Range(512, 16<<10);

BENCHMARK_TEMPLATE(BM_ViewCached, A, B, C, D)
    ->RangeMultiplier(2)->Range(512, 16<<10);

static void BM_Lambda(benchmark::State &state) {
    two::World world;
    make_entities<A, B>(&world, state.range(0));
    world.view<A, B>();

    for (auto _ : state) {
        world.each<A, B>([](A &a, B &b) {
            benchmark::DoNotOptimize(a);
            benchmark::DoNotOptimize(b);
        });
    }
}
BENCHMARK(BM_Lambda)
    ->RangeMultiplier(2)->Range(512, 4<<10)->Unit(benchmark::kMillisecond);

static void BM_LambdaWithCapture(benchmark::State &state) {
    two::World world;
    make_entities<A, B>(&world, state.range(0));
    world.view<A, B>();

    char data[16];
    benchmark::DoNotOptimize(data);

    for (auto _ : state) {
        world.each<A, B>([&data](A &a, B &b) {
            benchmark::DoNotOptimize(a);
            benchmark::DoNotOptimize(b);
            benchmark::DoNotOptimize(data);
        });
    }
}
BENCHMARK(BM_LambdaWithCapture)
    ->RangeMultiplier(2)->Range(512, 4<<10)->Unit(benchmark::kMillisecond);

template <typename... Components>
static void BM_ViewPreCache(benchmark::State &state) {
    for (auto _ : state) {
        state.PauseTiming();
        // Allocate on heap so we can manually delete the world
        // so that the destructor is not included in the benchmark.
        auto world = new two::World;
        make_entities<Components...>(world, state.range(0));
        state.ResumeTiming();

        auto entities = world->view<Components...>();
        benchmark::DoNotOptimize(entities.data());
        benchmark::ClobberMemory();

        state.PauseTiming();
        delete world;
        state.ResumeTiming();
    }
}
BENCHMARK_TEMPLATE(BM_ViewPreCache, A)
    ->RangeMultiplier(2)->Range(512, 4<<10)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_ViewPreCache, A, B)
    ->RangeMultiplier(2)->Range(512, 4<<10)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_ViewPreCache, A, B, C, D)
    ->RangeMultiplier(2)->Range(512, 4<<10)->Unit(benchmark::kMillisecond);

static void BM_Unpack(benchmark::State &state) {
    two::World world;
    make_entities<A>(&world, state.range(0));

    for (auto _ : state) {
        state.PauseTiming();
        auto entities = world.view<A>();
        state.ResumeTiming();

        for (auto entity : entities) {
            auto &a = world.unpack<A>(entity);
            benchmark::DoNotOptimize(a);
        }
    }
}
BENCHMARK(BM_Unpack)->Range(1, 4<<10);
