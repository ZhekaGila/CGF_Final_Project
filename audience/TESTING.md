# Participant 3 validation

Checked 2026-11-17 against upstream `audience-animation` commit `f41ce81`
(main scene at `8ad01b2`).

- CMake Release build: passed with GCC 14.2, GLFW 3.4, GLM and Mesa OpenGL
  on Linux. All application translation units compiled and linked.
- GLSL stage vertex + fragment link validation: passed with glslangValidator.
- Full scene runtime under Xvfb: shaders compiled, all scene meshes uploaded,
  audience visible; screenshot visually inspected.
- Launched from `/tmp`: passed, confirming shader lookup is independent of cwd.
- Keyboard smoke check: pause/resume, energy, three density changes, hide/show,
  Escape; application exited with code 0 and released resources.
- `git diff --check`: passed.

These checks do not replace native Apple Silicon testing or the measured FPS
comparison required on the TA machine. No Mac/TA FPS result is claimed.
