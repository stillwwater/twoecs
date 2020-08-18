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

// An event
struct QuitEvent {};

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

            vel.value.x -= 0.01f * dt;
            vel.value.y -= 0.02f * dt;
            vel.value.z -= 0.04f * dt;

            if (vel.value.x <= 0.0f)
                world->emit(QuitEvent{});
        }
    }

    void draw(two::World *world) override {
        world->each<Transform>([](Transform &tf) {
            printf("(%f, %f, %f)\n",
                   tf.position.x, tf.position.y, tf.position.z);
        });
    }
    // May also override System::load(World *)
    // and System::unload(World *)
};

class MainWorld : public two::World {
public:
    void load() override {
        // This will call System::load(world)
        make_system<MoveSystem>();
        bind<QuitEvent>(&MainWorld::quit, this);

        pack(make_entity(),
             Transform{Vector3{0.0f, 0.0f, 0.0f}},
             Velocity{Vector3{0.1f, 0.2f, 0.4f}});
    }

    void update(float dt) override {
        // This may be done in your engine if all systems
        // are updated in the same way.
        for (auto *system : systems()) {
            system->update(this, dt);
        }
    }

    bool quit(const QuitEvent &) {
        printf("Done.\n");
        return false;
    }
    // May also override World::unload()
};

int main() {
    bool running = true;
    MainWorld world;

    world.bind<QuitEvent>([&running](const QuitEvent &) {
        running = false;
        // Returning false means the event will continue to propagate
        // to all functions bound to this event.
        return false;
    });
    world.load();

    while (running) {
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
