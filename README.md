# CGF Final Project — Concert Stage

Real-time concert scene using C++17, OpenGL 3.3 Core, GLFW, GLAD and GLM.

## Features

- Procedural stage, instruments, performers and moving spotlights.
- Participant 3: one procedural low-poly spectator (13 boxes / 156 triangles),
  168 instances in front of the stage, six shirt colors and varied heights.
- GPU bounce and sway with individual phases; one indexed instanced draw call
  for the entire crowd. Static geometry and instance buffers upload once.
- Audience floor, crowd controls, FPS and control hints in the window title.

## Build: macOS Apple Silicon (Homebrew)

Install Xcode command line tools once (`xcode-select --install`) and Homebrew
if absent (https://brew.sh). Then, from the repository root:

```bash
brew install cmake glfw glm
cmake -S . -B build/homebrew -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(brew --prefix)"
cmake --build build/homebrew --parallel
./build/homebrew/ConcertStage
```

GLAD is already included; do not install/link GLEW for this application.
CMake embeds the shader source directory so launching from a build directory
also works. Reconfigure after moving the source tree.

## Build: existing vcpkg presets

Set `VCPKG_ROOT` to your vcpkg checkout. On Apple Silicon:

```bash
cmake --preset macos-arm64
cmake --build --preset macos-debug
./build/macos-arm64/ConcertStage
```

On Windows with the Visual Studio 2026 generator specified by the existing preset:

```powershell
cmake --preset windows-x64
cmake --build --preset windows-debug
.\build\windows-x64\Debug\ConcertStage.exe
```

For another Visual Studio version use a matching generator or edit a local
`CMakeUserPresets.json`. Use a separate build directory per toolchain.

## Controls

| Key | Action |
| --- | --- |
| P | Pause/resume audience animation (lights keep moving) |
| J | Toggle energetic / calm movement |
| N | Cycle spectator count: 168 → 56 → 112 → 168 |
| H | Hide/show crowd for an FPS comparison |
| Esc | Close window |

Click the window and use the English keyboard layout. The current main-branch
camera is fixed; camera controls remain Participant 4's work. The starting view
has been widened to include both the stage and the crowd.

## Team responsibilities

- Participant 1 — Stage and 3D Models
- Participant 2 — Lighting and Shaders
- Participant 3 — Nurila Berdaly: Audience and Animation
- Participant 4 — Camera, Controls and Light Show
- Participant 5 — Music, Effects and Integration

## Participant 3 integration and verification

See [audience/README.md](audience/README.md) for architecture, demonstration,
performance measurement and defense notes. This contribution does not claim
that the whole team's final report, camera, procedural texture, video or release
is complete. FPS must be measured on the target Mac/TA machine.

## Project layout

- `include/geometry`, `src/geometry`: existing CPU geometry
- `include/audience`, `src/audience`: spectator mesh and instance renderer
- `src/main.cpp`: scene integration, lighting and input
- `shaders`: separate GLSL programs
- `external/glad`: included OpenGL loader

## AI Use

ChatGPT/Codex assisted with Participant 3's procedural spectator model,
instancing implementation, shader animation, integration, build compatibility
and documentation. The participant must review and understand the code,
verify it on their own machine and describe their own subsequent changes in
the final report. Existing stage and lighting contributions are team work;
this note does not attribute them to Participant 3.
