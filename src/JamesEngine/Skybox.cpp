#include "Skybox.h"

#include "Core.h"
#include "Camera.h"
#include "Resources.h"

#include <glm/glm.hpp>

namespace JamesEngine
{

	Skybox::Skybox(std::shared_ptr<Core> _core)
	{
		mCore = _core;

		Renderer::Face face;
		face.a.m_position = glm::vec3(1.0f, 0.0f, 0.0f);
		face.b.m_position = glm::vec3(0.0f, 1.0f, 0.0f);
		face.c.m_position = glm::vec3(0.0f, 0.0f, 0.0f);
		face.a.m_texcoords = glm::vec2(1.0f, 0.0f);
		face.b.m_texcoords = glm::vec2(0.0f, 1.0f);
		face.c.m_texcoords = glm::vec2(0.0f, 0.0f);
		mRect->add(face);

		Renderer::Face face2;
		face2.a.m_position = glm::vec3(1.0f, 0.0f, 0.0f);
		face2.b.m_position = glm::vec3(1.0f, 1.0f, 0.0f);
		face2.c.m_position = glm::vec3(0.0f, 1.0f, 0.0f);
		face2.a.m_texcoords = glm::vec2(1.0f, 0.0f);
		face2.b.m_texcoords = glm::vec2(1.0f, 1.0f);
		face2.c.m_texcoords = glm::vec2(0.0f, 1.0f);
		mRect->add(face2);

		mBRDFLUT->clear();
		mBRDFLUT->bind();

		glViewport(0, 0, mBRDFLUT->getWidth(), mBRDFLUT->getHeight());
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);

		mBRDFLUTShader->use();

		mBRDFLUTShader->draw(mRect.get());

		mBRDFLUTShader->unuse();

		mBRDFLUT->unbind();

		auto objShader = mCore.lock()->GetResources()->Load<Shader>("shaders/ObjShader");

		objShader->mShader->use();
		objShader->mShader->uniform("u_BRDFLUT", mBRDFLUT, 28);
		objShader->mShader->unuse();
	}

	void Skybox::SetTexture(std::shared_ptr<SkyboxTexture> _texture)
	{
		mTexture = _texture;

		mIrradianceMap = std::make_shared<Renderer::RenderTexture>(64, 64, Renderer::RenderTextureType::IrradianceCubeMap);
		mIrradianceMap->clear();

		// Per face basis (columns: S, T, R)
		static const glm::vec3 S[6] = {
			{ 0, 0,-1}, { 0, 0, 1}, { 1, 0, 0}, { 1, 0, 0}, { 1, 0, 0}, {-1, 0, 0}
		};
		static const glm::vec3 T[6] = {
			{ 0,-1, 0}, { 0,-1, 0}, { 0, 0, 1}, { 0, 0,-1}, { 0,-1, 0}, { 0,-1, 0}
		};
		static const glm::vec3 R[6] = {
			{ 1, 0, 0}, {-1, 0, 0}, { 0, 1, 0}, { 0,-1, 0}, { 0, 0, 1}, { 0, 0,-1}
		};

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);

		mIrradianceMapShader->use();

		mIrradianceMapShader->cubemapUniform("u_SkyBox", mTexture->mTexture, 0);
		
		// Bake irradiance (no mips)
		for (int face = 0; face < 6; ++face)
		{
			mIrradianceMap->bindFace(face, 0);
			
			glm::mat3 faceBasis(S[face], T[face], R[face]);
			mIrradianceMapShader->uniform("u_FaceBasis", faceBasis);

			mIrradianceMapShader->draw(mRect.get());
		}
		mIrradianceMapShader->unuse();
		mIrradianceMap->unbind();


		mPrefilteredEnvMap = std::make_shared<Renderer::RenderTexture>(256, 256, Renderer::RenderTextureType::PrefilteredEnvCubeMap);
		mPrefilteredEnvMap->clear();

		mPrefilteredEnvMapShader->use();

		mPrefilteredEnvMapShader->cubemapUniform("u_SkyBox", mTexture->mTexture, 0);
		// Bind environment map
		int numMips = 1 + (int)std::floor(std::log2(mPrefilteredEnvMap->getWidth()));
		mPrefilteredEnvMapShader->uniform("u_SkyboxMaxLOD", numMips);

		for (int level = 0; level < numMips; ++level)
		{
			float roughness = (numMips > 1) ? (float)level / float(numMips - 1) : 0.0f;

			for (int face = 0; face < 6; ++face)
			{
				mPrefilteredEnvMap->bindFace(face, level); // sets COLOR_ATTACHMENT0 + viewport for this mip

				glm::mat3 faceBasis(S[face], T[face], R[face]);
				mPrefilteredEnvMapShader->uniform("u_FaceBasis", faceBasis);
				mPrefilteredEnvMapShader->uniform("u_Roughness", roughness);

				mPrefilteredEnvMapShader->draw(mRect.get());
			}
		}
		mPrefilteredEnvMapShader->unuse();
		mPrefilteredEnvMap->unbind();

		mCore.lock()->GetWindow()->ResetGLModes();

		auto objShader = mCore.lock()->GetResources()->Load<Shader>("shaders/ObjShader");

		objShader->mShader->use();
		objShader->mShader->cubemapUniform("u_IrradianceCube", mIrradianceMap, 30);
		objShader->mShader->cubemapUniform("u_PrefilterEnv", mPrefilteredEnvMap, 31);
		objShader->mShader->uniform("u_PrefilterMaxLOD", (float)(numMips - 1));
		objShader->mShader->unuse();
	}

	void Skybox::RenderSkybox()
	{
		if (!mTexture)
			return;
		std::shared_ptr<Camera> camera = mCore.lock()->GetCamera();
		glm::mat4 projection = camera->GetProjectionMatrix();
		glm::mat4 view = camera->GetViewMatrix();
		glm::mat4 invPV = glm::inverse(projection * view);

		mShader->use();

		mShader->uniform("u_InvPV", invPV);

		mShader->drawSkybox(mMesh.get(),mTexture->mTexture.get());

		mShader->unuse();

		mCore.lock()->GetWindow()->ResetGLModes();
	}
}