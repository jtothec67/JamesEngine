#include "ModelCollider.h"

#include "Core.h"
#include "SphereCollider.h"
#include "BoxCollider.h"

#include "MathsHelper.h"

#include <iostream>
#include <algorithm>

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

#ifdef _DEBUG
    void ModelCollider::OnGUI()
    {
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

    bool ModelCollider::IsColliding(std::shared_ptr<Collider> _other, glm::vec3& _collisionPoint)
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

		// We are model, other is sphere
        std::shared_ptr<SphereCollider> otherSphere = std::dynamic_pointer_cast<SphereCollider>(_other);
		if (otherSphere)
		{
            // Get sphere world position and radius.
            glm::vec3 spherePos = otherSphere->GetPosition() + otherSphere->GetPositionOffset();
            float sphereRadius = otherSphere->GetRadius();
            float sphereRadiusSq = sphereRadius * sphereRadius;

            // Build the model's world transformation matrix.
            glm::vec3 modelPos = GetPosition() + GetPositionOffset();
            glm::vec3 modelScale = GetScale();
            glm::vec3 modelRotation = GetRotation() + GetRotationOffset();

            glm::mat4 modelMatrix = glm::mat4(1.0f);
            modelMatrix = glm::translate(modelMatrix, modelPos);
            modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotation.x), glm::vec3(1, 0, 0));
            modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotation.y), glm::vec3(0, 1, 0));
            modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotation.z), glm::vec3(0, 0, 1));
            modelMatrix = glm::scale(modelMatrix, modelScale);

            // Test returned triangle faces of the model's BVH against the sphere.
            std::vector<Renderer::Model::Face> faces = GetTriangles(spherePos, glm::vec3(0), glm::vec3(sphereRadius * 2));
            for (const auto& face : faces)
            {
                // Transform each vertex into world space.
                glm::vec3 a = glm::vec3(modelMatrix * glm::vec4(face.a.position, 1.0f));
                glm::vec3 b = glm::vec3(modelMatrix * glm::vec4(face.b.position, 1.0f));
                glm::vec3 c = glm::vec3(modelMatrix * glm::vec4(face.c.position, 1.0f));

                // Compute the closest point on this triangle to the sphere center.
                glm::vec3 closestPoint = Maths::ClosestPointOnTriangle(spherePos, a, b, c);

                // Check if the distance from the sphere's center to this point is within the radius.
                glm::vec3 diff = spherePos - closestPoint;
                float distanceSq = glm::dot(diff, diff);
                if (distanceSq <= sphereRadiusSq)
                {
                    _collisionPoint = closestPoint;
                    return true;
                }
            }
		}

		// We are model, other is box
        std::shared_ptr<BoxCollider> otherBox = std::dynamic_pointer_cast<BoxCollider>(_other);
        if (otherBox)
		{
            // Get the box's world parameters.
            glm::vec3 boxPos = otherBox->GetPosition() + otherBox->GetPositionOffset();
            glm::vec3 boxRotation = otherBox->GetRotation() + otherBox->GetRotationOffset();
            glm::vec3 boxSize = otherBox->GetSize();
            glm::vec3 boxHalfSize = boxSize * 0.5f;

            // Build the box's rotation matrix (from Euler angles).
            glm::mat4 boxRotMatrix = glm::mat4(1.0f);
            boxRotMatrix = glm::rotate(boxRotMatrix, glm::radians(boxRotation.x), glm::vec3(1, 0, 0));
            boxRotMatrix = glm::rotate(boxRotMatrix, glm::radians(boxRotation.y), glm::vec3(0, 1, 0));
            boxRotMatrix = glm::rotate(boxRotMatrix, glm::radians(boxRotation.z), glm::vec3(0, 0, 1));
            // Inverse rotation (since the rotation matrix is orthonormal, the inverse is its transpose)
            glm::mat4 invBoxRotMatrix = glm::transpose(boxRotMatrix);

            // Build the model's world transformation matrix.
            glm::vec3 modelPos = GetPosition() + GetPositionOffset();
            glm::vec3 modelScale = GetScale();
            glm::vec3 modelRotation = GetRotation() + GetRotationOffset();
            glm::mat4 modelMatrix = glm::mat4(1.0f);
            modelMatrix = glm::translate(modelMatrix, modelPos);
            modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotation.x), glm::vec3(1, 0, 0));
            modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotation.y), glm::vec3(0, 1, 0));
            modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotation.z), glm::vec3(0, 0, 1));
            modelMatrix = glm::scale(modelMatrix, modelScale);

            // Test returned triangle faces of the model's BVH against the box.
			std::vector<Renderer::Model::Face> faces = GetTriangles(boxPos, boxRotation, boxSize);
            for (const auto& face : faces)
            {
                // Transform triangle vertices into world space.
                glm::vec3 a = glm::vec3(modelMatrix * glm::vec4(face.a.position, 1.0f));
                glm::vec3 b = glm::vec3(modelMatrix * glm::vec4(face.b.position, 1.0f));
                glm::vec3 c = glm::vec3(modelMatrix * glm::vec4(face.c.position, 1.0f));

                // Transform the vertices into the box's local space.
                glm::vec3 aLocal = glm::vec3(invBoxRotMatrix * glm::vec4(a - boxPos, 1.0f));
                glm::vec3 bLocal = glm::vec3(invBoxRotMatrix * glm::vec4(b - boxPos, 1.0f));
                glm::vec3 cLocal = glm::vec3(invBoxRotMatrix * glm::vec4(c - boxPos, 1.0f));
                glm::vec3 triVerts[3] = { aLocal, bLocal, cLocal };

                // Use the SAT-based triangle-box test.
                if (Maths::TriBoxOverlap(triVerts, boxHalfSize))
                {
                    // Compute an approximate collision point.
                    // Here we take the closest point on the triangle (in world space)
                    // to the box center.
                    glm::vec3 closestPoint = Maths::ClosestPointOnTriangle(boxPos, a, b, c);
                    _collisionPoint = closestPoint;
                    return true;
                }
            }
		}

		return false;
    }


    // --- BVH Building ---

    // Recursively builds a BVH node from a set of faces.
    std::unique_ptr<ModelCollider::BVHNode> ModelCollider::BuildBVH(const std::vector<Renderer::Model::Face>& faces, unsigned int leafThreshold)
    {
        auto node = std::make_unique<BVHNode>();

        // Compute the AABB that contains all triangles in this node.
        glm::vec3 aabbMin(FLT_MAX);
        glm::vec3 aabbMax(-FLT_MAX);

        for (const auto& face : faces)
        {
            // Use each vertex position from the face (in model local space)
            aabbMin = glm::min(aabbMin, face.a.position);
            aabbMin = glm::min(aabbMin, face.b.position);
            aabbMin = glm::min(aabbMin, face.c.position);

            aabbMax = glm::max(aabbMax, face.a.position);
            aabbMax = glm::max(aabbMax, face.b.position);
            aabbMax = glm::max(aabbMax, face.c.position);
        }
        node->aabbMin = aabbMin;
        node->aabbMax = aabbMax;

        // If the number of faces is small enough, make this a leaf.
        if (faces.size() <= leafThreshold)
        {
            node->triangles = faces;
            return node;
        }

        // Otherwise, choose the axis along which the AABB is widest.
        glm::vec3 extent = aabbMax - aabbMin;
        int axis = 0;
        if (extent.y > extent.x && extent.y > extent.z)
            axis = 1;
        else if (extent.z > extent.x && extent.z > extent.y)
            axis = 2;

        // Copy and sort the faces by the centroid along the chosen axis.
        std::vector<Renderer::Model::Face> sortedFaces = faces;
        std::sort(sortedFaces.begin(), sortedFaces.end(),
            [axis](const Renderer::Model::Face& f1, const Renderer::Model::Face& f2)
            {
                glm::vec3 centroid1 = (f1.a.position + f1.b.position + f1.c.position) / 3.0f;
                glm::vec3 centroid2 = (f2.a.position + f2.b.position + f2.c.position) / 3.0f;
                return centroid1[axis] < centroid2[axis];
            });

        size_t mid = sortedFaces.size() / 2;
        std::vector<Renderer::Model::Face> leftFaces(sortedFaces.begin(), sortedFaces.begin() + mid);
        std::vector<Renderer::Model::Face> rightFaces(sortedFaces.begin() + mid, sortedFaces.end());

        // Recursively build child nodes.
        node->left = BuildBVH(leftFaces, leafThreshold);
        node->right = BuildBVH(rightFaces, leafThreshold);

        return node;
    }

    
    // --- BVH Query ---
    // Recursively traverses the BVH and adds any triangles in nodes whose AABB
    // overlaps the query AABB.
    void ModelCollider::QueryBVH(const BVHNode* node, const glm::vec3& queryMin, const glm::vec3& queryMax, std::vector<Renderer::Model::Face>& outTriangles)
    {
        if (!node)
            return;

        // Check for overlap between node's AABB and the query AABB.
        if (node->aabbMax.x < queryMin.x || node->aabbMin.x > queryMax.x ||
            node->aabbMax.y < queryMin.y || node->aabbMin.y > queryMax.y ||
            node->aabbMax.z < queryMin.z || node->aabbMin.z > queryMax.z)
        {
            return; // No overlap.
        }

        // If this is a leaf node, add all its triangles.
        if (!node->left && !node->right)
        {
            outTriangles.insert(outTriangles.end(), node->triangles.begin(), node->triangles.end());
            return;
        }

        // Otherwise, query both children.
        if (node->left)
            QueryBVH(node->left.get(), queryMin, queryMax, outTriangles);
        if (node->right)
            QueryBVH(node->right.get(), queryMin, queryMax, outTriangles);
    }

    // --- GetTriangles using BVH ---

    std::vector<Renderer::Model::Face> ModelCollider::GetTriangles(const glm::vec3& boxPos, const glm::vec3& boxRotation, const glm::vec3& boxSize, unsigned int _leafThreshold)
    {
        std::vector<Renderer::Model::Face> result;
        if (mModel == nullptr)
            return result;

        // (Re)build the BVH if it hasn’t been built yet or if the leaf threshold has changed.
        if (!mBVHRoot || mBVHLeafThreshold != _leafThreshold)
        {
            mBVHLeafThreshold = _leafThreshold;
            const std::vector<Renderer::Model::Face>& faces = mModel->mModel->GetFaces();
            mBVHRoot = BuildBVH(faces, mBVHLeafThreshold);
        }

        // Compute the world transformation for this model.
        glm::vec3 modelPos = GetPosition() + GetPositionOffset();
        glm::vec3 modelScale = GetScale();
        glm::vec3 modelRotation = GetRotation() + GetRotationOffset();

        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, modelPos);
        modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotation.x), glm::vec3(1, 0, 0));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotation.y), glm::vec3(0, 1, 0));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotation.z), glm::vec3(0, 0, 1));
        modelMatrix = glm::scale(modelMatrix, modelScale);

        // Transform the box parameters (which are defined in world space) into the model's local space.
        // First, compute the inverse model matrix.
        glm::mat4 invModelMatrix = glm::inverse(modelMatrix);

        // Compute the eight corners of the box in world space.
        glm::vec3 halfSize = boxSize * 0.5f;
        std::vector<glm::vec3> corners;
        corners.reserve(8);
        for (int x = -1; x <= 1; x += 2)
        {
            for (int y = -1; y <= 1; y += 2)
            {
                for (int z = -1; z <= 1; z += 2)
                {
                    corners.push_back(glm::vec3(x * halfSize.x, y * halfSize.y, z * halfSize.z));
                }
            }
        }

        // Build the box's rotation matrix (from its Euler angles).
        glm::mat4 boxRotMatrix = glm::mat4(1.0f);
        boxRotMatrix = glm::rotate(boxRotMatrix, glm::radians(boxRotation.x), glm::vec3(1, 0, 0));
        boxRotMatrix = glm::rotate(boxRotMatrix, glm::radians(boxRotation.y), glm::vec3(0, 1, 0));
        boxRotMatrix = glm::rotate(boxRotMatrix, glm::radians(boxRotation.z), glm::vec3(0, 0, 1));

        // Transform each corner: first rotate, then translate, then bring into model space.
        for (auto& corner : corners)
        {
            // Apply the box’s rotation and translation.
            corner = glm::vec3(boxRotMatrix * glm::vec4(corner, 1.0f)) + boxPos;
            // Transform from world space into model local space.
            corner = glm::vec3(invModelMatrix * glm::vec4(corner, 1.0f));
        }

        // Compute an axis–aligned bounding box (AABB) from the transformed corners.
        glm::vec3 queryMin = corners[0];
        glm::vec3 queryMax = corners[0];
        for (size_t i = 1; i < corners.size(); ++i)
        {
            queryMin = glm::min(queryMin, corners[i]);
            queryMax = glm::max(queryMax, corners[i]);
        }

        // Query the BVH for triangles that might intersect the box.
        QueryBVH(mBVHRoot.get(), queryMin, queryMax, result);

        return result;
    }
}