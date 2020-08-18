#include "gtest/gtest.h"

#define TWO_ASSERTIONS
#define TWO_PARANOIA
#include "../entity.h"

struct A { int data; };
struct B { int data; };
struct C { int data; };
struct D { int data; };

class SystemA : public two::System {};
class SystemB : public two::System {};

TEST(ECS_World, MakeEntity) {
    two::World world;
    auto entity = world.make_entity();
    EXPECT_NE(entity, two::NullEntity);
    EXPECT_TRUE(world.contains<two::Active>(entity));

    // Check that the NullEntity was created
    EXPECT_EQ(2, world.unsafe_view_all().size());
}

TEST(ECS_World, ComponentOperations) {
    two::World world;
    auto entity = world.make_entity();
    auto &a0 = world.pack(entity, A{});
    world.pack(entity, B{}, C{});
    EXPECT_TRUE((world.contains<A, B, C>(entity)));

    auto &a1 = world.unpack<A>(entity);
    ASSERT_EQ(&a0, &a1);

    world.remove<A>(entity);
    EXPECT_FALSE(world.contains<A>(entity));
    EXPECT_DEBUG_DEATH(world.unpack<A>(entity), "");

    // Removing a component that has already been removed is a no-op.
    EXPECT_NO_FATAL_FAILURE(world.remove<A>(entity));

    world.set_active(entity, false);
    EXPECT_FALSE(world.contains<two::Active>(entity));
    world.set_active(entity, true);
    EXPECT_TRUE(world.contains<two::Active>(entity));
}

TEST(ECS_World, EntityArchetype) {
    two::World world;
    // Archetypes don't need to be inactive, you can just
    // as well copy components from an active entity to another.
    auto archetype = world.make_inactive_entity();
    EXPECT_FALSE(world.contains<two::Active>(archetype));
    world.pack(archetype, A{8}, B{16}, C{32});

    auto entity = world.make_entity(archetype);
    EXPECT_TRUE(world.contains<two::Active>(entity));
    EXPECT_TRUE((world.contains<A, B, C>(entity)));
    EXPECT_EQ(8, world.unpack<A>(entity).data);
    EXPECT_EQ(16, world.unpack<B>(entity).data);
    EXPECT_EQ(32, world.unpack<C>(entity).data);
}

TEST(ECS_World, EntityReuse) {
    two::World world;
    auto e0 = world.make_entity();
    EXPECT_EQ(0, two::entity_version(e0));
    world.pack(e0, A{});
    world.destroy_entity(e0);
    EXPECT_FALSE(world.contains<A>(e0));
    world.collect_unused_entities();

    auto e1 = world.make_entity();
    EXPECT_EQ(two::entity_index(e0), two::entity_index(e1));
    EXPECT_NE(e0, e1);
    EXPECT_EQ(1, two::entity_version(e1));
    EXPECT_DEBUG_DEATH(world.unpack<A>(e1), "");
}

TEST(ECS_World, View) {
    two::World world;
    auto e0 = world.make_entity();
    auto e1 = world.make_entity();
    auto e2 = world.make_entity();
    world.pack(e0, A{}, B{});
    world.pack(e1, A{});
    world.pack(e2, A{}, B{}, C{});

    auto v0 = world.view<A, B, C>();
    EXPECT_EQ(1, v0.size());
    EXPECT_EQ(e2, v0[0]);
    EXPECT_TRUE((world.contains<A, B, C>(v0[0])));

    EXPECT_EQ(3, world.view<A>().size());
    EXPECT_TRUE(world.view_one<A>().has_value);
    EXPECT_EQ(e0, world.view_one<A>().value());

    world.remove<A>(e0);
    EXPECT_EQ(2, world.view<A>().size());

    world.destroy_entity(e1);
    EXPECT_TRUE(world.view_one<A>().has_value);
    EXPECT_EQ(e2, world.view_one<A>().value());

    world.set_active(e2, false);
    EXPECT_EQ(0, world.view<A>().size());
    EXPECT_EQ(1, world.view<A>(true).size());
    EXPECT_EQ(2, world.view(true).size());

    world.set_active(e2, true);
    EXPECT_EQ(1, world.view<A>().size());
}

TEST(ECS_World, ViewEach) {
    two::World world;
    auto e0 = world.make_entity();
    auto &a = world.pack(e0, A{12});
    world.pack(e0, B{24});

    world.each<A, B>([e0](two::Entity entity, A &a, B &b) {
        EXPECT_EQ(entity, e0);
        EXPECT_EQ(12, a.data);
        EXPECT_EQ(24, b.data);
    });
    world.each<A, B>([](A &a, B &b) {
        EXPECT_EQ(12, a.data);
        EXPECT_EQ(24, b.data);
        a.data = 16;
    });
    EXPECT_EQ(16, a.data);
}

TEST(ECS_World, MakeSystem) {
    two::World world;
    auto *s0 = world.make_system<SystemA>();
    ASSERT_NE(s0, nullptr);
    EXPECT_EQ(s0, world.get_system<SystemA>());

    auto *sb = world.make_system_before<SystemA, SystemB>();
    ASSERT_EQ(2, world.systems().size());
    EXPECT_EQ(sb, world.systems()[0]);

    auto *s1 = world.make_system<SystemA>();
    EXPECT_EQ(s0, world.get_system<SystemA>());
    std::vector<SystemA *> systemsA;
    world.get_all_systems(&systemsA);
    EXPECT_EQ(2, systemsA.size());

    world.destroy_system(s1);
    EXPECT_EQ(2, world.systems().size());
}

TEST(ECS_World, Events) {
    two::World world;
    int res = 0;
    world.bind<int>([&res](const int &) {
        res = -1;
        return false;
    });
    world.bind<int>([&res](const int &event) {
        res = event;
        return true;
    });
    world.bind<int>([&res](const int &) {
        res = -2;
        return true;
    });
    world.emit(12);
    EXPECT_EQ(12, res);

    world.clear_event_channels();
    // Make sure this is not an error
    world.emit(24);
    world.emit(A{});
    EXPECT_EQ(12, res);
}
