# Vulkan ECS Engine

A minimal game engine written in C++20 with Vulkan

---

![demo](https://github.com/user-attachments/assets/43e76357-cc29-4d49-a576-cce8fb7f4f2d)

---

## Architecture

```
Engine
├── core/
│   ├── Window          GLFW window + resize callbacks
│   └── Engine          Main loop, owns all subsystems
│
├── ecs/
│   ├── Types.h         EntityID alias, NULL_ENTITY sentinel
│   ├── ComponentPool   Sparse-set storage (O(1) add/remove/lookup)
│   └── Registry        Entity lifecycle + typed View iteration
│
├── components/         Plain data – no logic, no virtual calls
│   ├── Transform       position / rotation (quat) / scale → mat4
│   ├── Mesh            CPU-side vertices + indices
│   └── Camera          view/projection matrix helpers
│
├── renderer/
│   ├── VulkanContext   Instance, device, queues, buffer utilities
│   ├── Swapchain       Swapchain, render pass, framebuffers, depth
│   └── Renderer        Pipeline, UBOs, GPU mesh cache, frame recording
│
└── systems/
    └── RenderSystem    Bridges ECS → Renderer; lazy GPU uploads
```

### Key design decisions

| Decision | Rationale |
|---|---|
| **Sparse-set ECS** | O(1) all operations, cache-friendly linear iteration – same approach as EnTT |
| **View<Ts...>** | Zero-allocation, compile-time multi-component iteration via parameter packs |
| **Push constants for model matrix** | Fastest per-draw path in Vulkan; no descriptor overhead |
| **UBO for camera** | Changes once per frame, shared across all draws – ideal for a descriptor set |
| **Staging → device-local buffers** | Correct production Vulkan memory model; avoids slow host-visible paths |
| **MAX_FRAMES_IN_FLIGHT = 2** | Double-buffer CPU/GPU work; eliminates pipeline stalls |
| **Dynamic viewport/scissor** | Resize without rebuilding the pipeline |
| **Components as plain data** | Systems own logic; components own state – strict SRP |

---

## Building

### Prerequisites

| Tool | Version |
|---|---|
| CMake | ≥ 3.20 |
| C++ compiler | GCC 12 / Clang 15 / MSVC 19.34 (C++20 required) |
| Vulkan SDK | ≥ 1.3 (includes `glslc`) |
| GLFW | 3.3+ |
| GLM | Any recent |

### Linux / macOS

```bash
sudo apt install libglfw3-dev libglm-dev

source ~/VulkanSDK/<version>/setup-env.sh

cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
./build/VulkanECSEngine
```

### Windows (MSVC)

```powershell

cmake -B build -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Debug
.\build\Debug\VulkanECSEngine.exe
```

---

## What you'll see

Three objects on a dark background:

- **Centre** – coloured cube (one colour per face)
- **Right**  – smaller cube rotated 45° on the Y axis
- **Left**   – white-apex pyramid

---

## Extending the engine

### Add a component
```cpp
struct Velocity { glm::vec3 linear{0.0f}; float angularY{0.0f}; };
```

### Add a system
```cpp
void PhysicsSystem::update(float dt) {
    m_registry.view<Transform, Velocity>().each(
        [dt](EntityID, Transform& t, Velocity& v) {
            t.position += v.linear * dt;
            t.rotation  = glm::angleAxis(v.angularY * dt, glm::vec3(0,1,0)) * t.rotation;
        });
}
```

### Add a new entity at runtime
```cpp
auto e = engine.registry().create();
engine.registry().emplace<Transform>(e, Transform{ .position = {5,0,0} });
engine.registry().emplace<Mesh>(e, makeCube());
engine.registry().emplace<Velocity>(e, Velocity{ .angularY = 1.5f });
```

---
