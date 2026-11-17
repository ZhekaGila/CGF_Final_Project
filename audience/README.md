# Audience & Animation — Participant 3

## Implemented

- [x] One low-poly spectator: head, hair, eyes, torso, raised arms, hands, legs and shoes.
- [x] 168 spectators, 14 columns × 12 rows on the +Z side of the stage.
- [x] Time-based bounce/sway with different phases, heights and shirt colors.
- [x] `glDrawElementsInstanced`, `glVertexAttribDivisor`, static instance data.
- [x] Pause, calm/energetic mode, density selection, visibility comparison, FPS title.

The model faces -Z (toward the musicians). Local feet start at Y=0. The stage
ends at Z=5; the crowd starts at Z=6.1 and has its own floor. Maximum jump
height is 0.24 times the energy setting, with no negative ground displacement.

## How it works

`createSpectatorMesh()` joins thirteen boxes into one mesh: 312 vertices and
468 indices (156 triangles). Each instance stores two `vec4`s: position and
uniform scale, then clothing tint and animation phase. Attributes 4 and 5 use
divisor 1, so all the spectator's vertices share the same instance parameters.

The stage vertex shader chooses its path with `uAudience`. Stage meshes use
the original model matrix. Spectators use:

```
beat = time * (3.8 + 0.25 * sin(phase)) + phase
bounce = max(sin(beat), 0) * 0.24 * energy
worldPosition = position + scale * Rz(sway) * localPosition + (0, bounce, 0)
```

Normals use the same rotation; uniform positive scale does not change their
direction. The fragment shader is unchanged, so the audience receives the
existing scene lighting. All body parts inherit one spectator transform.
Pause freezes only audience time, while spotlights continue moving.

The complete crowd costs one draw call and 26,208 triangles at full density.
No geometry, instance matrices or instance colors are uploaded every frame.
Only time, energy and the instancing flag change. `Audience::release()` runs
before GLFW destroys the OpenGL context and is safe to call again.

## Integration points

- `CMakeLists.txt`: compile `src/audience/Audience.cpp`; GLFW minimum 3.3;
  absolute shader source path for IDE/Terminal launches.
- `src/main.cpp`: initialize after context creation, add audience floor,
  update local animation clock/input, draw after opaque stage and before glow,
  release before context destruction. Starting camera is widened for the crowd.
- `shaders/stage.vert`: instanced path; original stage path is preserved.
- No extra runtime libraries, image files or external models are required.

Participant 2 can keep the fragment lighting implementation. Participant 4 can
replace the fixed camera and must preserve the crowd keys or document new ones.
If a teammate replaces `stage.vert`, retain locations 4/5 and audience uniforms.

## Manual verification on the target machine

1. Build using the root README. Confirm the stage and crowd are both visible.
2. Confirm spectators face the stage, stand on the floor, and do not intersect it.
3. Watch several seconds: spectators should bounce at different times and sway.
4. Press P: crowd freezes while lights keep moving. Press P again: no time jump.
5. Press J: calm movement has smaller amplitude. Press N three times: 56, 112, 168.
6. Hold N: only one change per key press. H hides/shows the crowd; Esc closes cleanly.
7. Resize/minimize/restore the window; the scene should keep its aspect ratio.
8. Record FPS for at least 10 seconds per case at 1280×720; keep hardware,
   build type, VSync and camera identical. H allows a no-crowd baseline.

| Hardware / OS | Build / VSync | Spectators | Resolution | Average FPS |
| --- | --- | --- | --- | --- |
| Fill after measurement | Release / on | 0 | 1280×720 | Not measured |
| Fill after measurement | Release / on | 56 | 1280×720 | Not measured |
| Fill after measurement | Release / on | 112 | 1280×720 | Not measured |
| Fill after measurement | Release / on | 168 | 1280×720 | Not measured |

VSync can cap all cases at the display refresh rate. Do not interpret equal
capped FPS as proof of zero rendering cost. Retina framebuffer resolution can
differ from logical window size. Report the actual framebuffer resolution.
The target requirement is stable 30+ FPS on the TA machine; no target-hardware
performance result is claimed here.

## Short defense explanation

I built one spectator from low-poly boxes and reused it for the crowd.
Instead of issuing 168 draw calls, I upload one model and one static instance
buffer. Each instance has its own location, scale, color and phase. The vertex
shader computes bounce and sway from elapsed time. That keeps movement frame
rate independent and avoids rebuilding or uploading the crowd every frame.
Normals rotate with the character, so the existing lighting still works.

Explain: a VAO describes attributes; a VBO stores vertices/instances; an EBO
stores triangle indices; divisor 1 advances data once per spectator. The shared
shader flag resets after drawing the audience to avoid changing stage rendering.

## Remaining team deliverables

The final report, screenshots, measured FPS table, presentation, demo video,
release tag and other participants' features remain team tasks. Complete the
AI Use note with what was personally reviewed and changed before submitting.
