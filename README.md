# libintl-lite

This is a version of the *libintl-lite* library. Libintl is part of *gettext* - popular framework to manage string translations.

Because of no dependencies code should work on all platforms supporting C++17.

Differences from the original *libintl* library:
* No dependencies, easy to build
* Provides `LoadMessageCatalog()` to read *.mo* file directly without relying on `LANGUAGE` environment variable or specific folder hierarchy
* Missing some obscure features?

Differences from original *libintl-lite* library:
* Supports string contexts
* Supports plural forms
* Allocates string table as a whole which should reduce memory consumption and fragmentation

## Usage

Add this to your CMake:
```
option()
add_subdirectory(libintl-lite)
...
target_link_libraries(myapp PRIVATE libintl)
```