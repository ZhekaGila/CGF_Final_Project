#pragma once

#include "geometry/MeshData.h"

namespace cgf
{
    MeshData makeBox(
        const glm::vec3& size,
        const glm::vec3& color
    );

    MeshData makeCylinder(
        float radius,
        const glm::vec3& color,
        unsigned int segments = 24
    );

    MeshData makeSphere(
        const glm::vec3& color,
        unsigned int rings = 12,
        unsigned int sectors = 24
    );
}
