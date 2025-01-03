#include "GUI.h"

#include "Core.h"
#include "Input.h"
#include "Texture.h"

namespace JamesEngine
{

	GUI::GUI(std::shared_ptr<Core> _core)
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
	}

    // Returns 0 if mouse not over button, 1 if mouse over button, 2 if mouse over button and clicked
    int GUI::Button(glm::vec2 _position, glm::vec2 _size, std::shared_ptr<Texture> _texture)
    {
        glm::mat4 model(1.0f);
        model = glm::translate(model, glm::vec3(_position.x, _position.y, 0));
        model = glm::scale(model, glm::vec3(_size.x, _size.y, 1));
        mShader->uniform("u_Model", model);

        int width = 800;
        int height = 600;

        glm::mat4 uiProjection = glm::ortho(0.0f, (float)width, 0.0f, (float)height, 0.0f, 1.0f);
        mShader->uniform("u_Projection", uiProjection);

        mShader->uniform("u_View", glm::mat4(1.0f));

        mShader->draw(mRect.get(), _texture->mTexture.get());

        glm::ivec2 mousePos = mCore.lock()->GetInput()->GetMouse()->GetPosition();

        // x,y mouse coordinates are from top left, so we need to flip the y
        mousePos.y = height - mousePos.y;

        // Mouse over button
        if (mousePos.x > _position.x && mousePos.x < _position.x + _size.x &&
			mousePos.y > _position.y && mousePos.y < _position.y + _size.y)
		{
            // Mouse clicked button
            if (mCore.lock()->GetInput()->GetMouse()->IsButtonDown(1))
			{
				return 2;
			}
            else
            {
                return 1;
            }
		}



		return 0;
	}

}