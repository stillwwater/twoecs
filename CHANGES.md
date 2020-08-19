# Change Log

2020-08-18
----------

* Added `World::each` overload where the function argument takes in an `Entity` as the first parameter.

* Replace `std::unordered_map` in `ComponentArray` with a sparse to speed up entity -> component lookup.

2020-08-17
----------

* Renamed `World::has_component` to `World::contains`, and `World::remove_component` to `World::remove`. This was done to be more consistent with the `World::pack` and `World::unpack` API where the component word is implied. Unlike `World::has_component`, `World::contains` can take multiple components as template parameters.

2020-08-16
----------

* Added entity versions. Previously when an entity was destroyed there was a possibility that if the entity id was later reused and assigned to a new entity, the destroyed Entity would now refer to the components of the newly assigned entity. `Entity` continues to be defined as an integer, but now the integer id contains both an index and a version number. The version number is incremented each time an entity index is reused, making it impossible for a new entity to take on the same id as a destroyed entity.
* Added variadic version of `World::pack` which can take more than one component.

* Removed `two::EventDispatcher`. `World` now handles event channels.

* Fixed `World::view<>(true)` returning `NullEntity`. Use `World::unsafe_view_all` to iterate through all entities including the `NullEntity`.

2020-08-15
----------

* Added `World::each` as an alternative to `World::view`.
