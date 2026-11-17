#pragma once

#include "geometry/MeshData.h"
#include <glad/glad.h>

namespace cgf {

// One reusable mesh, facing the stage (-Z), feet at local Y=0.
MeshData createSpectatorMesh();

// Must be initialized and destroyed while the OpenGL context is current.
class Audience {
public:
    Audience() = default;
    Audience(const Audience&) = delete;
    Audience& operator=(const Audience&) = delete;
    ~Audience();

    void initialize();
    // Caller binds the stage program and sets its camera/light uniforms.
    void draw(GLuint stageProgram, float animationTime, bool energetic) const;
    void release();
    void cycleDensity();
    GLsizei count() const { return visibleCount_; }

private:
    GLuint vao_ = 0, vertexBuffer_ = 0, indexBuffer_ = 0, instanceBuffer_ = 0;
    GLsizei indexCount_ = 0;
    GLsizei visibleCount_ = 168;
};

} // namespace cgf
