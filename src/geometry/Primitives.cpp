#include "geometry/Primitives.h"

#include <cmath>
#include <cstdint>

namespace cgf
{
    namespace
    {
        constexpr float PI = 3.14159265359f;
    }

    MeshData makeBox(
        const glm::vec3& size,
        const glm::vec3& color
    )
    {
        MeshData mesh;
        mesh.name = "Box";

        const glm::vec3 h = size * 0.5f;

        const glm::vec2 uv[4] = {
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {1.0f, 1.0f},
            {0.0f, 1.0f}
        };

        auto addFace = [&](const glm::vec3& normal,
                           const glm::vec3& a,
                           const glm::vec3& b,
                           const glm::vec3& c,
                           const glm::vec3& d)
        {
            const std::uint32_t start =
                static_cast<std::uint32_t>(mesh.vertices.size());

            mesh.vertices.push_back({a, normal, uv[0], color});
            mesh.vertices.push_back({b, normal, uv[1], color});
            mesh.vertices.push_back({c, normal, uv[2], color});
            mesh.vertices.push_back({d, normal, uv[3], color});

            mesh.indices.insert(
                mesh.indices.end(),
                {
                    start, start + 1, start + 2,
                    start, start + 2, start + 3
                }
            );
        };

        // Front (+Z)
        addFace(
            {0, 0, 1},
            {-h.x, -h.y, h.z},
            { h.x, -h.y, h.z},
            { h.x,  h.y, h.z},
            {-h.x,  h.y, h.z}
        );

        // Back (-Z)
        addFace(
            {0, 0, -1},
            { h.x, -h.y, -h.z},
            {-h.x, -h.y, -h.z},
            {-h.x,  h.y, -h.z},
            { h.x,  h.y, -h.z}
        );

        // Right (+X)
        addFace(
            {1, 0, 0},
            {h.x, -h.y, h.z},
            {h.x, -h.y, -h.z},
            {h.x,  h.y, -h.z},
            {h.x,  h.y, h.z}
        );

        // Left (-X)
        addFace(
            {-1, 0, 0},
            {-h.x, -h.y, -h.z},
            {-h.x, -h.y,  h.z},
            {-h.x,  h.y,  h.z},
            {-h.x,  h.y, -h.z}
        );

        // Top (+Y)
        addFace(
            {0, 1, 0},
            {-h.x, h.y,  h.z},
            { h.x, h.y,  h.z},
            { h.x, h.y, -h.z},
            {-h.x, h.y, -h.z}
        );

        // Bottom (-Y)
        addFace(
            {0, -1, 0},
            {-h.x, -h.y, -h.z},
            { h.x, -h.y, -h.z},
            { h.x, -h.y,  h.z},
            {-h.x, -h.y,  h.z}
        );

        return mesh;
    }


    MeshData makeCylinder(
        float radius,
        const glm::vec3& color,
        unsigned int segments
    )
    {
        MeshData mesh;
        mesh.name = "Cylinder";

        if (radius <= 0.0f || segments < 3)
            return mesh;

        // Cylinder height = 1, axis = Y.
        for (unsigned int i = 0; i <= segments; ++i)
        {
            const float angle =
                2.0f * PI * static_cast<float>(i) /
                static_cast<float>(segments);

            const float x = std::cos(angle);
            const float z = std::sin(angle);

            const glm::vec3 normal(x, 0.0f, z);

            mesh.vertices.push_back({
                {radius * x, -0.5f, radius * z},
                normal,
                {static_cast<float>(i) / segments, 0.0f},
                color
            });

            mesh.vertices.push_back({
                {radius * x, 0.5f, radius * z},
                normal,
                {static_cast<float>(i) / segments, 1.0f},
                color
            });
        }

        // Side triangles
        for (unsigned int i = 0; i < segments; ++i)
        {
            const std::uint32_t b = i * 2;
            const std::uint32_t t = b + 1;
            const std::uint32_t bn = b + 2;
            const std::uint32_t tn = b + 3;

            mesh.indices.insert(
                mesh.indices.end(),
                {b, t, tn, b, tn, bn}
            );
        }

        // Top cap
        const std::uint32_t topCenter =
            static_cast<std::uint32_t>(mesh.vertices.size());

        mesh.vertices.push_back({
            {0, 0.5f, 0},
            {0, 1, 0},
            {0.5f, 0.5f},
            color
        });

        for (unsigned int i = 0; i <= segments; ++i)
        {
            const float angle =
                2.0f * PI * static_cast<float>(i) /
                static_cast<float>(segments);

            const float x = std::cos(angle);
            const float z = std::sin(angle);

            mesh.vertices.push_back({
                {radius * x, 0.5f, radius * z},
                {0, 1, 0},
                {0.5f + 0.5f * x, 0.5f + 0.5f * z},
                color
            });
        }

        for (unsigned int i = 0; i < segments; ++i)
        {
            mesh.indices.insert(
                mesh.indices.end(),
                {
                    topCenter,
                    topCenter + i + 2,
                    topCenter + i + 1
                }
            );
        }

        // Bottom cap
        const std::uint32_t bottomCenter =
            static_cast<std::uint32_t>(mesh.vertices.size());

        mesh.vertices.push_back({
            {0, -0.5f, 0},
            {0, -1, 0},
            {0.5f, 0.5f},
            color
        });

        for (unsigned int i = 0; i <= segments; ++i)
        {
            const float angle =
                2.0f * PI * static_cast<float>(i) /
                static_cast<float>(segments);

            const float x = std::cos(angle);
            const float z = std::sin(angle);

            mesh.vertices.push_back({
                {radius * x, -0.5f, radius * z},
                {0, -1, 0},
                {0.5f + 0.5f * x, 0.5f + 0.5f * z},
                color
            });
        }

        for (unsigned int i = 0; i < segments; ++i)
        {
            mesh.indices.insert(
                mesh.indices.end(),
                {
                    bottomCenter,
                    bottomCenter + i + 1,
                    bottomCenter + i + 2
                }
            );
        }

        return mesh;
    }


    MeshData makeSphere(
        const glm::vec3& color,
        unsigned int rings,
        unsigned int sectors
    )
    {
        MeshData mesh;
        mesh.name = "Sphere";

        if (rings < 2 || sectors < 3)
            return mesh;

        for (unsigned int i = 0; i <= rings; ++i)
        {
            const float phi =
                PI * static_cast<float>(i) /
                static_cast<float>(rings);

            for (unsigned int j = 0; j <= sectors; ++j)
            {
                const float theta =
                    2.0f * PI * static_cast<float>(j) /
                    static_cast<float>(sectors);

                const glm::vec3 position(
                    std::sin(phi) * std::cos(theta),
                    std::cos(phi),
                    std::sin(phi) * std::sin(theta)
                );

                mesh.vertices.push_back({
                    position,
                    glm::normalize(position),
                    {
                        static_cast<float>(j) / sectors,
                        static_cast<float>(i) / rings
                    },
                    color
                });
            }
        }

        for (unsigned int i = 0; i < rings; ++i)
        {
            for (unsigned int j = 0; j < sectors; ++j)
            {
                const std::uint32_t a =
                    i * (sectors + 1) + j;

                const std::uint32_t b =
                    a + sectors + 1;

                if (i != 0)
                {
                    mesh.indices.insert(
                        mesh.indices.end(),
                        {a, a + 1, b}
                    );
                }

                if (i != rings - 1)
                {
                    mesh.indices.insert(
                        mesh.indices.end(),
                        {a + 1, b + 1, b}
                    );
                }
            }
        }

        return mesh;
    }
}
