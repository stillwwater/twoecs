# Two ECS

A single header implementation of ECS. Since there isn’t a strict definition of what an ECS is, this library follows the definition presented in [this GDC 2017 talk](https://www.youtube.com/watch?v=W3aieHjyNvw) on the implementation used in Overwatch. By this definition an Entity is just an id (aliased to `uint32_t` by default), a Component has no functionality, a System holds no data, and a World is a collection of Entities, Systems and Components.

This library is probably best used as a reference for learning, since if you’re implementing a game engine you’ll likely want to implement your own ECS anyways. The library is written in C++11 and does not use RTTI or exceptions.

## A minimal example

For the complete API documentation see the [docs](./docs/README.md). For a more complete example see the [SDL2 implemementation of a simple particle system](./examples/example_sdl.cpp).

### Components

Any struct that is copy constructible and copy assignable can be used as a component. Components do not need to be registered before they are used.

```cpp
struct Position {
    float x, y;
};

struct Velocity {
    float x, y;
};
```

### Systems
```cpp
class MoveSystem : public two::System {
public:
    void update(two::World *world, float dt) override {
        // Returns all entities that have both a Transform and
        // a Velocity component.
        world->each<Transform, Velocity>([dt](Transform &tf, Velocity &vel) {
            tf.x += vel.x * dt;
            tf.y += vel.y * dt;
        });
    }
    // May also override System::load(World *), System::draw(World *),
    // and System::unload(World *)
};
```

Alternative to `world->each`:

```cpp
void draw(two::World *world) override {
    // Alternative to the callback based world->each
    for (auto entity : world->view<Transform>()) {
         auto &tf = world->unpack<Transform>(entity);
         printf("(%f, %f)\n", tf.x, tf.y);
    }
}
```

### Worlds

Worlds manage systems, entities, components and events. Worlds can be used to load/unload game assets and setup entities and systems.

```cpp
class MainWorld : public two::World {
public:
    void load() override {
        // This will call System::load(world)
        make_system<MoveSystem>();
        
        auto entity = make_entity();
        pack(entity, Transform{0.0f, 0.0f}, Velocity{0.1f, 0.2f});
    }

    void update(float dt) override {
        // This may be done in your engine if all systems
        // are updated in the same way.
        for (auto *system : systems()) {
            system->update(this, dt);
        }
    }
    // May also override World::unload()
};

int main() {
    MainWorld world;
    world.load();
    for (int frame = 0; frame < 100000; ++i) {
        // Handle events
        // ...
        // Update world
        world.update(1.0f / 60.0f);
        // Render
        for (auto *system : world.systems()) {
            system->draw(&world);
        }
        world.collect_unused_entities();
    }
    world.destroy_systems();
    world.unload();
    return 0;
}
```

### Events

Events can be used to communicate between systems. Each world has independent event channels. Just like components, any data type can be used as an event.

```cpp
struct KeyDown {
    int key;	
};

int main() {
    two::World world;
    world.bind<KeyDown>([](const KeyDown &event) {
        printf("Key <%d> was pressed\n", event.key);
        return true;
    });
    world.emit(KeyDown{0x20});
    return 0;
}
```

## Performance

[benchmarks](./docs/benchmarks.md)

Given that the entity system is unlikely to be the bottleneck in an engine, this library prioritizes flexibility in the API over being as fast as possible.

