#include "MathsHelper.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

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

	//  ----------- TRIANGLE OVERLAP TEST FROM https://gamedev.stackexchange.com/questions/88060/triangle-triangle-intersection-code -----------

    /* some 3D macros */

#define CROSS(dest,v1,v2)                       \
dest[0]=v1[1]*v2[2]-v1[2]*v2[1]; \
dest[1]=v1[2]*v2[0]-v1[0]*v2[2]; \
dest[2]=v1[0]*v2[1]-v1[1]*v2[0];

#define DOT(v1,v2) (v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2])

#define SUB(dest,v1,v2) dest[0]=v1[0]-v2[0]; \
dest[1]=v1[1]-v2[1]; \
dest[2]=v1[2]-v2[2]; 

#define SCALAR(dest,alpha,v) dest[0] = alpha * v[0]; \
dest[1] = alpha * v[1]; \
dest[2] = alpha * v[2];

#define CHECK_MIN_MAX(p1,q1,r1,p2,q2,r2) {\
SUB(v1,p2,q1)\
SUB(v2,p1,q1)\
CROSS(N1,v1,v2)\
SUB(v1,q2,q1)\
if (DOT(v1,N1) > 0.0f) return 0;\
SUB(v1,p2,p1)\
SUB(v2,r1,p1)\
CROSS(N1,v1,v2)\
SUB(v1,r2,p1) \
if (DOT(v1,N1) > 0.0f) return 0;\
else return 1; }



/* Permutation in a canonical form of T2's vertices */

#define TRI_TRI_3D(p1,q1,r1,p2,q2,r2,dp2,dq2,dr2) { \
if (dp2 > 0.0f) { \
if (dq2 > 0.0f) CHECK_MIN_MAX(p1,r1,q1,r2,p2,q2) \
else if (dr2 > 0.0f) CHECK_MIN_MAX(p1,r1,q1,q2,r2,p2)\
else CHECK_MIN_MAX(p1,q1,r1,p2,q2,r2) }\
else if (dp2 < 0.0f) { \
if (dq2 < 0.0f) CHECK_MIN_MAX(p1,q1,r1,r2,p2,q2)\
else if (dr2 < 0.0f) CHECK_MIN_MAX(p1,q1,r1,q2,r2,p2)\
else CHECK_MIN_MAX(p1,r1,q1,p2,q2,r2)\
} else { \
if (dq2 < 0.0f) { \
if (dr2 >= 0.0f)  CHECK_MIN_MAX(p1,r1,q1,q2,r2,p2)\
else CHECK_MIN_MAX(p1,q1,r1,p2,q2,r2)\
} \
else if (dq2 > 0.0f) { \
if (dr2 > 0.0f) CHECK_MIN_MAX(p1,r1,q1,p2,q2,r2)\
else  CHECK_MIN_MAX(p1,q1,r1,q2,r2,p2)\
} \
else  { \
if (dr2 > 0.0f) CHECK_MIN_MAX(p1,q1,r1,r2,p2,q2)\
else if (dr2 < 0.0f) CHECK_MIN_MAX(p1,r1,q1,r2,p2,q2)\
else return coplanar_tri_tri3d(p1,q1,r1,p2,q2,r2,N1,N2);\
}}}



    /*
     *
     *  Three-dimensional Triangle-Triangle Overlap Test
     *
     */


    int tri_tri_overlap_test_3d(real p1[3], real q1[3], real r1[3],

        real p2[3], real q2[3], real r2[3])
    {
        real dp1, dq1, dr1, dp2, dq2, dr2;
        real v1[3], v2[3];
        real N1[3], N2[3];

        /* Compute distance signs  of p1, q1 and r1 to the plane of
         triangle(p2,q2,r2) */


        SUB(v1, p2, r2)
            SUB(v2, q2, r2)
            CROSS(N2, v1, v2)

            SUB(v1, p1, r2)
            dp1 = DOT(v1, N2);
        SUB(v1, q1, r2)
            dq1 = DOT(v1, N2);
        SUB(v1, r1, r2)
            dr1 = DOT(v1, N2);

        if (((dp1 * dq1) > 0.0f) && ((dp1 * dr1) > 0.0f))  return 0;

        /* Compute distance signs  of p2, q2 and r2 to the plane of
         triangle(p1,q1,r1) */


        SUB(v1, q1, p1)
            SUB(v2, r1, p1)
            CROSS(N1, v1, v2)

            SUB(v1, p2, r1)
            dp2 = DOT(v1, N1);
        SUB(v1, q2, r1)
            dq2 = DOT(v1, N1);
        SUB(v1, r2, r1)
            dr2 = DOT(v1, N1);

        if (((dp2 * dq2) > 0.0f) && ((dp2 * dr2) > 0.0f)) return 0;

        /* Permutation in a canonical form of T1's vertices */




        if (dp1 > 0.0f) {
            if (dq1 > 0.0f) TRI_TRI_3D(r1, p1, q1, p2, r2, q2, dp2, dr2, dq2)
            else if (dr1 > 0.0f) TRI_TRI_3D(q1, r1, p1, p2, r2, q2, dp2, dr2, dq2)
            else TRI_TRI_3D(p1, q1, r1, p2, q2, r2, dp2, dq2, dr2)
        }
        else if (dp1 < 0.0f) {
            if (dq1 < 0.0f) TRI_TRI_3D(r1, p1, q1, p2, q2, r2, dp2, dq2, dr2)
            else if (dr1 < 0.0f) TRI_TRI_3D(q1, r1, p1, p2, q2, r2, dp2, dq2, dr2)
            else TRI_TRI_3D(p1, q1, r1, p2, r2, q2, dp2, dr2, dq2)
        }
        else {
            if (dq1 < 0.0f) {
                if (dr1 >= 0.0f) TRI_TRI_3D(q1, r1, p1, p2, r2, q2, dp2, dr2, dq2)
                else TRI_TRI_3D(p1, q1, r1, p2, q2, r2, dp2, dq2, dr2)
            }
            else if (dq1 > 0.0f) {
                if (dr1 > 0.0f) TRI_TRI_3D(p1, q1, r1, p2, r2, q2, dp2, dr2, dq2)
                else TRI_TRI_3D(q1, r1, p1, p2, q2, r2, dp2, dq2, dr2)
            }
            else {
                if (dr1 > 0.0f) TRI_TRI_3D(r1, p1, q1, p2, q2, r2, dp2, dq2, dr2)
                else if (dr1 < 0.0f) TRI_TRI_3D(r1, p1, q1, p2, r2, q2, dp2, dr2, dq2)
                else return coplanar_tri_tri3d(p1, q1, r1, p2, q2, r2, N1, N2);
            }
        }
    };



    int coplanar_tri_tri3d(real p1[3], real q1[3], real r1[3],
        real p2[3], real q2[3], real r2[3],
        real normal_1[3], real normal_2[3]) {

        real P1[2], Q1[2], R1[2];
        real P2[2], Q2[2], R2[2];

        real n_x, n_y, n_z;

        n_x = ((normal_1[0] < 0) ? -normal_1[0] : normal_1[0]);
        n_y = ((normal_1[1] < 0) ? -normal_1[1] : normal_1[1]);
        n_z = ((normal_1[2] < 0) ? -normal_1[2] : normal_1[2]);


        /* Projection of the triangles in 3D onto 2D such that the area of
         the projection is maximized. */


        if ((n_x > n_z) && (n_x >= n_y)) {
            // Project onto plane YZ

            P1[0] = q1[2]; P1[1] = q1[1];
            Q1[0] = p1[2]; Q1[1] = p1[1];
            R1[0] = r1[2]; R1[1] = r1[1];

            P2[0] = q2[2]; P2[1] = q2[1];
            Q2[0] = p2[2]; Q2[1] = p2[1];
            R2[0] = r2[2]; R2[1] = r2[1];

        }
        else if ((n_y > n_z) && (n_y >= n_x)) {
            // Project onto plane XZ

            P1[0] = q1[0]; P1[1] = q1[2];
            Q1[0] = p1[0]; Q1[1] = p1[2];
            R1[0] = r1[0]; R1[1] = r1[2];

            P2[0] = q2[0]; P2[1] = q2[2];
            Q2[0] = p2[0]; Q2[1] = p2[2];
            R2[0] = r2[0]; R2[1] = r2[2];

        }
        else {
            // Project onto plane XY

            P1[0] = p1[0]; P1[1] = p1[1];
            Q1[0] = q1[0]; Q1[1] = q1[1];
            R1[0] = r1[0]; R1[1] = r1[1];

            P2[0] = p2[0]; P2[1] = p2[1];
            Q2[0] = q2[0]; Q2[1] = q2[1];
            R2[0] = r2[0]; R2[1] = r2[1];
        }

        return tri_tri_overlap_test_2d(P1, Q1, R1, P2, Q2, R2);

    };



    /*
        Three-dimensional Triangle-Triangle Intersection
     */

    /*
        This macro is called when the triangles surely intersect
        It constructs the segment of intersection of the two triangles
        if they are not coplanar.
     */

#define CONSTRUCT_INTERSECTION(p1,q1,r1,p2,q2,r2) { \
SUB(v1,q1,p1) \
SUB(v2,r2,p1) \
CROSS(N,v1,v2) \
SUB(v,p2,p1) \
if (DOT(v,N) > 0.0f) {\
SUB(v1,r1,p1) \
CROSS(N,v1,v2) \
if (DOT(v,N) <= 0.0f) { \
SUB(v2,q2,p1) \
CROSS(N,v1,v2) \
if (DOT(v,N) > 0.0f) { \
SUB(v1,p1,p2) \
SUB(v2,p1,r1) \
alpha = DOT(v1,N2) / DOT(v2,N2); \
SCALAR(v1,alpha,v2) \
SUB(source,p1,v1) \
SUB(v1,p2,p1) \
SUB(v2,p2,r2) \
alpha = DOT(v1,N1) / DOT(v2,N1); \
SCALAR(v1,alpha,v2) \
SUB(target,p2,v1) \
return 1; \
} else { \
SUB(v1,p2,p1) \
SUB(v2,p2,q2) \
alpha = DOT(v1,N1) / DOT(v2,N1); \
SCALAR(v1,alpha,v2) \
SUB(source,p2,v1) \
SUB(v1,p2,p1) \
SUB(v2,p2,r2) \
alpha = DOT(v1,N1) / DOT(v2,N1); \
SCALAR(v1,alpha,v2) \
SUB(target,p2,v1) \
return 1; \
} \
} else { \
return 0; \
} \
} else { \
SUB(v2,q2,p1) \
CROSS(N,v1,v2) \
if (DOT(v,N) < 0.0f) { \
return 0; \
} else { \
SUB(v1,r1,p1) \
CROSS(N,v1,v2) \
if (DOT(v,N) >= 0.0f) { \
SUB(v1,p1,p2) \
SUB(v2,p1,r1) \
alpha = DOT(v1,N2) / DOT(v2,N2); \
SCALAR(v1,alpha,v2) \
SUB(source,p1,v1) \
SUB(v1,p1,p2) \
SUB(v2,p1,q1) \
alpha = DOT(v1,N2) / DOT(v2,N2); \
SCALAR(v1,alpha,v2) \
SUB(target,p1,v1) \
return 1; \
} else { \
SUB(v1,p2,p1) \
SUB(v2,p2,q2) \
alpha = DOT(v1,N1) / DOT(v2,N1); \
SCALAR(v1,alpha,v2) \
SUB(source,p2,v1) \
SUB(v1,p1,p2) \
SUB(v2,p1,q1) \
alpha = DOT(v1,N2) / DOT(v2,N2); \
SCALAR(v1,alpha,v2) \
SUB(target,p1,v1) \
return 1; \
}}}} 


    /*
     *
     *  Two dimensional Triangle-Triangle Overlap Test
     *
     */


     /* some 2D macros */

#define ORIENT_2D(a, b, c)  ((a[0]-c[0])*(b[1]-c[1])-(a[1]-c[1])*(b[0]-c[0]))

#define INTERSECTION_TEST_VERTEX(P1, Q1, R1, P2, Q2, R2) {\
  if (ORIENT_2D(R2,P2,Q1) >= 0.0f)\
    if (ORIENT_2D(R2,Q2,Q1) <= 0.0f)\
      if (ORIENT_2D(P1,P2,Q1) > 0.0f) {\
        if (ORIENT_2D(P1,Q2,Q1) <= 0.0f) return 1; \
        else return 0;} else {\
        if (ORIENT_2D(P1,P2,R1) >= 0.0f)\
          if (ORIENT_2D(Q1,R1,P2) >= 0.0f) return 1; \
          else return 0;\
        else return 0;}\
    else \
      if (ORIENT_2D(P1,Q2,Q1) <= 0.0f)\
        if (ORIENT_2D(R2,Q2,R1) <= 0.0f)\
          if (ORIENT_2D(Q1,R1,Q2) >= 0.0f) return 1; \
          else return 0;\
        else return 0;\
      else return 0;\
  else\
    if (ORIENT_2D(R2,P2,R1) >= 0.0f) \
      if (ORIENT_2D(Q1,R1,R2) >= 0.0f)\
        if (ORIENT_2D(P1,P2,R1) >= 0.0f) return 1;\
        else return 0;\
      else \
        if (ORIENT_2D(Q1,R1,Q2) >= 0.0f) {\
          if (ORIENT_2D(R2,R1,Q2) >= 0.0f) return 1; \
          else return 0; }\
        else return 0; \
    else  return 0; \
 };


#define INTERSECTION_TEST_EDGE(P1, Q1, R1, P2, Q2, R2) { \
if (ORIENT_2D(R2,P2,Q1) >= 0.0f) {\
if (ORIENT_2D(P1,P2,Q1) >= 0.0f) { \
if (ORIENT_2D(P1,Q1,R2) >= 0.0f) return 1; \
else return 0;} else { \
if (ORIENT_2D(Q1,R1,P2) >= 0.0f){ \
if (ORIENT_2D(R1,P1,P2) >= 0.0f) return 1; else return 0;} \
else return 0; } \
} else {\
if (ORIENT_2D(R2,P2,R1) >= 0.0f) {\
if (ORIENT_2D(P1,P2,R1) >= 0.0f) {\
if (ORIENT_2D(P1,R1,R2) >= 0.0f) return 1;  \
else {\
if (ORIENT_2D(Q1,R1,R2) >= 0.0f) return 1; else return 0;}}\
else  return 0; }\
else return 0; }}



    int ccw_tri_tri_intersection_2d(real p1[2], real q1[2], real r1[2],
        real p2[2], real q2[2], real r2[2]) {
        if (ORIENT_2D(p2, q2, p1) >= 0.0f) {
            if (ORIENT_2D(q2, r2, p1) >= 0.0f) {
                if (ORIENT_2D(r2, p2, p1) >= 0.0f) return 1;
                else INTERSECTION_TEST_EDGE(p1, q1, r1, p2, q2, r2)
            }
            else {
                if (ORIENT_2D(r2, p2, p1) >= 0.0f)
                    INTERSECTION_TEST_EDGE(p1, q1, r1, r2, p2, q2)
                else INTERSECTION_TEST_VERTEX(p1, q1, r1, p2, q2, r2)
            }
        }
        else {
            if (ORIENT_2D(q2, r2, p1) >= 0.0f) {
                if (ORIENT_2D(r2, p2, p1) >= 0.0f)
                    INTERSECTION_TEST_EDGE(p1, q1, r1, q2, r2, p2)
                else  INTERSECTION_TEST_VERTEX(p1, q1, r1, q2, r2, p2)
            }
            else INTERSECTION_TEST_VERTEX(p1, q1, r1, r2, p2, q2)
        }
    };


    int tri_tri_overlap_test_2d(real p1[2], real q1[2], real r1[2],
        real p2[2], real q2[2], real r2[2]) {
        if (ORIENT_2D(p1, q1, r1) < 0.0f)
            if (ORIENT_2D(p2, q2, r2) < 0.0f)
                return ccw_tri_tri_intersection_2d(p1, r1, q1, p2, r2, q2);
            else
                return ccw_tri_tri_intersection_2d(p1, r1, q1, p2, q2, r2);
        else
            if (ORIENT_2D(p2, q2, r2) < 0.0f)
                return ccw_tri_tri_intersection_2d(p1, q1, r1, p2, r2, q2);
            else
                return ccw_tri_tri_intersection_2d(p1, q1, r1, p2, q2, r2);

    };
}