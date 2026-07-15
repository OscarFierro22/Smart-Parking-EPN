#ifndef COLLISION_MESH_H
#define COLLISION_MESH_H

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

class CollisionMesh
{
public:
    bool loadFromObj(const std::string& path)
    {
        clear();

        std::ifstream file(path);
        if (!file.is_open())
        {
            std::cerr << "No se pudo abrir la malla de colision: " << path << '\n';
            return false;
        }

        std::vector<glm::vec3> vertices;
        vertices.reserve(60000);

        std::string line;
        while (std::getline(file, line))
        {
            if (line.size() < 2)
                continue;

            if (line[0] == 'v' && line[1] == ' ')
            {
                std::istringstream stream(line.substr(2));
                glm::vec3 vertex(0.0f);
                stream >> vertex.x >> vertex.y >> vertex.z;
                vertices.push_back(vertex);
            }
            else if (line[0] == 'f' && line[1] == ' ')
            {
                std::istringstream stream(line.substr(2));
                std::vector<int> faceIndices;
                std::string token;

                while (stream >> token)
                {
                    const std::size_t slash = token.find('/');
                    const std::string vertexText = token.substr(0, slash);
                    if (vertexText.empty())
                        continue;

                    int index = std::stoi(vertexText);
                    if (index > 0)
                        index -= 1;
                    else if (index < 0)
                        index = static_cast<int>(vertices.size()) + index;

                    if (index >= 0 && index < static_cast<int>(vertices.size()))
                        faceIndices.push_back(index);
                }

                if (faceIndices.size() < 3)
                    continue;

                for (std::size_t i = 1; i + 1 < faceIndices.size(); ++i)
                {
                    addTriangle(
                        vertices[faceIndices[0]],
                        vertices[faceIndices[i]],
                        vertices[faceIndices[i + 1]]
                    );
                }
            }
        }

        buildSpatialGrid();

        std::cout
            << "Malla de colision cargada: "
            << triangles_.size()
            << " triangulos.\n";

        return !triangles_.empty();
    }

    void setGroundPlane(float y)
    {
        groundPlaneEnabled_ = true;
        groundPlaneY_ = y;
    }

    void addSolidBox(
        const glm::vec3& center,
        const glm::vec3& fullSize
    )
    {
        const glm::vec3 halfSize = fullSize * 0.5f;
        solidBoxes_.push_back(SolidBox{
            center - halfSize,
            center + halfSize
        });
    }

    bool empty() const
    {
        return triangles_.empty();
    }

    glm::vec3 moveCamera(
        const glm::vec3& startEye,
        const glm::vec3& desiredEye
    ) const
    {
        if (triangles_.empty())
            return desiredEye;

        const glm::vec3 totalMovement = desiredEye - startEye;
        const float movementLength = glm::length(totalMovement);

        if (movementLength < 0.000001f)
            return resolveCapsule(startEye, startEye);

        // Los pasos pequeños evitan atravesar superficies delgadas cuando
        // la cámara se mueve rápidamente.
        const float maximumStepLength = CAMERA_RADIUS * 0.45f;
        const int stepCount = std::max(
            1,
            static_cast<int>(std::ceil(movementLength / maximumStepLength))
        );

        const glm::vec3 stepMovement =
            totalMovement / static_cast<float>(stepCount);

        glm::vec3 current = startEye;

        for (int step = 0; step < stepCount; ++step)
        {
            const glm::vec3 previous = current;
            const glm::vec3 candidate = current + stepMovement;
            current = resolveCapsule(candidate, previous);
        }

        return current;
    }

private:
    struct Triangle
    {
        glm::vec3 a;
        glm::vec3 b;
        glm::vec3 c;
        glm::vec3 normal;
        glm::vec3 minimum;
        glm::vec3 maximum;
    };

    struct SolidBox
    {
        glm::vec3 minimum;
        glm::vec3 maximum;
    };

    struct CellKey
    {
        int x;
        int y;
        int z;

        bool operator==(const CellKey& other) const
        {
            return x == other.x && y == other.y && z == other.z;
        }
    };

    struct CellKeyHash
    {
        std::size_t operator()(const CellKey& key) const noexcept
        {
            const std::size_t hx = std::hash<int>{}(key.x);
            const std::size_t hy = std::hash<int>{}(key.y);
            const std::size_t hz = std::hash<int>{}(key.z);
            return hx ^ (hy << 1U) ^ (hz << 2U);
        }
    };

    static constexpr float CAMERA_RADIUS = 0.34f;
    static constexpr float EYE_TO_FEET = 1.65f;
    static constexpr float HEAD_ABOVE_EYE = 0.15f;
    static constexpr int CAPSULE_SPHERE_COUNT = 5;
    static constexpr int RESOLUTION_ITERATIONS = 6;
    static constexpr float COLLISION_MARGIN = 0.003f;
    static constexpr float CELL_SIZE = 3.0f;

    std::vector<Triangle> triangles_;
    std::unordered_map<CellKey, std::vector<int>, CellKeyHash> spatialGrid_;
    std::vector<SolidBox> solidBoxes_;
    bool groundPlaneEnabled_ = false;
    float groundPlaneY_ = -5.30f;

    void clear()
    {
        triangles_.clear();
        spatialGrid_.clear();
        solidBoxes_.clear();
        groundPlaneEnabled_ = false;
        groundPlaneY_ = -5.30f;
    }

    static glm::vec3 componentMinimum(
        const glm::vec3& left,
        const glm::vec3& right
    )
    {
        return glm::vec3(
            std::min(left.x, right.x),
            std::min(left.y, right.y),
            std::min(left.z, right.z)
        );
    }

    static glm::vec3 componentMaximum(
        const glm::vec3& left,
        const glm::vec3& right
    )
    {
        return glm::vec3(
            std::max(left.x, right.x),
            std::max(left.y, right.y),
            std::max(left.z, right.z)
        );
    }

    void addTriangle(
        const glm::vec3& a,
        const glm::vec3& b,
        const glm::vec3& c
    )
    {
        const glm::vec3 crossProduct = glm::cross(b - a, c - a);
        const float normalLength = glm::length(crossProduct);

        if (normalLength < 0.000001f)
            return;

        Triangle triangle;
        triangle.a = a;
        triangle.b = b;
        triangle.c = c;
        triangle.normal = crossProduct / normalLength;
        triangle.minimum = componentMinimum(a, componentMinimum(b, c));
        triangle.maximum = componentMaximum(a, componentMaximum(b, c));

        triangles_.push_back(triangle);
    }

    static int cellCoordinate(float value)
    {
        return static_cast<int>(std::floor(value / CELL_SIZE));
    }

    void buildSpatialGrid()
    {
        spatialGrid_.clear();

        for (int triangleIndex = 0;
             triangleIndex < static_cast<int>(triangles_.size());
             ++triangleIndex)
        {
            const Triangle& triangle = triangles_[triangleIndex];

            const int minimumX = cellCoordinate(triangle.minimum.x);
            const int minimumY = cellCoordinate(triangle.minimum.y);
            const int minimumZ = cellCoordinate(triangle.minimum.z);
            const int maximumX = cellCoordinate(triangle.maximum.x);
            const int maximumY = cellCoordinate(triangle.maximum.y);
            const int maximumZ = cellCoordinate(triangle.maximum.z);

            for (int x = minimumX; x <= maximumX; ++x)
            {
                for (int y = minimumY; y <= maximumY; ++y)
                {
                    for (int z = minimumZ; z <= maximumZ; ++z)
                    {
                        spatialGrid_[CellKey{ x, y, z }].push_back(triangleIndex);
                    }
                }
            }
        }
    }

    std::vector<int> queryNearbyTriangles(
        const glm::vec3& center,
        float radius
    ) const
    {
        std::vector<int> result;

        const glm::vec3 minimum = center - glm::vec3(radius);
        const glm::vec3 maximum = center + glm::vec3(radius);

        const int minimumX = cellCoordinate(minimum.x);
        const int minimumY = cellCoordinate(minimum.y);
        const int minimumZ = cellCoordinate(minimum.z);
        const int maximumX = cellCoordinate(maximum.x);
        const int maximumY = cellCoordinate(maximum.y);
        const int maximumZ = cellCoordinate(maximum.z);

        for (int x = minimumX; x <= maximumX; ++x)
        {
            for (int y = minimumY; y <= maximumY; ++y)
            {
                for (int z = minimumZ; z <= maximumZ; ++z)
                {
                    const auto iterator = spatialGrid_.find(CellKey{ x, y, z });
                    if (iterator == spatialGrid_.end())
                        continue;

                    result.insert(
                        result.end(),
                        iterator->second.begin(),
                        iterator->second.end()
                    );
                }
            }
        }

        std::sort(result.begin(), result.end());
        result.erase(std::unique(result.begin(), result.end()), result.end());
        return result;
    }

    static glm::vec3 closestPointOnTriangle(
        const glm::vec3& point,
        const glm::vec3& a,
        const glm::vec3& b,
        const glm::vec3& c
    )
    {
        const glm::vec3 ab = b - a;
        const glm::vec3 ac = c - a;
        const glm::vec3 ap = point - a;

        const float d1 = glm::dot(ab, ap);
        const float d2 = glm::dot(ac, ap);
        if (d1 <= 0.0f && d2 <= 0.0f)
            return a;

        const glm::vec3 bp = point - b;
        const float d3 = glm::dot(ab, bp);
        const float d4 = glm::dot(ac, bp);
        if (d3 >= 0.0f && d4 <= d3)
            return b;

        const float vc = d1 * d4 - d3 * d2;
        if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
        {
            const float v = d1 / (d1 - d3);
            return a + v * ab;
        }

        const glm::vec3 cp = point - c;
        const float d5 = glm::dot(ab, cp);
        const float d6 = glm::dot(ac, cp);
        if (d6 >= 0.0f && d5 <= d6)
            return c;

        const float vb = d5 * d2 - d1 * d6;
        if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
        {
            const float w = d2 / (d2 - d6);
            return a + w * ac;
        }

        const float va = d3 * d6 - d5 * d4;
        if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
        {
            const float w =
                (d4 - d3) /
                ((d4 - d3) + (d5 - d6));
            return b + w * (c - b);
        }

        const float denominator = 1.0f / (va + vb + vc);
        const float v = vb * denominator;
        const float w = vc * denominator;
        return a + ab * v + ac * w;
    }

    static bool sphereAabbOverlap(
        const glm::vec3& center,
        float radius,
        const Triangle& triangle
    )
    {
        return
            center.x + radius >= triangle.minimum.x &&
            center.x - radius <= triangle.maximum.x &&
            center.y + radius >= triangle.minimum.y &&
            center.y - radius <= triangle.maximum.y &&
            center.z + radius >= triangle.minimum.z &&
            center.z - radius <= triangle.maximum.z;
    }

    static float capsuleSphereOffset(int sphereIndex)
    {
        const float bottomCenter = -EYE_TO_FEET + CAMERA_RADIUS;
        const float topCenter = HEAD_ABOVE_EYE - CAMERA_RADIUS;
        const float t =
            static_cast<float>(sphereIndex) /
            static_cast<float>(CAPSULE_SPHERE_COUNT - 1);
        return bottomCenter + (topCenter - bottomCenter) * t;
    }

    static glm::vec3 clampPointToBox(
        const glm::vec3& point,
        const SolidBox& box
    )
    {
        return glm::vec3(
            std::clamp(point.x, box.minimum.x, box.maximum.x),
            std::clamp(point.y, box.minimum.y, box.maximum.y),
            std::clamp(point.z, box.minimum.z, box.maximum.z)
        );
    }

    static glm::vec3 pushSphereOutsideBox(
        const glm::vec3& sphereCenter,
        const glm::vec3& previousSphereCenter,
        float radius,
        const SolidBox& box,
        bool& collided
    )
    {
        collided = false;

        const glm::vec3 closestPoint = clampPointToBox(sphereCenter, box);
        const glm::vec3 difference = sphereCenter - closestPoint;
        const float distanceSquared = glm::dot(difference, difference);

        const bool centerInside =
            sphereCenter.x >= box.minimum.x && sphereCenter.x <= box.maximum.x &&
            sphereCenter.y >= box.minimum.y && sphereCenter.y <= box.maximum.y &&
            sphereCenter.z >= box.minimum.z && sphereCenter.z <= box.maximum.z;

        if (!centerInside)
        {
            if (distanceSquared >= radius * radius)
                return glm::vec3(0.0f);

            const float distance = std::sqrt(std::max(distanceSquared, 0.0f));
            if (distance < 0.00001f)
                return glm::vec3(0.0f);

            collided = true;
            return (difference / distance) * (radius - distance);
        }

        // El centro entró en la caja. Se devuelve hacia la cara por la que
        // venía; si ya estaba dentro, se elige la salida más cercana.
        struct Candidate
        {
            float distance;
            glm::vec3 correction;
        };

        std::vector<Candidate> candidates;
        candidates.reserve(6);

        auto addCandidate = [&](float distance, const glm::vec3& correction)
        {
            candidates.push_back(Candidate{ std::abs(distance), correction });
        };

        if (previousSphereCenter.x < box.minimum.x)
        {
            collided = true;
            return glm::vec3(box.minimum.x - radius - sphereCenter.x, 0.0f, 0.0f);
        }
        if (previousSphereCenter.x > box.maximum.x)
        {
            collided = true;
            return glm::vec3(box.maximum.x + radius - sphereCenter.x, 0.0f, 0.0f);
        }
        if (previousSphereCenter.y < box.minimum.y)
        {
            collided = true;
            return glm::vec3(0.0f, box.minimum.y - radius - sphereCenter.y, 0.0f);
        }
        if (previousSphereCenter.y > box.maximum.y)
        {
            collided = true;
            return glm::vec3(0.0f, box.maximum.y + radius - sphereCenter.y, 0.0f);
        }
        if (previousSphereCenter.z < box.minimum.z)
        {
            collided = true;
            return glm::vec3(0.0f, 0.0f, box.minimum.z - radius - sphereCenter.z);
        }
        if (previousSphereCenter.z > box.maximum.z)
        {
            collided = true;
            return glm::vec3(0.0f, 0.0f, box.maximum.z + radius - sphereCenter.z);
        }

        addCandidate(
            sphereCenter.x - box.minimum.x,
            glm::vec3(box.minimum.x - radius - sphereCenter.x, 0.0f, 0.0f)
        );
        addCandidate(
            box.maximum.x - sphereCenter.x,
            glm::vec3(box.maximum.x + radius - sphereCenter.x, 0.0f, 0.0f)
        );
        addCandidate(
            sphereCenter.y - box.minimum.y,
            glm::vec3(0.0f, box.minimum.y - radius - sphereCenter.y, 0.0f)
        );
        addCandidate(
            box.maximum.y - sphereCenter.y,
            glm::vec3(0.0f, box.maximum.y + radius - sphereCenter.y, 0.0f)
        );
        addCandidate(
            sphereCenter.z - box.minimum.z,
            glm::vec3(0.0f, 0.0f, box.minimum.z - radius - sphereCenter.z)
        );
        addCandidate(
            box.maximum.z - sphereCenter.z,
            glm::vec3(0.0f, 0.0f, box.maximum.z + radius - sphereCenter.z)
        );

        const auto nearest = std::min_element(
            candidates.begin(),
            candidates.end(),
            [](const Candidate& left, const Candidate& right)
            {
                return left.distance < right.distance;
            }
        );

        collided = true;
        return nearest->correction;
    }

    glm::vec3 resolveCapsule(
        glm::vec3 eyePosition,
        const glm::vec3& previousEyePosition
    ) const
    {
        // El piso procedural exterior también participa en la colisión.
        if (groundPlaneEnabled_)
        {
            const float minimumEyeY = groundPlaneY_ + EYE_TO_FEET;
            if (eyePosition.y < minimumEyeY)
                eyePosition.y = minimumEyeY;
        }

        for (int iteration = 0;
             iteration < RESOLUTION_ITERATIONS;
             ++iteration)
        {
            bool corrected = false;

            for (int sphereIndex = 0;
                 sphereIndex < CAPSULE_SPHERE_COUNT;
                 ++sphereIndex)
            {
                const float verticalOffset = capsuleSphereOffset(sphereIndex);
                glm::vec3 sphereCenter =
                    eyePosition + glm::vec3(0.0f, verticalOffset, 0.0f);

                const std::vector<int> nearbyTriangles =
                    queryNearbyTriangles(
                        sphereCenter,
                        CAMERA_RADIUS + COLLISION_MARGIN
                    );

                for (const int triangleIndex : nearbyTriangles)
                {
                    const Triangle& triangle = triangles_[triangleIndex];

                    if (!sphereAabbOverlap(
                            sphereCenter,
                            CAMERA_RADIUS + COLLISION_MARGIN,
                            triangle
                        ))
                    {
                        continue;
                    }

                    const glm::vec3 closestPoint = closestPointOnTriangle(
                        sphereCenter,
                        triangle.a,
                        triangle.b,
                        triangle.c
                    );

                    glm::vec3 difference = sphereCenter - closestPoint;
                    const float distanceSquared = glm::dot(difference, difference);
                    const float permittedDistance =
                        CAMERA_RADIUS + COLLISION_MARGIN;

                    if (distanceSquared >= permittedDistance * permittedDistance)
                        continue;

                    const float distance = std::sqrt(
                        std::max(distanceSquared, 0.0f)
                    );

                    glm::vec3 pushDirection(0.0f);

                    if (distance > 0.00001f)
                    {
                        pushDirection = difference / distance;
                    }
                    else
                    {
                        pushDirection = triangle.normal;

                        const glm::vec3 previousSphereCenter =
                            previousEyePosition +
                            glm::vec3(0.0f, verticalOffset, 0.0f);

                        const float previousSide = glm::dot(
                            previousSphereCenter - triangle.a,
                            pushDirection
                        );

                        if (previousSide < 0.0f)
                            pushDirection = -pushDirection;
                    }

                    const float penetration = permittedDistance - distance;
                    eyePosition += pushDirection * penetration;
                    sphereCenter += pushDirection * penetration;
                    corrected = true;
                }

                for (const SolidBox& box : solidBoxes_)
                {
                    bool boxCollision = false;
                    const glm::vec3 previousSphereCenter =
                        previousEyePosition +
                        glm::vec3(0.0f, verticalOffset, 0.0f);

                    const glm::vec3 correction = pushSphereOutsideBox(
                        sphereCenter,
                        previousSphereCenter,
                        CAMERA_RADIUS + COLLISION_MARGIN,
                        box,
                        boxCollision
                    );

                    if (!boxCollision)
                        continue;

                    eyePosition += correction;
                    sphereCenter += correction;
                    corrected = true;
                }
            }

            if (groundPlaneEnabled_)
            {
                const float minimumEyeY = groundPlaneY_ + EYE_TO_FEET;
                if (eyePosition.y < minimumEyeY)
                {
                    eyePosition.y = minimumEyeY;
                    corrected = true;
                }
            }

            if (!corrected)
                break;
        }

        return eyePosition;
    }
};

#endif
