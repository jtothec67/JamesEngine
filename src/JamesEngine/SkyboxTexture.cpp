#include "SkyboxTexture.h"

#include <string>
#include <vector>

#include <filesystem>
namespace fs = std::filesystem;

namespace JamesEngine
{

	void SkyboxTexture::OnLoad()
	{
		std::string basePath = GetPath();

        // Prefer HDR if it exists
        std::string ext = ".png";

        if (fs::exists(basePath + "/px.hdr"))
        {
            ext = ".hdr";
        }

        std::vector<std::string> paths = {
            basePath + "/px" + ext,
            basePath + "/nx" + ext,
            basePath + "/py" + ext,
            basePath + "/ny" + ext,
            basePath + "/pz" + ext,
            basePath + "/nz" + ext
        };


		mTexture = std::make_shared<Renderer::Texture>(paths);
	}

}