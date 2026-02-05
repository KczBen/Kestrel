# Vulkan rendering prototype

Learning project about Vulkan API for 3D rendering, using Vulkan 1.4 features and implementing various rendering techniques.

## Goals

This project is primarily for **learning purposes**, a fully featured release is not planned. It serves only to play and experiment with. Not all rendering techniques will be viable or performant, though graphics optimisation is one of the areas I intend to focus on.

With that said, there are some other elements to focus on.

- Technically correct API usage, the Vulkan validation layers are **always active**
- Keep the project maintainable
- Try to make implementing new effects and shaders easier

See the [ROADMAP](ROADMAP.md) for a list of planned features.

### Dependencies

Most dependencies are managed automatically. However, some libraries must be installed on the system for the project to build and run properly.

- ninja >= 1.13
- clang >= 21
- cmake >= 3.31.0
- [vcpkg](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started)
- SDL3 >= 3.4
- libvulkan >= 1.4
- vulkan-validation-layers >= 1.4

Note that a Vulkan 1.4 compatible graphics driver is required.

## Building

The project is built with CMake.

To build the project, navigate to the root directory of the project and run

```bash
cmake  --preset=clang
cd out/build
cmake --build clang
```
