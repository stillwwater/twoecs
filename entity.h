// Copyright (c) 2020 stillwwater
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#ifndef TWO_ENTITY_H
#define TWO_ENTITY_H

#include <bitset>
#include <array>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <algorithm>
#include <functional>
#include <cstdint>

// By default entities are 32 bit (16 bit index, 16 bit version number).
// Define TWO_ENTITY_64 to use 64 bit entities.
#ifdef TWO_ENTITY_64
#undef TWO_ENTITY_32
#define TWO_ENTITY_INT_TYPE uint64_t
#define TWO_ENTITY_INDEX_MASK 0xffffffffUL
#define TWO_ENTITY_VERSION_SHIFT 32UL
#else
#define TWO_ENTITY_32
#define TWO_ENTITY_INT_TYPE uint32_t
#define TWO_ENTITY_INDEX_MASK 0xffff
#define TWO_ENTITY_VERSION_SHIFT 16
#endif

// Allows size of component types (identifiers) to be configured
#ifndef TWO_COMPONENT_INT_TYPE
#define TWO_COMPONENT_INT_TYPE uint8_t
#endif

// Defines the maximum number of entities that may be alive at the same time
#ifndef TWO_ENTITY_MAX
#define TWO_ENTITY_MAX 8192
#endif

// Defines the maximum number of component types
#ifndef TWO_COMPONENT_MAX
#define TWO_COMPONENT_MAX 64
#endif

// Allows a custom allocator to be used to store component data
#ifndef TWO_COMPONENT_ARRAY_ALLOCATOR
#define TWO_COMPONENT_ARRAY_ALLOCATOR std::allocator
#endif

// Enable assertions. ASSERT and ASSERTS can be defined before including
// to use your own assertions.
#ifdef TWO_ASSERTIONS
#    ifndef ASSERT
#        include <cassert>
#        define ASSERT(exp) assert(exp)
#    endif
#    ifndef ASSERTS
#        define ASSERTS(exp, msg) ASSERT(exp && msg)
#    endif
#else
#    define ASSERT(exp) (void)sizeof(exp)
#    define ASSERTS(exp, msg) (void)sizeof(exp)
#endif
#define ASSERT_ENTITY(entity) ASSERT(entity != NullEntity)

// Extra checks that may be too slow even for debug builds
#ifdef TWO_PARANOIA
#define ASSERT_PARANOIA(exp) ASSERT(exp)
#define ASSERTS_PARANOIA(exp, msg) ASSERTS(exp, msg)
#else
#define ASSERT_PARANOIA(exp) (void)sizeof(exp)
#define ASSERTS_PARANOIA(exp, msg) (void)sizeof(exp)
#endif

#ifndef LIKELY
#    if defined(__clang__) || defined(__GNUC__) || defined(__INTEL_COMPILER)
#        define LIKELY(x) __builtin_expect(!!(x), 1)
#        define UNLIKELY(x) __builtin_expect(!!(x), 0)
#    else
#        define LIKELY(x) (x)
#        define UNLIKELY(x) (x)
#    endif
#endif

// Allows C++ 17 like folding of template parameter packs in expressions
#define TWO_TEMPLATE_FOLD(exp)               \
    using Expand_ = char[];                  \
    (void)Expand_{((void)(exp), char(0))...}

// Print information on each entity cache operation
#ifdef TWO_DEBUG_ENTITY
#include <cstdio>
#define TWO_MSG(...) printf(__VA_ARGS__)
#else
#define TWO_MSG(...)
#endif

namespace two {
namespace internal {

template <typename T>
struct TypeIdInfo {
    static const T *const value;
};

template <typename T>
const T *const TypeIdInfo<T>::value = nullptr;

} // internal

// Compile time id for a given type.
using type_id_t = const void *;

// Compile time typeid.
template <typename T>
constexpr type_id_t type_id() {
  return &internal::TypeIdInfo<typename std::remove_const<
      typename std::remove_volatile<typename std::remove_pointer<
          typename std::remove_reference<T>::type>::type>::type>::type>::value;
}

// Type erased `unique_ptr`, this adds some memory overhead since it needs
// a custom deleter.
using unique_void_ptr_t = std::unique_ptr<void, void (*)(void *)>;

template <typename T>
unique_void_ptr_t unique_void_ptr(T *p) {
    return unique_void_ptr_t(p, [](void *data) {
        delete static_cast<T *>(data);
    });
}

// A unique identifier representing each entity in the world.
using Entity = TWO_ENTITY_INT_TYPE;
static_assert(std::is_integral<Entity>(), "Entity must be integral");

// A unique identifier representing each type of component.
using ComponentType = TWO_COMPONENT_INT_TYPE;
static_assert(std::is_integral<ComponentType>(),
              "ComponentType must be integral");

// Holds information on which components are attached to an entity.
// 1 bit is used for each component type.
// Note: Do not serialize an entity mask since which bit represents a
// given component may change. Use the contains<Component> function
// instead.
using EntityMask = std::bitset<TWO_COMPONENT_MAX>;

// Used to represent an entity that has no value. The `NullEntity` exists
// in the world but has no components.
constexpr Entity NullEntity = 0;

// Creates an Entity id from an index and version
inline Entity entity_id(TWO_ENTITY_INT_TYPE i, TWO_ENTITY_INT_TYPE version) {
    return i | (version << TWO_ENTITY_VERSION_SHIFT);
}

// Returns the index part of an entity id.
inline TWO_ENTITY_INT_TYPE entity_index(Entity entity) {
    return entity & TWO_ENTITY_INDEX_MASK;
}

// Returns the version part of an entity id.
inline TWO_ENTITY_INT_TYPE entity_version(Entity entity) {
    return entity >> TWO_ENTITY_VERSION_SHIFT;
}

// A simple optional type in order to support C++ 11.
template <typename T>
class Optional {
public:
    bool has_value;

    Optional(const T &value) : has_value{true}, val{value} {}
    Optional(T &&value) : has_value{true}, val{std::move(value)} {}
    Optional() : has_value{false} {}

    const T &value() const &;
    T &value() &;

    const T &&value() const &&;
    T &&value() &&;

    T value_or(T &&u) { has_value ? std::move(val) : u; };
    T value_or(const T &u) const { has_value ? val : u; };

private:
    T val;
};

// An empty component that is used to indicate whether the entity it is
// attached to is currently active.
struct Active {};

class World;

// Base class for all systems. The lifetime of systems is managed by a World.
class System {
public:
    virtual ~System() = default;
    virtual void load(World *world);
    virtual void update(World *world, float dt);
    virtual void draw(World *world);
    virtual void unload(World *world);
};

class IComponentArray {
public:
    virtual ~IComponentArray() = default;
    virtual void remove(Entity entity) = 0;
    virtual void copy(Entity dst, Entity src) = 0;
};

// Manages all instances of a component type and keeps track of which
// entity a component is attached to.
template <typename T>
class ComponentArray : public IComponentArray {
public:
    static_assert(std::is_copy_assignable<T>(),
                  "Component type must be copy assignable");

    static_assert(std::is_copy_constructible<T>(),
                  "Component type must be copy constructible");

    // Entity int type is used to ensure we can address the maximum
    // number of entities.
    using PackedSizeType = TWO_ENTITY_INT_TYPE;

    // Approximate amount of memory reserved when the array is initialized,
    // used to reduce the amount of initial allocations.
    static constexpr size_t MinSize = 1024;

    ComponentArray();

    // Returns a component of type T given an Entity.
    // Note: References returned by this function are only guaranteed to be
    // valid during the frame in which the component was read, after
    // that the component may become invalidated. Don't hold reference.
    inline T &read(Entity entity);

    // Set a component in the packed array and associate an entity with the
    // component.
    T &write(Entity entity, const T &component);

    // Invalidate this component type for an entity.
    // This function will always succeed, even if the entity does not
    // contain a component of this type.
    //
    // > References returned by `read` may become invalid after remove is
    // called.
    void remove(Entity entity) override;

    // Copy component to `dst` from `src`.
    void copy(Entity dst, Entity src) override;

    // Returns true if the entity has a component of type T.
    inline bool contains(Entity entity) const;

    // Returns the number of valid components in the packed array.
    size_t count() const { return packed_count; };

private:
    // All instances of component type T are stored in a contiguous vector.
    std::vector<T, TWO_COMPONENT_ARRAY_ALLOCATOR<T>> packed_array;

    // Maps an Entity id to an index in the packed array.
    std::unordered_map<Entity, PackedSizeType> entity_to_packed;

    // Maps an index in the packed component array to an Entity.
    std::unordered_map<PackedSizeType, Entity> packed_to_entity;

    // Number of valid entries in the packed array, other entries beyond
    // this count may be uninitialized or invalid data.
    size_t packed_count = 0;
};

// An event channel handles events for a single event type.
template <typename Event>
class EventChannel {
public:
    using EventHandler = std::function<bool (const Event &)>;

    // Adds a function as an event handler
    void bind(EventHandler &&fn);

    // Emits an event to all event handlers
    void emit(const Event &event) const;

private:
    std::vector<EventHandler> handlers;
};

// A world holds a collection of systems, components and entities.
class World {
public:
    template <typename T>
    using ViewFunc = typename std::common_type<std::function<T>>::type;

    World() = default;

    World(const World &) = delete;
    World &operator=(const World &) = delete;

    World(World &&) = default;
    World &operator=(World &&) = default;

    virtual ~World() = default;

    // Called before the first frame when the world is created.
    virtual void load();

    // Called once per per frame after all events have been handled and
    // before draw.
    //
    // > Systems are not updated on their own, you should call update
    // on systems here. Note that you should not call `draw()` on
    // each system since that is handled in the main loop.
    virtual void update(float dt);

    // Called before a world is unloaded.
    //
    // > Note: When overriding this function you need to call unload
    // on each system and free them.
    virtual void unload();

    // Creates a new entity in the world with an Active component.
    Entity make_entity();

    // Creates a new entity and copies components from another.
    Entity make_entity(Entity archetype);

    // Creates a new inactive entity in the world. The entity will need
    // to have active set before it can be used by systems.
    // Useful to create entities without initializing them.
    // > Note: Inactive entities still exist in the world and can have
    // components added to them.
    Entity make_inactive_entity();

    // Copy components from entity `src` to entity `dst`.
    void copy_entity(Entity dst, Entity src);

    // Destroys an entity and all of its components.
    void destroy_entity(Entity entity);

    // Returns the entity mask
    inline const EntityMask &get_mask(Entity entity) const;

    // Adds or replaces a component and associates an entity with the
    // component.
    //
    // Adding components will invalidate the cache. The number of cached views
    // is *usually* approximately equal to the number of systems, so this
    // operation is not that expensive but you should avoid doing it every
    // frame. This does not apply to 're-packing' components (see note below).
    //
    // > Note: If the entity already has a component of the same type, the old
    // component will be replaced. Replacing a component with a new instance
    // is *much* faster than calling `remove` and then `pack` which
    // would result in the cache being rebuilt twice. Replacing a component
    // does not invalidate the cache and is cheap operation.
    template <typename Component>
    Component &pack(Entity entity, const Component &component);

    // Shortcut to pack multiple components to an entity, equivalent to
    // calling `pack(entity, component)` for each component.
    //
    // Unlike the original `pack` function, this function does not return a
    // reference to the component that was just packed.
    template <typename C0, typename... Cn>
    void pack(Entity entity, const C0 &component, const Cn &...components);

    // Returns a component of the given type associated with an entity.
    // This function will only check if the component does not exist for an
    // entity if assertions are enabled, otherwise it is unchecked.
    // Use contains if you need to verify the existence of a component
    // before removing it. This is a cheap operation.
    //
    // This function returns a reference to a component in the packed array.
    // The reference may become invalid after `remove` is called since `remove`
    // may move components in memory.
    //
    //     auto &a = world.unpack<A>(entity1);
    //     a.x = 5; // a is a valid reference and x will be updated.
    //
    //     auto b = world.unpack<B>(entity1); // copy B
    //     world.remove<B>(entity2);
    //     b.x = 5;
    //     world.pack(entity1, b); // Ensures b will be updated in the array
    //
    //     auto &c = world.unpack<C>(entity1);
    //     world.remove<C>(entity2);
    //     c.x = 5; // may not update c.x in the array
    //
    // If you plan on holding the reference it is better to copy the
    // component and then pack it again if you have modified the component.
    // Re-packing a component is a cheap operation and will not invalidate.
    // the cache.
    //
    // > Do not store this reference between frames such as in a member
    // variable, store the entity instead and call unpack each frame. This
    // operation is designed to be called multiple times per frame so it
    // is very fast, there is no need to `cache` a component reference in
    // a member variable.
    template <typename Component>
    inline Component &unpack(Entity entity);

    // Returns true if a component of the given type is associated with an
    // entity. This is a cheap operation.
    template <typename Component>
    inline bool contains(Entity entity) const;

    // Returns true if an entity contains all given components.
    template <typename C0, typename C1, typename... Cn>
    inline bool contains(Entity entity) const;

    // Removes a component from an entity. Removing components invalidates
    // the cache.
    //
    // This function will only check if the component does not exist for an
    // entity if assertions are enabled, otherwise it is unchecked. Use
    // `contains` if you need to verify the existence of a component before
    // removing it.
    template <typename Component>
    void remove(Entity entity);

    // Adds or removes an Active component
    inline void set_active(Entity entity, bool active);

    // Returns all entities that have all requested components. By default
    // only entities with an `Active` component are matched unless
    // `include_inactive` is true.
    //
    //     for (auto entity : view<A, B, C>()) {
    //         auto &a = unpack<A>(entity);
    //         // ...
    //     }
    //
    // The first call to this function will build a cache with the entities
    // that contain the requested components, subsequent calls will return
    // the cached data as long as the data is still valid. If a cache is no
    // longer valid, this function will rebuild the cache by applying all
    // necessary diffs to make the cache valid.
    //
    // The cost of rebuilding the cache depends on how many diff operations
    // are needed. Any operation that changes whether an entity should be
    // in this cache (does the entity have all requested components?) counts
    // as a single Add or Remove diff. Functions like `remove`,
    // `make_entity`, `pack`, `destroy_entity` may cause a cache to be
    // invalidated. Functions that may invalidate the cache are documented.
    template <typename... Components>
    const std::vector<Entity> &view(bool include_inactive = false);

    // Calls `fn` with a reference to each unpacked component for every entity
    // with all requested components.
    //
    //     each<A, B, C>([](A &a, B &b, C &c) {
    //         // ...
    //     });
    //
    // This function calls `view<Components...>()` internally so the same
    // notes about `view` apply here as well.
    template <typename... Components>
    inline void each(ViewFunc<void (Components &...)> &&fn,
                     bool include_inactive = false);

    // Calls `fn` with the entity and a reference to each unpacked component
    // for every entity with all requested components.
    //
    //     each<A, B, C>([](Entity entity, A &a, B &b, C &c) {
    //         // ...
    //     });
    //
    // This function calls `view<Components...>()` internally so the same
    // notes about `view` apply here as well.
    template <typename... Components>
    inline void each(ViewFunc<void (Entity, Components &...)> &&fn,
                     bool include_inactive = false);

    // Returns the **first** entity that contains all components requested.
    // Views always keep entities in the order that the entity was
    // added to the view, so `view_one()` will reliabily return the same
    // entity that matches the constraints unless the entity was destroyed
    // or has had one of the required components removed.
    //
    // The returned optional will have no value if no entities exist with
    // all requested components.
    template <typename... Components>
    Optional<Entity> view_one(bool include_inactive = false);

    // Finds the first entity with the requested component and unpacks
    // the component requested. This is convenience function for getting at
    // a single component in a single entity.
    //
    //     auto camera = world.unpack_one<Camera>();
    //
    //     // is equivalent to
    //     auto entity = world.view_one<Camera>().value();
    //     auto camera = world.unpack<Camera>(entity);
    //
    // Unlike `view_one()` this function will panic if no entities with
    // the requested component were matched. Only use this function if not
    // matching any entity should be an error, if not use `view_one()`
    // instead.
    template <typename Component>
    Component &unpack_one(bool include_inactive = false);

    // Returns all entities in the world. Entities returned may be inactive.
    // > Note: Calling `destroy_entity()` will invalidate the iterator, use
    // `view<>(true)` to get all entities without having `destroy_entity()`
    // invalidate the iterator.
    inline const std::vector<Entity> &unsafe_view_all() { return entities; }

    // Creates and adds a system to the world. This function calls
    // `System::load` to initialize the system.
    //
    // > Systems are not unique. 'Duplicate' systems, that is different
    // instances of the same system type, are allowed in the world.
    template <class T, typename... Args>
    T *make_system(Args &&...args);

    // Adds a system to the world. This function calls `System::load` to
    // initialize the system.
    //
    // > You may add multiple different instances of the same system type,
    // but passing a system pointer that points to an instance that is
    // already in the world is not allowed and will not be checked by default.
    // Enable `TWO_PARANOIA` to check for duplicate pointers.
    template <class T>
    T *make_system(T *system);

    // Adds a system to the world before another system if it exists.
    // `Before` is an existing system.
    // `T` is the system to be added before an existing system.
    template <class Before, class T, typename... Args>
    T *make_system_before(Args &&...args);

    // Returns the first system that matches the given type.
    // System returned will be `nullptr` if it is not found.
    template <class T>
    T *get_system();

    // Returns all system that matches the given type.
    // Systems returned will not be null.
    template <class T>
    void get_all_systems(std::vector<T *> *systems);

    // System must not be null. Do not destroy a system while the main update
    // loop is running as it could invalidate the system iterator, consider
    // checking if the system should run or not in the system update loop
    // instead.
    void destroy_system(System *system);

    // Destroy all systems in the world.
    void destroy_systems();

    // Systems returned will not be null.
    inline const std::vector<System *> &systems() { return active_systems; }

    // Adds a function to receive events of type T
    template <typename Event>
    void bind(typename EventChannel<Event>::EventHandler &&fn);

    // Same as `bind(fn)` but allows a member function to be used
    // as an event handler.
    //
    //     bind<KeyDownEvent>(&World::keydown, this);
    template <typename Event, typename Func, class T>
    void bind(Func &&fn, T this_ptr);

    // Emits an event to all event handlers. If a handler function in the
    // chain returns true then the event is considered handled and will
    // not propagate to other listeners.
    template <typename Event>
    void emit(const Event &event) const;

    // Removes all event handlers, you'll unlikely need to call this since
    // events are cleared when the world is destroyed.
    inline void clear_event_channels() { channels.clear(); };

    // Components will be registered on their own if a new type of component is
    // added to an entity. There is no need to call this function unless you
    // are doing something specific that requires it.
    template <typename Component>
    void register_component();

    // Registers a component type if it does not exist and returns it.
    // Components are registered automatically so there is usually no reason
    // to call this function.
    template <typename Component>
    inline ComponentType find_or_register_component();

    // Recycles entity ids so that they can be safely reused. This function
    // exists to ensure we don't reuse entity ids that are still present in
    // some cache even though the entity has been destroyed. This can happen
    // since cache operations are done in a 'lazy' manner.
    // This function should be called at the end of each frame.
    void collect_unused_entities();

private:
    // Used to speed up entity lookups
    struct EntityCache {
        struct Diff {
            enum Operation { Add, Remove };
            Entity entity;
            Operation op;
        };
        std::vector<Entity> entities;
        std::vector<Diff> diffs;
        std::unordered_set<Entity> lookup;
    };

    struct DestroyedEntity {
        Entity entity;
        // Caches that needs to be rebuilt before the entity can be reused
        std::vector<EntityCache *> caches;
    };

    size_t component_type_index = 0;
    size_t alive_count = 0;

    // Systems cannot outlive World.
    std::vector<System *> active_systems;

    // Kept separate since most of the time we just want to iterate through
    // all the systems and do not need to know their types.
    std::vector<type_id_t> active_system_types;

    // Contains available entity ids. When creating entities check if this
    // is not empty, otherwise use alive_count + 1 as the new id.
    std::vector<Entity> unused_entities;

    // Contains available entity ids that may still be present in
    // some cache. Calling `collect_unused_entities()` will remove the
    // entity from the caches so that the entity can be reused.
    std::vector<DestroyedEntity> destroyed_entities;

    // All alive (but not necessarily active) entities.
    std::vector<Entity> entities;

    std::unordered_map<EntityMask, EntityCache> view_cache;

    // Index with component index from component_types[type]
    std::array<std::unique_ptr<IComponentArray>, TWO_COMPONENT_MAX> components;

    // Masks for all entities.
    std::array<EntityMask, TWO_ENTITY_MAX> entity_masks{};

    std::unordered_map<type_id_t, ComponentType> component_types;

    // Event channels.
    std::unordered_map<type_id_t, unique_void_ptr_t> channels;

    void apply_diffs_to_cache(EntityCache *cache);
    void invalidate_cache(EntityCache *cache, EntityCache::Diff &&diff);
};

inline const EntityMask &World::get_mask(Entity entity) const {
    return entity_masks[entity_index(entity)];
}

template <typename Component>
Component &World::pack(Entity entity, const Component &component) {
    ASSERT_ENTITY(entity);
    auto index = entity_index(entity);
    auto current_mask = entity_masks[index];
    auto &mask = entity_masks[index];

    // Component may not have been regisered yet
    auto type = find_or_register_component<Component>();
    mask.set(type);

    auto *a = static_cast<ComponentArray<Component> *>(components[type].get());
    auto &new_component = a->write(entity, component);

    if (current_mask[type] == mask[type]) {
        // entity already has a component of this type, the component was
        // replaced, but since the mask is unchanged there is no need to
        // rebuild the cache.
        TWO_MSG("%s is unchanged since entity #%x is unchanged\n",
                mask.to_string().c_str(), entity);
        return new_component;
    }

    // Invalidate caches
    for (auto &cached : view_cache) {
        if ((mask & cached.first) != cached.first) {
            continue;
        }
        auto &lookup = cached.second.lookup;
        if (lookup.find(entity) != lookup.end()) {
            // Entity is already in the cache
            continue;
        }

        invalidate_cache(&cached.second,
            EntityCache::Diff{entity, EntityCache::Diff::Add});

        TWO_MSG("%s now includes entity #%x\n",
                mask.to_string().c_str(), entity);
    }
    return new_component;
}

template <typename C0, typename... Cn>
void World::pack(Entity entity, const C0 &component, const Cn &...components) {
    pack(entity, component);
    pack(entity, components...);
}

template <typename Component>
inline Component &World::unpack(Entity entity) {
    ASSERT_ENTITY(entity);
    // Assume component was registered when it was packed
    ASSERT(component_types.find(type_id<Component>())
           != component_types.end());

    auto type = component_types[type_id<Component>()];
    auto *a = static_cast<ComponentArray<Component> *>(components[type].get());
    return a->read(entity);
}

template <typename Component>
inline bool World::contains(Entity entity) const {
    // This function must work if a component has never been registered,
    // since it's reasonable to check if an entity has a component when
    // a component type has never been added to any entity.
    auto type_it = component_types.find(type_id<Component>());
    if (type_it == component_types.end()) {
        return false;
    }
    auto type = type_it->second;
    auto *a = static_cast<ComponentArray<Component> *>(components[type].get());
    return a->contains(entity);
}

template <typename C0, typename C1, typename... Cn>
inline bool World::contains(Entity entity) const {
    return contains<C0>(entity)
        && contains<C1>(entity)
        && contains<Cn...>(entity);
}

template <typename Component>
void World::remove(Entity entity) {
    // Assume component was registered when it was packed
    ASSERT(component_types.find(type_id<Component>())
           != component_types.end());

    auto type = component_types[type_id<Component>()];
    auto *a = static_cast<ComponentArray<Component> *>(components[type].get());
    a->remove(entity);

    // Invalidate caches
    for (auto &cached : view_cache) {
        if (!cached.first[type]) {
            continue;
        }
        auto &lookup = cached.second.lookup;
        if (lookup.find(entity) == lookup.end()) {
            // Entity has already been removed from cache
            continue;
        }
        invalidate_cache(&cached.second,
            EntityCache::Diff{entity, EntityCache::Diff::Remove});

        TWO_MSG("%s no longer includes entity #%x\n",
                cached.first.to_string().c_str(), entity);
    }
    entity_masks[entity_index(entity)].reset(type);
}

inline void World::set_active(Entity entity, bool active) {
    ASSERT_ENTITY(entity);
    // If active is constant this conditional should be removed when
    // the function is inlined
    if (active)
        pack(entity, Active{});
    else
        remove<Active>(entity);
}

template <typename... Components>
const std::vector<Entity> &World::view(bool include_inactive) {
    static const ComponentType active_component_t =
        find_or_register_component<Active>();

    EntityMask mask;
    // Component may not have been registered
    TWO_TEMPLATE_FOLD(mask.set(find_or_register_component<Components>()));

    if (!include_inactive) {
        mask.set(active_component_t);
    }

    auto cache_it = view_cache.find(mask);
    if (LIKELY(cache_it != view_cache.end())) {
        auto &cache = cache_it->second;

        TWO_MSG("%s view (%lu) [ops: %lu]\n",
                mask.to_string().c_str(),
                cache.entities.size(),
                cache.diffs.size());

        if (LIKELY(cache.diffs.empty())) {
            return cache.entities;
        }
        apply_diffs_to_cache(&cache);
        return cache.entities;
    }
    TWO_MSG("%s view (initial cache build)\n", mask.to_string().c_str());

    std::vector<Entity> cache;
    std::unordered_set<Entity> lookup;

    for (auto entity : entities) {
        if ((mask & entity_masks[entity_index(entity)]) == mask) {
            if (LIKELY(entity != NullEntity)) {
                cache.push_back(entity);
                lookup.insert(entity);
            }
        }
    }
    view_cache.emplace(std::make_pair(mask,
        EntityCache{std::move(cache), {}, std::move(lookup)}));

    return view_cache[mask].entities;
}

template <typename... Components>
inline void World::each(ViewFunc<void (Components &...)> &&fn,
                        bool include_inactive) {
    for (const auto entity : view<Components...>(include_inactive)) {
        fn(unpack<Components>(entity)...);
    }
}

template <typename... Components>
inline void World::each(ViewFunc<void (Entity, Components &...)> &&fn,
                        bool include_inactive) {
    for (const auto entity : view<Components...>(include_inactive)) {
        fn(entity, unpack<Components>(entity)...);
    }
}

template <typename... Components>
Optional<Entity> World::view_one(bool include_inactive) {
    auto &v = view<Components...>(include_inactive);
    if (v.size() > 0) {
        return v[0];
    }
    return {};
}

template <typename Component>
Component &World::unpack_one(bool include_inactive) {
    auto &v = view<Component>(include_inactive);
    ASSERTS(v.size() > 0, "No entities were matched");
    return unpack<Component>(v[0]);
}

template <class T, typename... Args>
T *World::make_system(Args &&...args) {
    return make_system(new T(std::forward<Args>(args)...));
}

template <class T>
T *World::make_system(T *system) {
    static_assert(std::is_convertible<T *, System *>(),
                  "Type cannot be converted to a System");
    ASSERTS_PARANOIA(
        std::find(active_systems.begin(),
                  active_systems.end(), system) == active_systems.end(),
        "Duplicate systems in the world");

    active_systems.push_back(system);
    active_system_types.push_back(type_id<T>());
    system->load(this);
    return system;
}

template <class Before, class T, typename... Args>
T *World::make_system_before(Args &&...args) {
    static_assert(std::is_convertible<T *, System *>(),
                  "Type cannot be converted to a System");
    ASSERT(active_systems.size() == active_system_types.size());

    auto pos = std::find(active_system_types.begin(),
                         active_system_types.end(),
                         type_id<Before>());

    auto *system = new T(std::forward<Args>(args)...);
    if (pos == active_system_types.end()) {
        return system;
    }
    size_t index = std::distance(pos, active_system_types.end()) - 1;
    active_systems.insert(active_systems.begin() + index, system);
    active_system_types.insert(pos, type_id<T>());
    return system;
}

template <class T>
T *World::get_system() {
    static_assert(std::is_convertible<T *, System *>(),
                  "Type cannot be converted to a System");
    ASSERT(active_systems.size() == active_system_types.size());

    auto pos = std::find(active_system_types.begin(),
                         active_system_types.end(),
                         type_id<T>());

    if (pos == active_system_types.end()) {
        return nullptr;
    }
    auto index = std::distance(active_system_types.begin(), pos);
    return static_cast<T *>(active_systems[index]);
}

template <class T>
void World::get_all_systems(std::vector<T *> *systems) {
    static_assert(std::is_convertible<T *, System *>(),
                  "Type cannot be converted to a System");
    ASSERT(active_systems.size() == active_system_types.size());
    ASSERT(systems != nullptr);

    for (size_t i = 0; i < active_systems.size(); ++i) {
        if (active_system_types[i] != type_id<T>()) {
            continue;
        }
        systems->push_back(static_cast<T *>(active_systems[i]));
    }
}

template <typename Component>
void World::register_component() {
    // Component must not already exist
    ASSERT(component_types.find(
        type_id<Component>()) == component_types.end());

    int i = component_type_index++;
    component_types.emplace(std::make_pair(type_id<Component>(), i));
    components[i] = std::unique_ptr<ComponentArray<Component>>(
        new ComponentArray<Component>);
}

template <typename Component>
inline ComponentType World::find_or_register_component() {
    auto match = component_types.find(type_id<Component>());
    if (LIKELY(match != component_types.end())) {
        return match->second;
    }
    register_component<Component>();
    return component_types[type_id<Component>()];
}

inline Entity World::make_entity() {
    auto entity = make_inactive_entity();
    pack(entity, Active{});
    return entity;
}

inline Entity World::make_entity(Entity archetype) {
    ASSERT_ENTITY(archetype);
    auto entity = make_entity();
    copy_entity(entity, archetype);
    return entity;
}

inline Entity World::make_inactive_entity() {
    Entity entity;
    if (unused_entities.empty()) {
        ASSERTS(alive_count < TWO_ENTITY_MAX, "Too many entities");
        entity = alive_count++;

        // Create a nullentity that will never have any components
        // This is useful when we need to store entities in an array and
        // need a way to define entities that are not valid.
        if (entity == NullEntity) {
            entities.push_back(NullEntity);
            ++entity;
            ++alive_count;
        }
    } else {
        entity = unused_entities.back();
        unused_entities.pop_back();
        auto version = entity_version(entity);
        auto index = entity_index(entity);
        entity = entity_id(index, version + 1);
    }
    entities.push_back(entity);
    return entity;
}

inline void World::copy_entity(Entity dst, Entity src) {
    ASSERT_ENTITY(dst);
    auto &dst_mask = entity_masks[entity_index(dst)];
    auto &src_mask = entity_masks[entity_index(src)];
    for (const auto &type_it : component_types) {
        if (!src_mask.test(type_it.second)) {
            continue;
        }
        auto &a = components[type_it.second];
        a->copy(dst, src);
        dst_mask.set(type_it.second);
    }

    for (auto &cached : view_cache) {
        if ((dst_mask & cached.first) != cached.first) {
            continue;
        }
        auto &lookup = cached.second.lookup;
        if (lookup.find(dst) != lookup.end()) {
            continue;
        }

        invalidate_cache(&cached.second,
            EntityCache::Diff{dst, EntityCache::Diff::Add});

        TWO_MSG("%s now includes entity #%x (copied from #%x)\n",
                dst_mask.to_string().c_str(), dst, src);
    }
}

inline void World::destroy_entity(Entity entity) {
    ASSERT_ENTITY(entity);
    for (auto &a : components) {
        if (a != nullptr) {
            a->remove(entity);
        }
    }
    entity_masks[entity_index(entity)] = 0;

    DestroyedEntity destroyed;
    destroyed.entity = entity;

    for (auto &cached : view_cache) {
        auto &lookup = cached.second.lookup;
        if (lookup.find(entity) == lookup.end()) {
            continue;
        }
        cached.second.diffs.emplace_back(
            EntityCache::Diff{entity, EntityCache::Diff::Remove});

        // This cache must be rebuilt before the entity can be reused.
        destroyed.caches.push_back(&cached.second);

        TWO_MSG("%s no longer includes entity #%x (destroyed)\n",
                cached.first.to_string().c_str(), entity);
    }
    auto rem = std::find(entities.begin(), entities.end(), entity);
    ASSERT(rem != entities.end());
    std::swap(*rem, entities.back());
    entities.pop_back();
    destroyed_entities.emplace_back(std::move(destroyed));
}

inline void World::collect_unused_entities() {
    if (destroyed_entities.size() == 0) {
        return;
    }
    for (const auto &destroyed : destroyed_entities) {
        for (auto *cache : destroyed.caches) {
            // In most cases the cache will have no diffs since if this cache
            // is viewed every frame by some system it would have been rebuilt
            // by this point anyway.
            apply_diffs_to_cache(cache);
        }
        // Make entity id available again
        unused_entities.push_back(destroyed.entity);
    }
    destroyed_entities.clear();
}

inline void World::destroy_system(System *system) {
    ASSERT(active_systems.size() == active_system_types.size());
    ASSERT(system != nullptr);

    auto pos = std::find(active_systems.begin(), active_systems.end(), system);
    if (pos == active_systems.end()) {
        TWO_MSG("warn: Trying to destroy a system that does not exist\n");
        return;
    }
    system->unload(this);
    delete system;

    auto index = std::distance(active_systems.begin(), pos);
    active_systems.erase(pos);
    active_system_types.erase(active_system_types.begin() + index);
}

inline void World::destroy_systems() {
    for (auto *system : active_systems) {
        system->unload(this);
        delete system;
    }
    active_systems.clear();
    active_system_types.clear();
}

inline void World::apply_diffs_to_cache(EntityCache *cache) {
    ASSERT(cache != nullptr);
    for (const auto &diff : cache->diffs) {
        switch (diff.op) {
        case EntityCache::Diff::Add:
            cache->entities.push_back(diff.entity);
            cache->lookup.insert(diff.entity);
            break;
        case EntityCache::Diff::Remove:
            {
                auto &vec = cache->entities;
                auto rem = std::find(vec.begin(), vec.end(), diff.entity);
                ASSERT(rem != vec.end());

                std::swap(*rem, vec.back());
                vec.pop_back();
                cache->lookup.erase(diff.entity);
                break;
            }
        default:
            ASSERTS(false, "Invalid cache operation");
            break;
        }
    }
    cache->diffs.clear();
}

inline void World::invalidate_cache(EntityCache *c, EntityCache::Diff &&diff) {
    for (const auto &d : c->diffs) {
        if (d.entity == diff.entity && d.op == diff.op) {
            return;
        }
    }
    c->diffs.emplace_back(std::move(diff));
}

template <typename Event>
void World::bind(typename EventChannel<Event>::EventHandler &&fn) {
    constexpr auto type = type_id<Event>();
    if (channels.find(type) == channels.end()) {
        auto c = unique_void_ptr<EventChannel<Event>>(new EventChannel<Event>);
        channels.emplace(std::make_pair(type, std::move(c)));
    }
    static_cast<EventChannel<Event> *>(channels.at(type).get())
        ->bind(std::move(fn));
}

template <typename Event, typename Func, class T>
void World::bind(Func &&fn, T this_ptr) {
    bind<Event>(std::bind(fn, this_ptr, std::placeholders::_1));
}

template <typename Event>
void World::emit(const Event &event) const {
    auto chan_it = channels.find(type_id<Event>());
    if (chan_it == channels.end()) {
        return;
    }
    static_cast<EventChannel<Event> *>(chan_it->second.get())->emit(event);
}

inline void World::load() {}
inline void World::update(float) {}
inline void World::unload() {}

template <typename T>
inline ComponentArray<T>::ComponentArray() {
    packed_array.reserve(MinSize / sizeof(T));
}

template <typename T>
inline T &ComponentArray<T>::read(Entity entity) {
    ASSERTS(entity_to_packed.find(entity) != entity_to_packed.end(),
            "Missing component on Entity.");
    return packed_array[entity_to_packed[entity]];
}

template <typename T>
T &ComponentArray<T>::write(Entity entity, const T &component) {
    if (entity_to_packed.find(entity) != entity_to_packed.end()) {
        // Replace component
        auto pos = entity_to_packed[entity];
        packed_array[pos] = component;
        return packed_array[pos];
    }
    ASSERT(packed_count < TWO_ENTITY_MAX);

    auto pos = packed_count++;
    entity_to_packed[entity] = pos;
    packed_to_entity[pos] = entity;

    if (pos < packed_array.size())
        packed_array[pos] = component;
    else
        packed_array.push_back(component);

    return packed_array[pos];
}

template <typename T>
void ComponentArray<T>::remove(Entity entity) {
    if (entity_to_packed.find(entity) == entity_to_packed.end()) {
        // This is a no-op since calling this as a virtual member function
        // means there is no way for the caller to check if the entity
        // contains a component. `contains` is not virtual as it needs to
        // be fast.
        return;
    }
    // Move the last component into the empty slot to keep the array packed
    auto last = packed_count - 1;
    auto removed = entity_to_packed[entity];
    packed_array[removed] = packed_array[last];

    // Need to know which entity "owns" the component we just moved
    auto moved_entity = packed_to_entity[last];
    packed_to_entity[removed] = moved_entity;

    // Update the entity that has its component moved to reference
    // the new location in the packed array
    entity_to_packed[moved_entity] = removed;

    entity_to_packed.erase(entity);
    packed_to_entity.erase(last);
    --packed_count;
}

template <typename T>
void ComponentArray<T>::copy(Entity dst, Entity src) {
    write(dst, read(src));
}

template <typename T>
inline bool ComponentArray<T>::contains(Entity entity) const {
    return entity_to_packed.find(entity) != entity_to_packed.end();
}

inline void System::load(World *) {}
inline void System::update(World *, float) {}
inline void System::draw(World *) {}
inline void System::unload(World *) {}

template <typename Event>
void EventChannel<Event>::bind(EventHandler &&fn) {
    handlers.push_back(std::move(fn));
}

template <typename Event>
void EventChannel<Event>::emit(const Event &event) const {
    for (const auto &fn : handlers) {
        if (fn(event)) break;
    }
}

template <typename T>
const T &Optional<T>::value() const & {
    ASSERT(has_value);
    return val;
}

template <typename T>
T &Optional<T>::value() & {
    ASSERT(has_value);
    return val;
}

template <typename T>
const T &&Optional<T>::value() const && {
    ASSERT(has_value);
    return std::move(val);
}

template <typename T>
T &&Optional<T>::value() && {
    ASSERT(has_value);
    return std::move(val);
}

} // two

#endif // TWO_ENTITY_H
