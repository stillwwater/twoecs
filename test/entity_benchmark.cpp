#include <memory>

#include "benchmark/benchmark.h"

// With this many entities the World must be heap allocated
#define TWO_ENTITY_64
#define TWO_ENTITY_MAX ((1024<<10) + 1)
#include "../entity.h"

// Some dummy components
struct A { int64_t data; };
struct B { int64_t data; };
struct C { int64_t data; };
struct D { int64_t data; };

template <typename... Components>
static void make_entities(const std::unique_ptr<two::World> &world, int n) {
    for (int i = 0; i < n; ++i) {
        auto entity = world->make_entity();
        world->pack(entity, Components{}...);
    }
}

static void destroy_entities(const std::unique_ptr<two::World> &world) {
    auto &entities = world->unsafe_view_all();
    for (int64_t i = entities.size() - 1; i >= 0; --i) {
        auto entity = entities[i];
        if (entity != two::NullEntity)
            world->destroy_entity(entity);
    }
    world->collect_unused_entities();
}

template <typename... Components>
static void BM_IterateAndUnpack(benchmark::State &state) {
    std::unique_ptr<two::World> world(new two::World);
    make_entities<Components...>(world, state.range(0));

    for (auto _ : state) {
        for (auto entity : world->view<Components...>()) {
            TWO_TEMPLATE_FOLD(benchmark::DoNotOptimize(
                world->unpack<Components>(entity)));
        }
    }
}
BENCHMARK_TEMPLATE(BM_IterateAndUnpack, A)
    ->Range(256, 1024<<10)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_IterateAndUnpack, A, B)
    ->Range(256, 1024<<10)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_IterateAndUnpack, A, B, C, D)
    ->Range(256, 1024<<10)
    ->Unit(benchmark::kMillisecond);

static void BM_IterateLambda1(benchmark::State &state) {
    std::unique_ptr<two::World> world(new two::World);
    make_entities<A>(world, state.range(0));
    world->view<A>();

    for (auto _ : state) {
        world->each<A>([](A &a) {
            benchmark::DoNotOptimize(a);
        });
    }
}
BENCHMARK(BM_IterateLambda1)
    ->Range(256, 1024<<10)
    ->Unit(benchmark::kMillisecond);

static void BM_IterateLambda2(benchmark::State &state) {
    std::unique_ptr<two::World> world(new two::World);
    make_entities<A, B>(world, state.range(0));
    world->view<A, B>();

    for (auto _ : state) {
        world->each<A, B>([](A &a, B &b) {
            benchmark::DoNotOptimize(a);
            benchmark::DoNotOptimize(b);
        });
    }
}
BENCHMARK(BM_IterateLambda2)
    ->Range(256, 1024<<10)
    ->Unit(benchmark::kMillisecond);

template <typename... Components>
static void BM_View(benchmark::State &state) {
    std::unique_ptr<two::World> world(new two::World);
    make_entities<Components...>(world, state.range(0));

    for (auto _ : state) {
        auto entities = world->view<Components...>();
        benchmark::DoNotOptimize(entities.data());
        benchmark::ClobberMemory();
    }
}
BENCHMARK_TEMPLATE(BM_View, A)
    ->Range(256, 1024<<10)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_View, A, B)
    ->Range(256, 1024<<10)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_View, A, B, C, D)
    ->Range(256, 1024<<10)
    ->Unit(benchmark::kMillisecond);

static void BM_Contains(benchmark::State &state) {
    std::unique_ptr<two::World> world(new two::World);
    make_entities<A>(world, state.range(0));
    for (auto _ : state) {
        for (auto entity : world->unsafe_view_all()) {
            benchmark::DoNotOptimize(world->contains<A>(entity));
        }
    }
}
BENCHMARK(BM_Contains)
    ->Range(256, 1024<<10)
    ->Unit(benchmark::kMillisecond);

static void BM_CreateEntityAndPack(benchmark::State &state) {
    std::unique_ptr<two::World> world(new two::World);
    for (auto _ : state) {
        for (int64_t i = 0; i < state.range(0); ++i) {
            auto entity = world->make_entity();
            benchmark::DoNotOptimize(entity);
            world->pack(entity, A{});
        }
        state.PauseTiming();
        destroy_entities(world);
        state.ResumeTiming();
    }
}
BENCHMARK(BM_CreateEntityAndPack)
    ->Range(256, 32<<10)
    ->Unit(benchmark::kMillisecond);

static void BM_EmitEvent2(benchmark::State &state) {
    std::unique_ptr<two::World> world(new two::World);
    world->bind<A>([](const A &event) {
        benchmark::DoNotOptimize(event);
        return true;
    });
    world->bind<B>([](const B &event) {
        benchmark::DoNotOptimize(event);
        return true;
    });
    for (auto _ : state) {
        for (int64_t i = 0; i < state.range(0); ++i) {
            world->emit(A{12});
            world->emit(B{24});
        }
    }
}
BENCHMARK(BM_EmitEvent2)
    ->Range(256, 1024<<10)
    ->Unit(benchmark::kMillisecond);
