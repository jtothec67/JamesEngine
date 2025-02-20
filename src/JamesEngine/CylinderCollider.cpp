#include "CylinderCollider.h"

#include "Core.h"
#include "SphereCollider.h"
#include "ModelCollider.h"
#include "CapsuleCollider.h"
#include "MathsHelper.h"

#include <iostream>

#ifdef _DEBUG
#include "Camera.h"
#include "Entity.h"

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#endif

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

namespace JamesEngine
{

    std::vector<glm::vec3> ClipPolygonAgainstPlane(const std::vector<glm::vec3>& poly, float planeZ, bool keepBelow)
    {
        std::vector<glm::vec3> output;
        int n = poly.size();
        for (int i = 0; i < n; i++)
        {
            const glm::vec3& current = poly[i];
            const glm::vec3& prev = poly[(i + n - 1) % n];
            bool currInside = (keepBelow ? (current.z <= planeZ) : (current.z >= planeZ));
            bool prevInside = (keepBelow ? (prev.z <= planeZ) : (prev.z >= planeZ));
            if (currInside)
            {
                if (!prevInside)
                {
                    // Edge entering: compute intersection t with plane z = planeZ.
                    float t = (planeZ - prev.z) / (current.z - prev.z);
                    glm::vec3 intersect = prev + t * (current - prev);
                    output.push_back(intersect);
                }
                output.push_back(current);
            }
            else if (prevInside)
            {
                // Edge exiting: compute intersection.
                float t = (planeZ - prev.z) / (current.z - prev.z);
                glm::vec3 intersect = prev + t * (current - prev);
                output.push_back(intersect);
            }
        }
        return output;
    }

    // Helper: Check whether a 2D point is inside a convex polygon (assumes vertices ordered counterclockwise).
    bool PointInPolygon(const std::vector<glm::vec2>& poly, const glm::vec2& point)
    {
        int n = poly.size();
        // For convex polygon, the origin must lie consistently to one side of each edge.
        for (int i = 0; i < n; i++)
        {
            glm::vec2 a = poly[i];
            glm::vec2 b = poly[(i + 1) % n];
            glm::vec2 edge = b - a;
            glm::vec2 toPoint = point - a;
            // Compute the z-component of the 2D cross product.
            float cross = edge.x * toPoint.y - edge.y * toPoint.x;
            if (cross < 0)
                return false;
        }
        return true;
    }

    // Helper: Squared distance from point p to segment ab.
    float SquaredDistancePointSegment(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b)
    {
        glm::vec2 ab = b - a;
        float t = glm::dot(p - a, ab) / glm::dot(ab, ab);
        t = glm::clamp(t, 0.0f, 1.0f);
        glm::vec2 proj = a + t * ab;
        glm::vec2 diff = p - proj;
        return glm::dot(diff, diff);
    }

    // Helper: Test whether a convex polygon (given by poly vertices in 2D) intersects a circle (disk)
    // centered at (0,0) with radius r.
    bool PolygonCircleIntersection(const std::vector<glm::vec2>& poly, float r)
    {
        float r2 = r * r;
        int n = poly.size();
        if (n == 0)
            return false;
        // If polygon is a point.
        if (n == 1)
            return glm::dot(poly[0], poly[0]) <= r2;
        // If polygon is a line segment.
        if (n == 2)
        {
            if (glm::dot(poly[0], poly[0]) <= r2 || glm::dot(poly[1], poly[1]) <= r2)
                return true;
            float d2 = SquaredDistancePointSegment(glm::vec2(0, 0), poly[0], poly[1]);
            return d2 <= r2;
        }
        // For polygon with three or more vertices:
        // (a) Check if the circle center (0,0) is inside the polygon.
        if (PointInPolygon(poly, glm::vec2(0, 0)))
            return true;
        // (b) Check each edge to see if the circle and edge intersect.
        for (int i = 0; i < n; i++)
        {
            glm::vec2 a = poly[i];
            glm::vec2 b = poly[(i + 1) % n];
            float d2 = SquaredDistancePointSegment(glm::vec2(0, 0), a, b);
            if (d2 <= r2)
                return true;
        }
        return false;
    }

#ifdef _DEBUG
    void CylinderCollider::OnGUI()
    {
        if (!mDebugVisual)
            return;

        std::shared_ptr<Camera> camera = GetEntity()->GetCore()->GetCamera();

        mShader->uniform("projection", camera->GetProjectionMatrix());

        mShader->uniform("view", camera->GetViewMatrix());

        glm::mat4 mModelMatrix = glm::mat4(1.f);
        mModelMatrix = glm::translate(mModelMatrix, GetPosition() + mPositionOffset);
        glm::vec3 rotation = GetRotation() + GetRotationOffset();
        mModelMatrix = glm::rotate(mModelMatrix, glm::radians(rotation.x), glm::vec3(1, 0, 0));
        mModelMatrix = glm::rotate(mModelMatrix, glm::radians(rotation.y), glm::vec3(0, 1, 0));
        mModelMatrix = glm::rotate(mModelMatrix, glm::radians(rotation.z + 90), glm::vec3(0, 0, 1));
        mModelMatrix = glm::scale(mModelMatrix, glm::vec3(mRadius, mHeight / 2, mRadius));

        mShader->uniform("model", mModelMatrix);

        mShader->uniform("outlineWidth", 1.f);

        mShader->uniform("outlineColor", glm::vec3(0, 1, 0));

        glDisable(GL_DEPTH_TEST);

        mShader->drawOutline(mModel.get());

        glEnable(GL_DEPTH_TEST);
    }
#endif

    inline glm::vec3 ClosestPointTriangleToOriginXZ(const glm::vec3& A,
        const glm::vec3& B,
        const glm::vec3& C)
    {
        // Project vertices onto xz–plane.
        glm::vec2 A2(A.x, A.z);
        glm::vec2 B2(B.x, B.z);
        glm::vec2 C2(C.x, C.z);
        glm::vec2 P(0.0f, 0.0f);

        // Compute barycentrics for P with respect to triangle A2, B2, C2.
        glm::vec2 v0 = B2 - A2;
        glm::vec2 v1 = C2 - A2;
        glm::vec2 v2 = P - A2;
        float d00 = glm::dot(v0, v0);
        float d01 = glm::dot(v0, v1);
        float d11 = glm::dot(v1, v1);
        float d20 = glm::dot(v2, v0);
        float d21 = glm::dot(v2, v1);
        float denom = d00 * d11 - d01 * d01;

        // If triangle is degenerate, fallback to A.
        if (fabs(denom) < 1e-6f)
            return A;

        float v = (d11 * d20 - d01 * d21) / denom;
        float w = (d00 * d21 - d01 * d20) / denom;
        float u = 1.0f - v - w;

        // If the point lies inside the triangle, P is the closest.
        if (u >= 0.0f && v >= 0.0f && w >= 0.0f)
        {
            return A * u + B * v + C * w;
        }

        // Otherwise, find the closest point on each edge and pick the best.
        auto ClosestPointOnEdge2D = [&](const glm::vec3& P3, const glm::vec3& Q3) -> glm::vec3 {
            glm::vec2 P3_2(P3.x, P3.z);
            glm::vec2 Q3_2(Q3.x, Q3.z);
            glm::vec2 dir = Q3_2 - P3_2;
            float t = 0.0f;
            if (glm::dot(dir, dir) > 1e-6f)
                t = glm::clamp(glm::dot(-P3_2, dir) / glm::dot(dir, dir), 0.0f, 1.0f);
            return P3 + t * (Q3 - P3);
            };

        glm::vec3 cpAB = ClosestPointOnEdge2D(A, B);
        glm::vec3 cpAC = ClosestPointOnEdge2D(A, C);
        glm::vec3 cpBC = ClosestPointOnEdge2D(B, C);

        auto sqrDistXZ = [&](const glm::vec3& pt) -> float {
            return pt.x * pt.x + pt.z * pt.z;
            };

        float dAB = sqrDistXZ(cpAB);
        float dAC = sqrDistXZ(cpAC);
        float dBC = sqrDistXZ(cpBC);

        glm::vec3 best = cpAB;
        float bestDist = dAB;
        if (dAC < bestDist) { bestDist = dAC; best = cpAC; }
        if (dBC < bestDist) { bestDist = dBC; best = cpBC; }

        return best;
    }

    bool CylinderCollider::IsColliding(std::shared_ptr<Collider> _other, glm::vec3& _collisionPoint)
    {
        if (_other == nullptr)
        {
            std::cout << "You should add a collider to an entity with a rigidbody" << std::endl;
            return false;
        }

        // We are cylinder, other is model
        std::shared_ptr<ModelCollider> otherModel = std::dynamic_pointer_cast<ModelCollider>(_other);
        if (otherModel)
        {
            // Get cylinder parameters.
        // Assume GetPosition(), GetRotation(), and mPositionOffset/mRotationOffset are defined.
            glm::vec3 cylinderCenter = GetPosition() + mPositionOffset;
            glm::vec3 cylinderRotation = GetRotation() + mRotationOffset; // in degrees.
            float cylinderRadius = GetRadius();
            float cylinderHeight = GetHeight();

            // Build the cylinder's rotation matrix.
            // We assume that the cylinder collider is defined so that its canonical form
            // has center at the origin and axis (0,0,1). Thus, this rotation rotates the canonical
            // cylinder into world space. To transform a world point into cylinder space, we subtract
            // the center and apply the inverse (transpose) of this rotation matrix.
            glm::mat4 cylRotationMatrix = glm::yawPitchRoll(glm::radians(cylinderRotation.y),
                glm::radians(cylinderRotation.x),
                glm::radians(cylinderRotation.z));
            glm::mat3 invCylRotationMatrix = glm::transpose(glm::mat3(cylRotationMatrix));

            // Define a bounding box that encloses the entire cylinder.
            glm::vec3 boxSize = glm::vec3(cylinderRadius * 2.0f, cylinderHeight, cylinderRadius * 2.0f);
            // Retrieve the triangles from the model collider (returned in model space).
            std::vector<Renderer::Model::Face> triangles = otherModel->GetTriangles(cylinderCenter, cylinderRotation, boxSize);

            // Get the model collider's world transform.
            glm::vec3 modelPosition = otherModel->GetPosition();
            glm::vec3 modelRotation = otherModel->GetRotation();
            glm::vec3 modelScale = otherModel->GetScale();
            glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), modelPosition)
                * glm::yawPitchRoll(glm::radians(modelRotation.y),
                    glm::radians(modelRotation.x),
                    glm::radians(modelRotation.z))
                * glm::scale(glm::mat4(1.0f), modelScale);

            // For each triangle from the model:
            for (const Renderer::Model::Face& tri : triangles)
            {
                // Convert triangle vertices from model space to world space.
                glm::vec3 v0_world = glm::vec3(modelMatrix * glm::vec4(tri.a.position, 1.0f));
                glm::vec3 v1_world = glm::vec3(modelMatrix * glm::vec4(tri.b.position, 1.0f));
                glm::vec3 v2_world = glm::vec3(modelMatrix * glm::vec4(tri.c.position, 1.0f));

                // Transform the vertices from world space into cylinder (canonical) space.
                // In cylinder space the cylinder is defined by:
                //   x^2 + y^2 <= r^2  and  |z| <= h/2.
                glm::vec3 v0 = invCylRotationMatrix * (v0_world - cylinderCenter);
                glm::vec3 v1 = invCylRotationMatrix * (v1_world - cylinderCenter);
                glm::vec3 v2 = invCylRotationMatrix * (v2_world - cylinderCenter);

                // Start with the triangle as a polygon.
                std::vector<glm::vec3> poly;
                poly.push_back(v0);
                poly.push_back(v1);
                poly.push_back(v2);

                // Clip the polygon against the slab z <= h/2 (upper plane).
                poly = ClipPolygonAgainstPlane(poly, cylinderHeight / 2.0f, true);
                // Clip the result against the slab z >= -h/2 (lower plane).
                poly = ClipPolygonAgainstPlane(poly, -cylinderHeight / 2.0f, false);

                // If the clipped polygon is empty, then this triangle does not intersect the cylinder’s height.
                if (poly.empty())
                    continue;

                // Project the clipped polygon onto the xy–plane.
                std::vector<glm::vec2> poly2d;
                for (const glm::vec3& p : poly)
                {
                    poly2d.push_back(glm::vec2(p.x, p.y));
                }

                // Test whether the 2D polygon (the projection of the clipped triangle) intersects the disk of radius r.
                if (PolygonCircleIntersection(poly2d, cylinderRadius))
                {
                    // Optionally, you might compute a collision point here.
                    return true;
                }
            }
        }

        return false;
    }

    float CylinderCollider::CylinderSDF(const glm::vec3& p, float h, float R)
    {
        float halfH = h * 0.5f;
        float radial = sqrt(p.x * p.x + p.z * p.z);
        float d_radial = radial - R;
        float d_vertical = fabs(p.y) - halfH;
        // If both distances are negative, the point is inside; otherwise add the outside part.
        float inside = std::min(std::max(d_radial, d_vertical), 0.0f);
        float outside = glm::length(glm::max(glm::vec2(d_radial, d_vertical), glm::vec2(0.0f)));
        return inside + outside;
    }


    

}