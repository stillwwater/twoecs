// SDL2 particle system example
//
// Controls:
//   * Press space to toggle gravity
//   * Move mouse to move emitter

#include <chrono>
#include <cstdlib>

#include "SDL.h"

#define TWO_ENTITY_MAX (32<<10)
#include "../entity.h"

// Config
constexpr int   WindowX      = 800;
constexpr int   WindowY      = 600;
constexpr float EmitterX     = WindowX / 2.f;
constexpr float EmitterY     = WindowY / 2.f;
constexpr float MinLifetime  = 1.f;
constexpr float MaxLifetime  = 5.f;
constexpr int   MaxParticles = 8192;
constexpr float MaxSpeed     = 200.f;
constexpr float MinSize      = 8.f;
constexpr float MaxSize      = 16.f;
constexpr float Gravity      = 1000.f;

static SDL_Renderer *gfx = nullptr;

// -- Utility -------------------------

struct float2 {
    float x, y;

    float2() = default;
    float2(float x, float y) : x{x}, y{y} {}
    explicit float2(float s) : x{s}, y{s} {}
};

float2 operator+(const float2 &a, const float2 &b) {
    return {a.x + b.x, a.y + b.y};
}

float2 operator*(const float2 &a, float s) {
    return {a.x * s, a.y * s};
}

float dot(const float2 &a, const float2 &b) {
    return a.x * b.x + a.y * b.y;
}

struct Color {
    uint8_t r, g, b, a;

    Color() = default;
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
        : r{r}, g{g}, b{b}, a{a} {}
};

static float randf() {
    return float(rand() / double(RAND_MAX));
}

static float randf(float a, float b) {
    return a + (b - a) * randf();
}

static float2 rand_dir(float scale = 1.f) {
    for (;;) {
        float2 v(randf(-1.f, 1.f), randf(-1.f, 1.f));
        if (dot(v, v) <= 1.f)
            return v * scale;
    }
}

static float remap(float a, float b, float c, float d, float x) {
    return c + ((d - c) / (b - a)) * (x - a);
}

// -- Events --------------------------

struct QuitEvent {};

struct KeyDown {
    SDL_Keycode key;
    SDL_Scancode scancode;
};

// -- Components ----------------------

struct Transform {
    float2 position;
    float2 scale;
};

struct Sprite {
    Color color;
};

struct Particle {
    float lifetime;
    float2 velocity;
};

struct Emitter {
    float2 origin;
    float gravity;
};

static void spawn_particle(two::World *world, two::Entity entity,
                           const float2 &origin) {
    world->pack(entity,
         Transform{origin, float2(randf(MinSize, MaxSize))},
         Sprite{{0xbb, 0xaa, 0xee, 0xff}},
         Particle{randf(MinLifetime, MaxLifetime), rand_dir(MaxSpeed)});
}

// -- Systems -------------------------

class ParticleSystem : public two::System {
public:
    void update(two::World *world, float dt) override {
        auto &emitter = world->unpack_one<Emitter>();
        world->each<Transform, Particle, Sprite>(
            [&](two::Entity entity, Transform &tf, Particle &p, Sprite &sp) {
                tf.position = tf.position + p.velocity * dt;
                p.lifetime -= dt;
                p.velocity.y += emitter.gravity * dt;

                sp.color.a = remap(MinLifetime - 1.f, MaxLifetime + 1.f,
                                   0, 255.f, p.lifetime);

                if (p.lifetime <= 0.f) {
                    spawn_particle(world, entity, emitter.origin);
                }
            });
    }
};

class SpriteRenderer : public two::System {
public:
    void load(two::World *) override {
        SDL_SetRenderDrawBlendMode(gfx, SDL_BLENDMODE_BLEND);
    }

    void draw(two::World *world) override {
        SDL_SetRenderDrawColor(gfx, 0, 0, 0, 255);
        SDL_RenderClear(gfx);

        int w, h;
        SDL_GetRendererOutputSize(gfx, &w, &h);

        world->each<Transform, Sprite>([w, h](Transform &tf, Sprite &sprite) {
            SDL_Rect dst{int(tf.position.x), int(tf.position.y),
                         int(tf.scale.x), int(tf.scale.y)};

            if (dst.x >= 0 && dst.y >= 0
                && dst.x + dst.w <= w && dst.y + dst.h <= h) {
                SDL_SetRenderDrawColor(gfx, sprite.color.r, sprite.color.g,
                                       sprite.color.b, sprite.color.a);
                SDL_RenderFillRect(gfx, &dst);
            }
        });
        SDL_RenderPresent(gfx);
    }
};

class MoveSystem : public two::System {
public:
    void update(two::World *world, float) override {
        int x, y;
        SDL_GetMouseState(&x, &y);
        world->each<Emitter>([x, y](Emitter &emitter) {
            emitter.origin = float2(x, y);
        });
    }
};

// -- World ---------------------------

class MainWorld : public two::World {
public:
    void load() override {
        make_system<SpriteRenderer>();
        make_system<MoveSystem>();
        make_system<ParticleSystem>();

        bind<KeyDown>(&MainWorld::keydown, this);

        pack(make_entity(), Emitter{float2(EmitterX, EmitterY), 0.f});
        for (int i = 0; i < MaxParticles; ++i) {
            spawn_particle(this, make_entity(), float2(EmitterX, EmitterY));
        }
    }

    void update(float dt) override {
        for (auto *system : systems()) {
            system->update(this, dt);
        }
    }

    bool keydown(const KeyDown &event) {
        if (event.key == SDLK_SPACE) {
            auto &emitter = unpack_one<Emitter>();
            emitter.gravity = emitter.gravity == 0.f ? Gravity : 0.f;
            return true;
        }
        return false;
    }
};

int main(int, char *[]) {
    auto *window = SDL_CreateWindow("Two ECS - Particles",
                                    SDL_WINDOWPOS_CENTERED,
                                    SDL_WINDOWPOS_CENTERED,
                                    WindowX, WindowY, 0);
    SDL_SetHint(SDL_HINT_RENDER_BATCHING, "1");
    gfx = SDL_CreateRenderer(
        window, 0, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    MainWorld world;
    world.load();

    auto frame_begin = std::chrono::high_resolution_clock::now();
    auto frame_end = frame_begin;
    bool running = true;
    SDL_Event e;

    world.bind<QuitEvent>([&running](const QuitEvent &) {
        running = false;
        return false;
    });

    while (running) {
        frame_begin = frame_end;
        frame_end = std::chrono::high_resolution_clock::now();
        auto dt_micro = std::chrono::duration_cast<std::chrono::microseconds>(
            (frame_end - frame_begin)).count();
        float dt = float(double(dt_micro) * 1e-6);

        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT:
                world.emit(QuitEvent{});
                break;
            case SDL_KEYDOWN:
                world.emit(KeyDown{e.key.keysym.sym, e.key.keysym.scancode});
                break;
            }
        }

        world.update(dt);
        for (auto *system : world.systems()) {
            system->draw(&world);
        }
        world.collect_unused_entities();
    }
    SDL_DestroyRenderer(gfx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
