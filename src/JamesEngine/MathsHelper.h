#pragma once

#include <glm/glm.hpp>

namespace Maths
{
	// Given a point 'p' and a triangle defined by vertices a, b, and c, this function returns
	// the closest point on the triangle to p. (Based on algorithms described in "Real-Time
	// Collision Detection" by Christer Ericson.)
    glm::vec3 ClosestPointOnTriangle(const glm::vec3& p,
        const glm::vec3& a,
        const glm::vec3& b,
        const glm::vec3& c);

    // Given a triangle (with vertices already transformed into a coordinate system
    // in which the box is axis-aligned and centered at the origin) and the box half-sizes,
    // returns true if the triangle and box overlap.
    //
    // This implementation is based on the SAT method described in:
    // "A Fast Triangle-Box Overlap Test" by Tomas Akenine-Möller.
    bool TriBoxOverlap(const glm::vec3 triVerts[3], const glm::vec3& boxHalfSize);

    // Tests if two triangles (V0,V1,V2) and (U0,U1,U2) intersect.
    // This is a simplified version using plane–separation tests. In a robust
    // implementation you would use a complete triangle–triangle intersection
    // algorithm (e.g. "NoDivTriTriIsect" by Tomas Möller).
    bool TrianglesIntersect(const glm::vec3& V0, const glm::vec3& V1, const glm::vec3& V2,
        const glm::vec3& U0, const glm::vec3& U1, const glm::vec3& U2);
}