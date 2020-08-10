#include "../entity.h"

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
        world.collect_unused_entities();
    }
    world.destroy_systems();
    world.unload();
    return 0;
}
