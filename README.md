# Two ECS

A single header implementation of an Entity Component System ([ECS](https://en.wikipedia.org/wiki/Entity_component_system)). Since there isn’t a strict definition of what an ECS is, this library follows the definition presented in [this GDC 2017 talk](https://www.youtube.com/watch?v=W3aieHjyNvw&t=638s) on the implementation used in Overwatch. By this definition an Entity is just an id (aliased to `uint16_t` by default), a Component has no functionality, a System holds no data, and a World is a collection of Entities, Systems and Components.

This library is probably best used as a reference for learning, since if you’re implementing a game engine you’ll likely want to implement your own ECS anyways. The library is written in C++11 so it should compile on consoles, it also does not rely on RTTI and can be compiled with `-fno-rtti` and `-fno-exceptions` (or msvc equivalents).

A minimal example (for the complete API documentation see the [docs](./docs/README.md))

```cpp
// A simple 3D vector
struct Vector3 {
    float x, y, z;
};

// A transform component,
// note that we don't need to register the component type.
struct Transform {
    Vector3 position;
};

// And a velocity component. Any struct can be used as a component
// as long as it is copyable.
struct Velocity {
    Vector3 value;
};

class MoveSystem : public two::System {
public:
    void update(two::World *world, float dt) override {
        // Returns all entities that have both a Transform and
        // a Velocity component.
        for (auto entity : world->view<Transform, Velocity>()) {
            auto &tf = world->unpack<Transform>(entity);
            auto &vel = world->unpack<Velocity>(entity);
            tf.position.x += vel.value.x * dt;
            tf.position.y += vel.value.y * dt;
            tf.position.z += vel.value.z * dt;
        }
    }
    // May also override System::load(World *),
    // System::unload(World *) and System::draw(World *)
};

class MainWorld : public two::World {
public:
    void load() override {
        // This will call System::load(world)
        make_system<MoveSystem>();

        auto entity = make_entity();
        pack(entity, Transform{Vector3{0.0f, 0.0f, 0.0f}});
        pack(entity, Velocity{Vector3{0.1f, 0.2f, 0.4f}});
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
    for (int frame = 0; frame < 1000000; ++frame) {
        // Handle events
        // ...
        // Update world
        world.update(1.0f / 60.0f);
        // Render
        for (auto *system : world.systems()) {
            system->draw(&world);
        }
    }
    world.destroy_systems();
    world.unload();
    return 0;
}
```

## Performance

Entity component systems generally have very good performance, but given that the entity system is unlikely to be the bottleneck in an engine, this library prioritizes flexibility in the API over being the fastest it could be. That being said, the most common operations are optimized to have as little overhead as possible and are suitable to be called many times per frame. For example, retrieving a list of entities with given components (`view<Components...>()`) is under 200 instructions on amd64. The documentation goes over the performance implications of the more performance critical functions.

Two ECS relies heavily on hash tables, and a good chunk of CPU time in this implementation is used by `std::unordered_map` and `std::unordered_set`, so replacing these with a hash table with open addressing is likely to improve performance quite a bit due to better cache locality.

