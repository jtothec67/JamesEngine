#include "MathsHelper.h"

namespace Maths
{
    glm::vec3 ClosestPointOnTriangle(const glm::vec3& p,
        const glm::vec3& a,
        const glm::vec3& b,
        const glm::vec3& c)
    {
        // Compute vectors from a to the other points and p.
        glm::vec3 ab = b - a;
        glm::vec3 ac = c - a;
        glm::vec3 ap = p - a;

        // Compute dot products
        float d1 = glm::dot(ab, ap);
        float d2 = glm::dot(ac, ap);
        // Check if p is in vertex region outside A.
        if (d1 <= 0.0f && d2 <= 0.0f)
            return a;

        // Check if p is in vertex region outside B.
        glm::vec3 bp = p - b;
        float d3 = glm::dot(ab, bp);
        float d4 = glm::dot(ac, bp);
        if (d3 >= 0.0f && d4 <= d3)
            return b;

        // Check if p is in edge region of AB, and if so, return projection onto AB.
        float vc = d1 * d4 - d3 * d2;
        if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
        {
            float v = d1 / (d1 - d3);
            return a + v * ab;
        }

        // Check if p is in vertex region outside C.
        glm::vec3 cp = p - c;
        float d5 = glm::dot(ab, cp);
        float d6 = glm::dot(ac, cp);
        if (d6 >= 0.0f && d5 <= d6)
            return c;

        // Check if p is in edge region of AC, and if so, return projection onto AC.
        float vb = d5 * d2 - d1 * d6;
        if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
        {
            float w = d2 / (d2 - d6);
            return a + w * ac;
        }

        // Check if p is in edge region of BC, and if so, return projection onto BC.
        float va = d3 * d6 - d5 * d4;
        if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
        {
            float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
            return b + w * (c - b);
        }

        // Otherwise, p is inside face region.
        float denom = 1.0f / (va + vb + vc);
        float v = vb * denom;
        float w = vc * denom;
        return a + ab * v + ac * w;
    }

    bool TriBoxOverlap(const glm::vec3 triVerts[3], const glm::vec3& boxHalfSize)
    {
        // The triangle vertices (in box local space: box center at origin)
        const glm::vec3& v0 = triVerts[0];
        const glm::vec3& v1 = triVerts[1];
        const glm::vec3& v2 = triVerts[2];

        // Compute the edge vectors of the triangle
        glm::vec3 e0 = v1 - v0;
        glm::vec3 e1 = v2 - v1;
        glm::vec3 e2 = v0 - v2;

        float fex = std::fabs(e0.x);
        float fey = std::fabs(e0.y);
        float fez = std::fabs(e0.z);
        {
            // Test axis L = cross(e0, (1,0,0)) --> (0, -e0.z, e0.y)
            float p0 = e0.z * v0.y - e0.y * v0.z;
            float p2 = e0.z * v2.y - e0.y * v2.z;
            float minVal = std::min(p0, p2);
            float maxVal = std::max(p0, p2);
            float rad = fey * boxHalfSize.z + fez * boxHalfSize.y;
            if (minVal > rad || maxVal < -rad)
                return false;
        }
        {
            // Test axis L = cross(e0, (0,1,0)) --> (e0.z, 0, -e0.x)
            float p0 = -e0.z * v0.x + e0.x * v0.z;
            float p2 = -e0.z * v2.x + e0.x * v2.z;
            float minVal = std::min(p0, p2);
            float maxVal = std::max(p0, p2);
            float rad = fex * boxHalfSize.z + fez * boxHalfSize.x;
            if (minVal > rad || maxVal < -rad)
                return false;
        }
        {
            // Test axis L = cross(e0, (0,0,1)) --> (-e0.y, e0.x, 0)
            float p0 = e0.y * v0.x - e0.x * v0.y;
            float p2 = e0.y * v2.x - e0.x * v2.y;
            float minVal = std::min(p0, p2);
            float maxVal = std::max(p0, p2);
            float rad = fex * boxHalfSize.y + fey * boxHalfSize.x;
            if (minVal > rad || maxVal < -rad)
                return false;
        }

        // Repeat tests for edge e1
        fex = std::fabs(e1.x); fey = std::fabs(e1.y); fez = std::fabs(e1.z);
        {
            float p0 = e1.z * v0.y - e1.y * v0.z;
            float p1 = e1.z * v1.y - e1.y * v1.z;
            float minVal = std::min(p0, p1);
            float maxVal = std::max(p0, p1);
            float rad = fey * boxHalfSize.z + fez * boxHalfSize.y;
            if (minVal > rad || maxVal < -rad)
                return false;
        }
        {
            float p0 = -e1.z * v0.x + e1.x * v0.z;
            float p1 = -e1.z * v1.x + e1.x * v1.z;
            float minVal = std::min(p0, p1);
            float maxVal = std::max(p0, p1);
            float rad = fex * boxHalfSize.z + fez * boxHalfSize.x;
            if (minVal > rad || maxVal < -rad)
                return false;
        }
        {
            float p0 = e1.y * v0.x - e1.x * v0.y;
            float p1 = e1.y * v1.x - e1.x * v1.y;
            float minVal = std::min(p0, p1);
            float maxVal = std::max(p0, p1);
            float rad = fex * boxHalfSize.y + fey * boxHalfSize.x;
            if (minVal > rad || maxVal < -rad)
                return false;
        }

        // Repeat tests for edge e2
        fex = std::fabs(e2.x); fey = std::fabs(e2.y); fez = std::fabs(e2.z);
        {
            float p0 = e2.z * v0.y - e2.y * v0.z;
            float p1 = e2.z * v1.y - e2.y * v1.z;
            float minVal = std::min(p0, p1);
            float maxVal = std::max(p0, p1);
            float rad = fey * boxHalfSize.z + fez * boxHalfSize.y;
            if (minVal > rad || maxVal < -rad)
                return false;
        }
        {
            float p0 = -e2.z * v0.x + e2.x * v0.z;
            float p1 = -e2.z * v1.x + e2.x * v1.z;
            float minVal = std::min(p0, p1);
            float maxVal = std::max(p0, p1);
            float rad = fex * boxHalfSize.z + fez * boxHalfSize.x;
            if (minVal > rad || maxVal < -rad)
                return false;
        }
        {
            float p0 = e2.y * v0.x - e2.x * v0.y;
            float p1 = e2.y * v1.x - e2.x * v1.y;
            float minVal = std::min(p0, p1);
            float maxVal = std::max(p0, p1);
            float rad = fex * boxHalfSize.y + fey * boxHalfSize.x;
            if (minVal > rad || maxVal < -rad)
                return false;
        }

        // Test overlap in the x, y, and z directions.
        {
            float minVal = std::min({ v0.x, v1.x, v2.x });
            float maxVal = std::max({ v0.x, v1.x, v2.x });
            if (minVal > boxHalfSize.x || maxVal < -boxHalfSize.x)
                return false;
        }
        {
            float minVal = std::min({ v0.y, v1.y, v2.y });
            float maxVal = std::max({ v0.y, v1.y, v2.y });
            if (minVal > boxHalfSize.y || maxVal < -boxHalfSize.y)
                return false;
        }
        {
            float minVal = std::min({ v0.z, v1.z, v2.z });
            float maxVal = std::max({ v0.z, v1.z, v2.z });
            if (minVal > boxHalfSize.z || maxVal < -boxHalfSize.z)
                return false;
        }

        // Finally, test if the plane of the triangle intersects the box.
        glm::vec3 normal = glm::cross(e0, e1);
        float d = -glm::dot(normal, v0);
        float r = boxHalfSize.x * std::fabs(normal.x) +
            boxHalfSize.y * std::fabs(normal.y) +
            boxHalfSize.z * std::fabs(normal.z);
        if (-r > d || d > r)
            return false;

        return true;
    }

    bool TrianglesIntersect(const glm::vec3& V0, const glm::vec3& V1, const glm::vec3& V2,
        const glm::vec3& U0, const glm::vec3& U1, const glm::vec3& U2)
    {
        const float EPSILON = 1e-6f;
        glm::vec3 E1, E2;
        glm::vec3 N1, N2;
        float d1, d2;
        float du0, du1, du2, dv0, dv1, dv2;

        // Compute plane equation of triangle(V0,V1,V2)
        E1 = V1 - V0;
        E2 = V2 - V0;
        N1 = glm::cross(E1, E2);
        d1 = -glm::dot(N1, V0);
        du0 = glm::dot(N1, U0) + d1;
        du1 = glm::dot(N1, U1) + d1;
        du2 = glm::dot(N1, U2) + d1;
        if (fabs(du0) < EPSILON) du0 = 0.0f;
        if (fabs(du1) < EPSILON) du1 = 0.0f;
        if (fabs(du2) < EPSILON) du2 = 0.0f;
        if (du0 * du1 > 0.0f && du0 * du2 > 0.0f)
            return false;

        // Compute plane equation of triangle(U0,U1,U2)
        E1 = U1 - U0;
        E2 = U2 - U0;
        N2 = glm::cross(E1, E2);
        d2 = -glm::dot(N2, U0);
        dv0 = glm::dot(N2, V0) + d2;
        dv1 = glm::dot(N2, V1) + d2;
        dv2 = glm::dot(N2, V2) + d2;
        if (fabs(dv0) < EPSILON) dv0 = 0.0f;
        if (fabs(dv1) < EPSILON) dv1 = 0.0f;
        if (fabs(dv2) < EPSILON) dv2 = 0.0f;
        if (dv0 * dv1 > 0.0f && dv0 * dv2 > 0.0f)
            return false;

        // Compute direction of intersection line
        glm::vec3 D = glm::cross(N1, N2);

        // To compute the intersection intervals on the line, we choose the largest component of D.
        float max = fabs(D.x);
        int index = 0;
        if (fabs(D.y) > max) { max = fabs(D.y); index = 1; }
        if (fabs(D.z) > max) { max = fabs(D.z); index = 2; }

        // Project all vertices onto the selected axis.
        auto proj = [index](const glm::vec3& v) -> float {
            if (index == 0) return v.x;
            else if (index == 1) return v.y;
            else return v.z;
            };

        float V0p = proj(V0);
        float V1p = proj(V1);
        float V2p = proj(V2);
        float U0p = proj(U0);
        float U1p = proj(U1);
        float U2p = proj(U2);

        float minV = std::min({ V0p, V1p, V2p });
        float maxV = std::max({ V0p, V1p, V2p });
        float minU = std::min({ U0p, U1p, U2p });
        float maxU = std::max({ U0p, U1p, U2p });

        // If the projection intervals do not overlap, then the triangles do not intersect.
        if (maxV < minU || maxU < minV)
            return false;

        // If we reach here, the triangles overlap on both planes and along the line of intersection.
        // (This does not catch all degenerate cases, but is robust enough for many applications.)
        return true;
    }
}