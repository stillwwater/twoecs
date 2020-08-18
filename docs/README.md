# `entity.h`

### Defines

These are guarded with `#ifndef DEFINE` so you can define yourself these before including `entity.h`.

``` cpp
// By default entities are 32 bit (16 bit index, 16 bit version number).
// Define TWO_ENTITY_64 to use 64 bit entities.
#define TWO_ENTITY_32

// Allows size of component types (identifiers) to be configured
#define TWO_COMPONENT_INT_TYPE uint8_t

// Defines the maximum number of entities that may be alive at the same time
#define TWO_ENTITY_MAX 8192

// Defines the maximum number of component types
#define TWO_COMPONENT_MAX 64

// Allows a custom allocator to be used to store component data
#define TWO_COMPONENT_ARRAY_ALLOCATOR std::allocator

// Prints information on each entity cache operation
// Compile with -DTWO_DEBUG_ENTITY
#define TWO_MSG(...)

// Enable assertions. ASSERT and ASSERTS can be defined before including
// to use your own assertions. Compile with -DTWO_ASSERTIONS.
#define ASSERT(exp)
#define ASSERTS(exp, msg)

// Extra checks that may be too slow even for debug builds.
// Compile with -DTWO_PARANOIA.
#define ASSERT_PARANOIA(exp)
#define ASSERTS_PARANOIA(exp, msg)
```

### Type alias `two::type_id_t`

``` cpp
using type_id_t = const void *;
```

Compile time id for a given type.

-----

### Function `two::type_id`

``` cpp
template <typename T>
constexpr type_id_t type_id();
```

Compile time typeid.

-----

### Type alias `two::unique_void_ptr_t`

``` cpp
using unique_void_ptr_t = std::unique_ptr<void, void (*)(void *)>;
```

Type erased `unique_ptr`, this adds some memory overhead since it needs a custom deleter.

---

### Function `two::unique_void_ptr`

```cpp
template <typename T>
unique_void_ptr_t unique_void_ptr(T *p);
```

-----

### Type alias `two::Entity`

``` cpp
using Entity = TWO_ENTITY_INT_TYPE;
```

A unique identifier representing each entity in the world.

-----

### Type alias `two::ComponentType`

``` cpp
using ComponentType = TWO_COMPONENT_INT_TYPE;
```

A unique identifier representing each type of component.

-----

### Type alias `two::EntityMask`

``` cpp
using EntityMask = std::bitset<TWO_COMPONENT_MAX>;
```

Holds information on which components are attached to an entity.

1 bit is used for each component type. Note: Do not serialize an entity mask since which bit represents a given component may change. Use the `contains<Component>` function instead.

-----

### Variable `two::NullEntity`

``` cpp
constexpr Entity NullEntity = 0;
```

Used to represent an entity that has no value. The `NullEntity` exists in the world but has no components.

-----

### Function `two::entity_id`

```cpp
inline two::Entity entity_id(TWO_ENTITY_INT_TYPE i, TWO_ENTITY_INT_TYPE version);
```

Creates an Entity id from an index and version.

-----

### Function `two::entity_index`

```cpp
inline TWO_ENTITY_INT_TYPE entity_index(two::Entity entity);
```

Returns the index part of an entity id.

---

### Function `two::entity_version`

```cpp
inline TWO_ENTITY_INT_TYPE entity_version(two::Entity entity)
```

Returns the version part of an entity id.

-----

### Class `two::Optional`

``` cpp
template <typename T>
class Optional {
public:
    bool has_value;

    Optional(const T &value);
    Optional(T&& value);
    Optional();

    const T &value() const &;
    T& value() &;

    const T &&value() const &&;
    T&& value() &&;

    T value_or(T&& u);
    T value_or(const T &u) const;
};
```

A simple optional type in order to support C++ 11.

-----

### Struct `two::Active`

``` cpp
struct Active {};
```

An empty component that is used to indicate whether the entity it is attached to is currently active.

-----

### Class `two::System`

``` cpp
class System {
public:
    virtual ~System() = default;
    virtual void load(two::World* world);
    virtual void update(two::World* world, float dt);
    virtual void draw(two::World* world);
    virtual void unload(two::World* world);
};
```

Base class for all systems. The lifetime of systems is managed by a World.

-----

### Class `two::ComponentArray`

``` cpp
template <typename T>
class ComponentArray : public IComponentArray {
public:
    static_assert(std::is_copy_assignable<T>(), "Component type must be copy assignable");

    static_assert(std::is_copy_constructible<T>(), "Component type must be copy constructible");

    using PackedSizeType = uint16_t;

    static constexpr uint32_t MinSize = 1024;

    ComponentArray();

    T &read(two::Entity entity);

    T &write(two::Entity entity, const T &component);

    virtual void remove(two::Entity entity) override;

    virtual void copy(two::Entity dst, two::Entity src) override;

    bool contains(two::Entity entity) const;

    size_t count() const;
};
```

Manages all instances of a component type and keeps track of which entity a component is attached to.

### Type alias `two::ComponentArray::PackedSizeType`

``` cpp
using PackedSizeType = TWO_ENTITY_INT_TYPE;
```

Entity int type is used to ensure we can address the maximum number of entities.

-----

### Variable `two::ComponentArray::MinSize`

``` cpp
static constexpr size_t MinSize;
```

Approximate amount of memory reserved when the array is initialized, used to reduce the amount of initial allocations.

-----

### Function `two::ComponentArray::read`

``` cpp
T &read(two::Entity entity);
```

Returns a component of type T given an Entity.

Note: References returned by this function are only guaranteed to be valid during the frame in which the component was read, after that the component may become invalidated. Don’t hold reference.

-----

### Function `two::ComponentArray::write`

``` cpp
T &write(two::Entity entity, const T &component);
```

Set a component in the packed array and associate an entity with the component.

-----

### Function `two::ComponentArray::remove`

``` cpp
virtual void remove(two::Entity entity) override;
```

Invalidate this component type for an entity.

This function will always succeed, even if the entity does not contain a component of this type.

> References returned by `read` may become invalid after remove is called.

---

### Function `two::ComponentArray::copy`

```cpp
virtual void copy(two::Entity dst, two::Entity src) override;
```

Copy component to `dst` from `src`.

-----

### Function `two::ComponentArray::count`

``` cpp
size_t count() const;
```

Returns the number of valid components in the packed array.

-----

-----

### Class `two::World`

``` cpp
class World {
public:
    template <typename T>
    using ViewFunc = typename std::common_type<std::function<T>>::type;

    World() = default;

    World(const two::World &) = delete;

    two::World &operator=(const two::World &) = delete;

    World(two::World &&) = default;

    two::World &operator=(two::World &&) = default;

    virtual ~World() = default;

    virtual void load();

    virtual void update(float dt);

    virtual void unload();

    two::Entity make_entity();

    two::Entity make_entity(two::Entity archetype);

    two::Entity make_inactive_entity();

    void copy_entity(two::Entity dst, two::Entity src);

    void destroy_entity(two::Entity entity);

    const two::EntityMask &get_mask(two::Entity entity) const;

    template <typename Component>
    Component &pack(two::Entity entity, const Component &component);

    template <typename Component, typename... Components>
    void pack(two::Entity entity, const Component &h, const Components &...t);

    template <typename Component>
    Component &unpack(two::Entity entity);

    template <typename Component>
    bool contains(two::Entity entity);

    template <typename Component>
    void remove(two::Entity entity);

    template <typename Component>
    void register_component();

    void set_active(two::Entity entity, bool active);

    template <typename... Components>
    const std::vector<Entity> &view(bool include_inactive = false);

    template <typename... Components>
    inline void each(ViewFunc<void (Components &...)> &&fn, bool include_inactive = false);

    template <typename... Components>
    inline void each(ViewFunc<void (Entity, Components &...)> &&fn, bool include_inactive = false);
    
    template <typename... Components>
    Optional<two::Entity> view_one(bool include_inactive = false);

    template <typename Component>
    Component &unpack_one(bool include_inactive = false);

    const std::vector<Entity> &unsafe_view_all();

    template <class T, typename... Args>
    T *make_system(Args &&...args);

    template <class T>
    T *make_system(T *system);

    template <class Before, class T, typename... Args>
    T *make_system_before(Args &&...args);

    template <class T>
    T *get_system();

    template <class T>
    void get_all_systems(std::vector<T *> *systems);

    void destroy_system(two::System *system);

    void destroy_systems();

    const std::vector<System *> &systems();

    template <typename Event>
    void bind(typename EventChannel<Event>::EventHandler &&fn);

    template <typename Event, typename Func, class T>
    void bind(Func &&fn, T this_ptr);

    void emit(const Event &event) const;

    inline void clear_event_channels();

    template <typename Component>
    two::ComponentType find_or_register_component();

    void collect_unused_entities();
};
```

A world holds a collection of systems, components and entities.

### Function `two::World::load`

``` cpp
virtual void load();
```

Called before the first frame when the world is created.

-----

### Function `two::World::update`

``` cpp
virtual void update(float dt);
```

Called once per per frame after all events have been handled and before draw.

> Systems are not updated on their own, you should call update on systems here. Note that you should not call `draw()` on each system since that is handled in the main loop.

-----

### Function `two::World::unload`

``` cpp
virtual void unload();
```

Called before a world is unloaded.

> Note: When overriding this function you need to call unload on each system and free them.

-----

### Function `two::World::make_entity`

``` cpp
two::Entity make_entity();
```

Creates a new entity in the world with an Active component.

-----

### Function `two::World::make_entity`

``` cpp
two::Entity make_entity(two::Entity archetype);
```

Creates a new entity and copies components from another.

-----

### Function `two::World::make_inactive_entity`

``` cpp
two::Entity make_inactive_entity();
```

Creates a new inactive entity in the world. The entity will need to have active set before it can be used by systems.

Useful to create entities without initializing them.

> Note: Inactive entities still exist in the world and can have components added to them.

-----

### Function `two::World::copy_entity`

``` cpp
void copy_entity(two::Entity dst, two::Entity src);
```

Copy components from entity `src` to entity `dst`.

-----

### Function `two::World::destroy_entity`

``` cpp
void destroy_entity(two::Entity entity);
```

Destroys an entity and all of its components.

-----

### Function `two::World::get_mask`

``` cpp
const two::EntityMask &get_mask(two::Entity entity) const;
```

Returns the entity mask

-----

### Function `two::World::pack`

``` cpp
template <typename Component>
Component &pack(two::Entity entity, const Component &component);
```

Adds or replaces a component and associates an entity with the component.

Adding components will invalidate the cache. The number of cached views is *usually* approximately equal to the number of systems, so this operation is not that expensive but you should avoid doing it every frame. This does not apply to ‘re-packing’ components (see note below).

> Note: If the entity already has a component of the same type, the old component will be replaced. Replacing a component with a new instance is *much* faster than calling `remove` and then `pack` which would result in the cache being rebuilt twice. Replacing a component does not invalidate the cache and is cheap operation.

-----

### Function `two::World::pack`

```cpp
template <typename Component, typename... Components>
void pack(two::Entity entity, const Component &h, const Components &...t);
```

Shortcut to pack multiple components to an entity, equivalent to calling `pack(entity, component)` for each component.

Unlike the original `pack` function, this function does not return a reference to the component that was just packed.

-----

### Function `two::World::unpack`

``` cpp
template <typename Component>
Component &unpack(two::Entity entity);
```

Returns a component of the given type associated with an entity.

This function will only check if the component does not exist for an entity if assertions are enabled, otherwise it is unchecked. Use has\_component if you need to verify the existence of a component before removing it. This is a cheap operation.

This function returns a reference to a component in the packed array. The reference may become invalid after `remove` is called since `remove` may move components in memory.

```cpp
auto &a = world.unpack<A>(entity1);
a.x = 5; // a is a valid reference and x will be updated.

auto b = world.unpack<B>(entity1); // copy B
world.remove<B>(entity2);
b.x = 5;
world.pack(entity1, b); // Ensures b will be updated in the array

auto &c = world.unpack<C>(entity1);
world.remove<C>(entity2);
c.x = 5; // may not update c.x in the array
```

If you plan on holding the reference it is better to copy the component and then pack it again if you have modified the component. Re-packing a component is a cheap operation and will not invalidate. the cache.

> Do not store this reference between frames such as in a member variable, store the entity instead and call unpack each frame. This operation is designed to be called multiple times per frame so it is very fast, there is no need to `cache` a component reference in a member variable.

-----

### Function `two::World::contains`

``` cpp
template <typename Component>
bool contains(two::Entity entity);
```

Returns true if a component of the given type is associated with an entity. This is a cheap operation.

-----

### Function `two::World::remove`

``` cpp
template <typename Component>
void remove(two::Entity entity);
```

Removes a component from an entity. Removing components invalidates the cache.

This function will only check if the component does not exist for an entity if assertions are enabled, otherwise it is unchecked. Use has\_component if you need to verify the existence of a component before removing it.

-----

### Function `two::World::register_component`

``` cpp
template <typename Component>
void register_component();
```

Components will be registered on their own if a new type of component is added to an entity. There is no need to call this function unless you are doing something specific that requires it.

-----

### Function `two::World::set_active`

``` cpp
void set_active(two::Entity entity, bool active);
```

Adds or removes an Active component

---

### Function `two::World::view`

```cpp
template <typename... Components>
const std::vector<two::Entity> &view(bool include_inactive = false);
```

Returns all entities that have all requested components. By default only entities with an `Active` component are matched unless `include_inactive` is true.

```cpp
for (auto entity : view<A, B, C>()) {
    auto &a = unpack<A>(entity);
    // ...
}
```

The first call to this function will build a cache with the entities that contain the requested components, subsequent calls will return the cached data as long as the data is still valid. If a cache is no longer valid, this function will rebuild the cache by applying all necessary diffs to make the cache valid.

The cost of rebuilding the cache depends on how many diff operations are needed. Any operation that changes whether an entity should be in this cache (does the entity have all requested components?) counts as a single Add or Remove diff. Functions like `remove`, `make_entity`, `pack`, `destroy_entity` may cause a cache to be invalidated. Functions that may invalidate the cache are documented.

-----

### Function `two::World::each`

```cpp
template <typename... Components>
inline void each(ViewFunc<void (Components &...)> &&fn, bool include_inactive = false);
```

 Calls `fn` with each unpacked component for every entity with all requested components.
```cpp
each<A, B, C>([](A &a, B &b, C &c) {
    // ...
});
```

This function calls `view<Components...>()` internally so the same notes about `view` apply here as well.

-----

### Function `two::World::each`

```cpp
template <typename... Components>
inline void each(ViewFunc<void (two::Entity, Components &...)> &&fn, bool include_inactive = false);
```

Calls `fn` with the entity and a reference to each unpacked component for every entity with all requested components.

```cpp
each<A, B, C>([](Entity entity, A &a, B &b, C &c) {
    // ...
});
```

This function calls `view<Components...>()` internally so the same notes about `view` apply here as well.

-----

### Function `two::World::view_one`

``` cpp
template <typename... Components>
Optional<two::Entity> view_one(bool include_inactive = false);
```

Returns the **first** entity that contains all components requested.

Views always keep entities in the order that the entity was added to the view, so `view_one()` will reliabily return the same entity that matches the constraints unless the entity was destroyed or has had one of the required components removed.

The returned optional will have no value if no entities exist with all requested components.

-----

### Function `two::World::unpack_one`

``` cpp
template <typename Component>
Component &unpack_one(bool include_inactive = false);
```

Finds the first entity with the requested component and unpacks the component requested. This is convenience function for getting at a single component in a single entity.

```cpp
auto camera = world.unpack_one<Camera>();

// is equivalent to
auto entity = world.view_one<Camera>().value();
auto camera = world.unpack<Camera>(entity);
```

Unlike `view_one()` this function will panic if no entities with the requested component were matched. Only use this function if not matching any entity should be an error, if not use `view_one()` instead.

-----

### Function `two::World::unsafe_view_all`

``` cpp
const std::vector<Entity> &unsafe_view_all();
```

Returns all entities in the world. Entities returned may be inactive.

> Note: Calling `destroy_entity()` will invalidate the iterator, use `view<>(true)` to get all entities without having `destroy_entity()` invalidate the iterator.

-----

### Function `two::World::make_system`

``` cpp
template <class T, typename... Args>
T *make_system(Args &&...args);
```

Creates and adds a system to the world. This function calls `System::load` to initialize the system.

> Systems are not unique. ‘Duplicate’ systems, that is different instances of the same system type, are allowed in the world.

-----

### Function `two::World::make_system`

``` cpp
template <class T>
T *make_system(T *system);
```

Adds a system to the world. This function calls `System::load` to initialize the system.

> You may add multiple different instances of the same system type, but passing a system pointer that points to an instance that is already in the world is not allowed and will not be checked by default. Enable `TWO_PARANOIA` to check for duplicate pointers.

-----

### Function `two::World::make_system_before`

``` cpp
template <class Before, class T, typename... Args>
T *make_system_before(Args &&...args);
```

Adds a system to the world before another system if it exists.

`Before` is an existing system. `T` is the system to be added before an existing system.

-----

### Function `two::World::get_system`

``` cpp
template <class T>
T *get_system();
```

Returns the first system that matches the given type.

System returned will be `nullptr` if it is not found.

-----

### Function `two::World::get_all_systems`

``` cpp
template <class T>
void get_all_systems(std::vector<T *> *systems);
```

Returns all system that matches the given type.

Systems returned will not be null.

-----

### Function `two::World::destroy_system`

``` cpp
void destroy_system(two::System *system);
```

System must not be null. Do not destroy a system while the main update loop is running as it could invalidate the system iterator, consider checking if the system should run or not in the system update loop instead.

-----

### Function `two::World::destroy_systems`

``` cpp
void destroy_systems();
```

Destroy all systems in the world.

-----

### Function `two::World::systems`

``` cpp
const std::vector<System *> &systems();
```

Systems returned will not be null.

-----

### Function `two::World::bind`

``` cpp
template <typename Event>
void bind(typename EventChannel<Event>::EventHandler fn);
```

Adds a function to receive events of type T

-----

### Function `two::World::bind`

``` cpp
template <typename Event, typename Func, class T>
void bind(Func fn, T this_ptr);
```

Same as `bind(fn)` but allows a member function to be used as an event handler.

```cpp
bind<KeyDownEvent>(&World::keydown, this);
```

-----

### Function `two::World::emit`

``` cpp
template <typename Event>
void emit(const Event &event) const;
```

Emits an event to all event handlers. If a handler function in the chain returns true then the event is considered handled and will not propagate to other listeners.

-----

### Function `two::World::clear_event_channels`

``` cpp
void clear_event_channels();
```

Removes all event handlers

-----

### Function `two::World::find_or_register_component`

``` cpp
template <typename Component>
two::ComponentType find_or_register_component();
```

Registers a component type if it does not exist and returns it.

Components are registered automatically so there is usually no reason to call this function.

-----

### Function `two::World::collect_unused_entities`

``` cpp
void collect_unused_entities();
```

Recycles entity ids so that they can be safely reused. This function exists to ensure we don’t reuse entity ids that are still present in some cache even though the entity has been destroyed. This can happen since cache operations are done in a ‘lazy’ manner.

This function should be called at the end of each frame.

-----

### Class `two::EventChannel`

``` cpp
template <typename Event>
class EventChannel {
public:
    using EventHandler = std::function<bool (const Event &)>;

    void bind(two::EventChannel::EventHandler &&fn);
    void emit(const Event &event) const;
};
```

An event channel handles events for a single event type.

This is an implementation detail of `EventDispatcher`.

### Function `two::EventChannel::bind`

``` cpp
void bind(two::EventChannel::EventHandler &&fn);
```

Adds a function as an event handler

-----

### Function `two::EventChannel::emit`

``` cpp
void emit(const Event &event) const;
```

Emits an event to all event handlers
