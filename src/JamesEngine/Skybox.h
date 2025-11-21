#pragma once

#include "SkyboxTexture.h"
#include "Renderer/Mesh.h"
#include "Renderer/Shader.h"
#include "Renderer/RenderTexture.h"

namespace JamesEngine
{

	class Core;

	class Skybox
	{
	public:
		Skybox(std::shared_ptr<Core> _core);

		void RenderSkybox();

		void SetTexture(std::shared_ptr<SkyboxTexture> _texture);
		std::shared_ptr<SkyboxTexture> GetTexture() { return mTexture; }

		void SetEnvironmentIntensity(float _intensity);
		float GetEnvironmentIntensity() { return mEnvironmentIntensity; }

		std::shared_ptr<Renderer::RenderTexture> GetBRDFLUT() { return mBRDFLUT; }

	private:
		std::shared_ptr<SkyboxTexture> mTexture;
		std::shared_ptr<Renderer::Mesh> mMesh = std::make_shared<Renderer::Mesh>("skybox");
		std::shared_ptr<Renderer::Shader> mShader = std::make_shared<Renderer::Shader>("../assets/shaders/SkyboxShader.vert", "../assets/shaders/SkyboxShader.frag");

		std::shared_ptr<Renderer::Mesh> mRect = std::make_shared<Renderer::Mesh>();

		std::shared_ptr<Renderer::RenderTexture> mBRDFLUT = std::make_shared<Renderer::RenderTexture>(512, 512, Renderer::RenderTextureType::BRDF_LUT);
		std::shared_ptr<Renderer::Shader> mBRDFLUTShader = std::make_shared<Renderer::Shader>("../assets/shaders/GenerateBRDFLUT.vert", "../assets/shaders/GenerateBRDFLUT.frag");
		

		std::shared_ptr<Renderer::RenderTexture> mIrradianceMap;
		std::shared_ptr<Renderer::Shader> mIrradianceMapShader = std::make_shared<Renderer::Shader>("../assets/shaders/GenerateIrradianceMap.vert", "../assets/shaders/GenerateIrradianceMap.frag");

		std::shared_ptr<Renderer::RenderTexture> mPrefilteredEnvMap;
		std::shared_ptr<Renderer::Shader> mPrefilteredEnvMapShader = std::make_shared<Renderer::Shader>("../assets/shaders/GeneratePrefilteredEnvMap.vert", "../assets/shaders/GeneratePrefilteredEnvMap.frag");

		float mEnvironmentIntensity = 1.f;

		std::weak_ptr<Core> mCore;
	};

}