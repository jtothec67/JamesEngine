#include "ModelCollider.h"

#include "Core.h"
#include "SphereCollider.h"
#include "BoxCollider.h"
#include "Transform.h"

#include "MathsHelper.h"

#include <iostream>
#include <algorithm>
#include <iomanip>

#ifdef _DEBUG
#include "Camera.h"
#include "Entity.h"

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#endif

#include <fcl/fcl.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>

namespace JamesEngine
{

#ifdef _DEBUG
    void ModelCollider::OnGUI()
    {
		if (!mDebugVisual)
			return;

        if (mModel == nullptr)
            return;

        std::shared_ptr<Camera> camera = GetEntity()->GetCore()->GetCamera();

        mShader->uniform("projection", camera->GetProjectionMatrix());

        mShader->uniform("view", camera->GetViewMatrix());

        glm::mat4 mModelMatrix = glm::mat4(1.f);
        mModelMatrix = glm::translate(mModelMatrix, GetPosition() + mPositionOffset);
        glm::vec3 rotation = GetRotation() + GetRotationOffset();
        mModelMatrix = glm::rotate(mModelMatrix, glm::radians(rotation.x), glm::vec3(1, 0, 0));
        mModelMatrix = glm::rotate(mModelMatrix, glm::radians(rotation.y), glm::vec3(0, 1, 0));
        mModelMatrix = glm::rotate(mModelMatrix, glm::radians(rotation.z), glm::vec3(0, 0, 1));
        mModelMatrix = glm::scale(mModelMatrix, glm::vec3(GetScale().x, GetScale().y, GetScale().z));

        mShader->uniform("model", mModelMatrix);

        mShader->uniform("outlineWidth", 1.f);

        mShader->uniform("outlineColor", glm::vec3(0, 1, 0));

        glDisable(GL_DEPTH_TEST);

        mShader->drawOutline(mModel->mModel.get());

        glEnable(GL_DEPTH_TEST);
    }
#endif

    bool ModelCollider::IsColliding(std::shared_ptr<Collider> _other, glm::vec3& _collisionPoint, glm::vec3& _normal, float& _penetrationDepth)
    {
        if (_other == nullptr)
        {
            std::cout << "You should add a collider to an entity with a rigidbody" << std::endl;
            return false;
        }

		if (mModel == nullptr)
		{
			std::cout << "You need to add a model to the model collider" << std::endl;
			return false;
		}

        UpdateFCLTransform(GetPosition(), GetRotation());
        _other->UpdateFCLTransform(_other->GetPosition(), _other->GetRotation());

        // Get FCL collision objects
        fcl::CollisionObjectd* objA = GetFCLObject();
        fcl::CollisionObjectd* objB = _other->GetFCLObject();

        // Request contact information
        fcl::CollisionRequestd request;
        request.enable_contact = true;
        request.num_max_contacts = 1;

        fcl::CollisionResultd result;

        // Perform collision
        fcl::collide(objA, objB, request, result);

        if (result.isCollision())
        {
            // Extract contact info
            std::vector<fcl::Contactd> contacts;
            result.getContacts(contacts);

            if (!contacts.empty())
            {
                const fcl::Contactd& contact = contacts[0];

                _collisionPoint = glm::vec3(contact.pos[0], contact.pos[1], contact.pos[2]);
                _normal = glm::vec3(contact.normal[0], contact.normal[1], contact.normal[2]);
                _penetrationDepth = std::fabs(static_cast<float>(contact.penetration_depth));

                std::cout << "Position 1: " << GetPosition().x << ", " << GetPosition().y << ", " << GetPosition().z << std::endl;
                std::cout << "Rotation 1: " << GetRotation().x << ", " << GetRotation().y << ", " << GetRotation().z << std::endl;

                std::cout << "Position 2: " << _other->GetPosition().x << ", " << _other->GetPosition().y << ", " << _other->GetPosition().z << std::endl;
                std::cout << "Rotation 2: " << _other->GetRotation().x << ", " << _other->GetRotation().y << ", " << _other->GetRotation().z << std::endl;

                std::cout << "Collision point: " << _collisionPoint.x << ", " << _collisionPoint.y << ", " << _collisionPoint.z << std::endl;
                std::cout << "Normal: " << _normal.x << ", " << _normal.y << ", " << _normal.z << std::endl;
                std::cout << "Penetration depth: " << _penetrationDepth << std::endl;
                std::cout << std::endl;

                return true;
            }
        }
        else
        {
            return false;
        }
    }


    float TetrahedronVolume(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {
        return glm::dot(a, glm::cross(b, c)) / 6.0f;
    }

    glm::mat3 ModelCollider::UpdateInertiaTensor(float _mass)
    {
        // Accumulators for volume, center of mass, and inertia at the origin.
        float totalVolume = 0.0f;
        glm::vec3 totalCOM(0.0f);
        glm::mat3 I_origin(0.0f);

        std::vector<Renderer::Model::Face> faces = GetModel()->mModel->GetFaces();

        // Iterate over each triangle face and treat it as forming a tetrahedron with the origin.
        for (const auto& tri : faces)
        {
            glm::vec3 v0 = tri.a.position;
            glm::vec3 v1 = tri.b.position;
            glm::vec3 v2 = tri.c.position;

            // Compute the signed volume of the tetrahedron.
            float vol = TetrahedronVolume(v0, v1, v2);
            totalVolume += vol;

            // Tetrahedron center-of-mass (with vertices 0, v0, v1, v2)
            glm::vec3 tetCOM = (glm::vec3(0.0f) + v0 + v1 + v2) / 4.0f;
            totalCOM += vol * tetCOM;

            // Compute approximate inertia integrals for the tetrahedron relative to the origin.
            // Using an integration factor of (vol / 10) with sums over squared terms.
            float Ixx = vol / 10.0f * (
                v0.y * v0.y + v1.y * v1.y + v2.y * v2.y +
                v0.y * v1.y + v0.y * v2.y + v1.y * v2.y +
                v0.z * v0.z + v1.z * v1.z + v2.z * v2.z +
                v0.z * v1.z + v0.z * v2.z + v1.z * v2.z
                );
            float Iyy = vol / 10.0f * (
                v0.x * v0.x + v1.x * v1.x + v2.x * v2.x +
                v0.x * v1.x + v0.x * v2.x + v1.x * v2.x +
                v0.z * v0.z + v1.z * v1.z + v2.z * v2.z +
                v0.z * v1.z + v0.z * v2.z + v1.z * v2.z
                );
            float Izz = vol / 10.0f * (
                v0.x * v0.x + v1.x * v1.x + v2.x * v2.x +
                v0.x * v1.x + v0.x * v2.x + v1.x * v2.x +
                v0.y * v0.y + v1.y * v1.y + v2.y * v2.y +
                v0.y * v1.y + v0.y * v2.y + v1.y * v2.y
                );

            glm::mat3 I_tet(0.0f);
            I_tet[0][0] = Ixx;
            I_tet[1][1] = Iyy;
            I_tet[2][2] = Izz;

            // Sum the inertia contribution weighted by volume.
            I_origin += I_tet;
        }

        // Avoid division by zero if the volume is near zero.
        if (fabs(totalVolume) < 1e-6f)
            return glm::mat3(1.0f);

        glm::mat3 I_com = I_origin;

        // Scale the inertia tensor from "volume mass" to the actual mass.
        // (totalVolume here is proportional to mass if density is 1; scale by _mass / totalVolume)
        glm::mat3 inertiaTensor = I_com * (_mass / totalVolume);

        //return glm::mat3((2.0f / 5.0f) * _mass * mModel->mModel->get_length() * mModel->mModel->get_width());
        return inertiaTensor;
    }

    void ModelCollider::SetModel(std::shared_ptr<Model> _model)
    {
        mModel = _model;

        if (!mIsConvex)
        {
            // Get the faces from your model
            std::vector<Renderer::Model::Face> faces = mModel->mModel->GetFaces();

            // Create a new BVH model using OBBRSS for bounding volumes.
            std::shared_ptr<fcl::BVHModel<fcl::OBBRSSd>> bvhModel = std::make_shared<fcl::BVHModel<fcl::OBBRSSd>>();

            bvhModel->beginModel();

            // Loop over each face and add its triangle to the model
            for (const auto& face : faces)
            {
                // Each face holds vertices a, b, and c; each vertex has a glm::vec3 'position'
                fcl::Vector3d v0(face.a.position.x, face.a.position.y, face.a.position.z);
                fcl::Vector3d v1(face.b.position.x, face.b.position.y, face.b.position.z);
                fcl::Vector3d v2(face.c.position.x, face.c.position.y, face.c.position.z);

                // Add the triangle to the BVH model
                bvhModel->addTriangle(v0, v1, v2);
            }

            // Finalize the model so FCL builds the internal structure
            bvhModel->endModel();

            mNonConvexShape = bvhModel;
            InitFCLObject(mNonConvexShape);
        }
        else
        {
            // Retrieve all faces from the model.
            std::vector<Renderer::Model::Face> faces = mModel->mModel->GetFaces();

            // Step 1: Gather all unique vertices.
            const float tol = 0.0001f; // Tolerance for duplicate detection.
            std::vector<fcl::Vector3d> uniqueVertices;

            for (const auto& face : faces) {
                // Each face has vertices a, b, and c.
                glm::vec3 vertices[3] = { face.a.position, face.b.position, face.c.position };

                for (int i = 0; i < 3; ++i) {
                    bool found = false;
                    // Check if this vertex is already in our unique list.
                    for (const auto& uv : uniqueVertices) {
                        glm::vec3 uvglm(uv[0], uv[1], uv[2]);
                        if (glm::length2(vertices[i] - uvglm) < tol * tol) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        uniqueVertices.push_back(fcl::Vector3d(vertices[i].x, vertices[i].y, vertices[i].z));
                    }
                }
            }

            // Step 2: Build connectivity information.
            // The 'polygons' vector is a flat list where each face is represented as:
            // [num_vertices, index0, index1, index2]
            // We also record face count indirectly using polygonOffsets.
            std::vector<int> polygons;
            std::vector<int> polygonOffsets; // Not used by the FCL constructor but helps us count faces.

            auto findVertexIndex = [&](const glm::vec3& pos) -> int {
                for (size_t i = 0; i < uniqueVertices.size(); ++i) {
                    glm::vec3 uv(uniqueVertices[i][0], uniqueVertices[i][1], uniqueVertices[i][2]);
                    if (glm::length2(pos - uv) < tol * tol)
                        return static_cast<int>(i);
                }
                return -1; // Should never happen if the vertex exists.
                };

            for (const auto& face : faces) {
                // Record the starting offset for this face (for counting purposes)
                polygonOffsets.push_back(static_cast<int>(polygons.size()));
                // For a triangle, the first value is 3.
                polygons.push_back(3);
                // Then add the indices for vertices a, b, and c.
                polygons.push_back(findVertexIndex(face.a.position));
                polygons.push_back(findVertexIndex(face.b.position));
                polygons.push_back(findVertexIndex(face.c.position));
            }

            // Step 3: Wrap the unique vertices and polygons vectors into shared pointers.
            // FCL's Convexd constructor expects:
            //   const std::shared_ptr<const std::vector<fcl::Vector3d>>& vertices,
            //   int num_faces,
            //   const std::shared_ptr<const std::vector<int>>& faces,
            //   bool throw_if_valid
            auto vertices_ptr = std::make_shared<const std::vector<fcl::Vector3d>>(std::move(uniqueVertices));
            auto polygons_ptr = std::make_shared<const std::vector<int>>(std::move(polygons));

            // The number of faces is the size of polygonOffsets.
            int num_faces = static_cast<int>(polygonOffsets.size());

            // Step 4: Construct the convex collision geometry.
            mConvexShape = std::make_shared<fcl::Convexd>(vertices_ptr, num_faces, polygons_ptr, true);
            InitFCLObject(mConvexShape);

			std::cout << "Convex shape created" << std::endl;
        }
    }

}