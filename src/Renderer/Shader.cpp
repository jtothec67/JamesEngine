#include "Shader.h"

#include <glm/ext.hpp>

#include <iostream>
#include <exception>

namespace Renderer
{

	Shader::Shader(const std::string& _vertpath, const std::string& _fragpath)
	{
		m_vertpath = _vertpath;
		m_fragpath = _fragpath;

		std::ifstream vfile(_vertpath);

		if (!vfile.is_open())
		{
			std::cout << "Couldn't open vertex shader: " << _vertpath << std::endl;
			throw std::exception();
		}

		std::string vline;

		while (!vfile.eof())
		{
			std::getline(vfile, vline);
			vline += "\n";
			m_vertsrc += vline;
		}


		std::ifstream lfile(_fragpath);

		if (!lfile.is_open())
		{
			std::cout << "Couldn't open fragment shader: " << _fragpath << std::endl;
			throw std::exception();
		}

		std::string lline;

		while (!lfile.eof())
		{
			std::getline(lfile, lline);
			lline += "\n";
			m_fragsrc += lline;
		}
	}

	GLuint Shader::id()
	{
		if (m_dirty)
		{
			GLuint v_id = glCreateShader(GL_VERTEX_SHADER);
			const GLchar* GLvertsrc = m_vertsrc.c_str();
			glShaderSource(v_id, 1, &GLvertsrc, NULL);
			glCompileShader(v_id);
			GLint success = 0;
			glGetShaderiv(v_id, GL_COMPILE_STATUS, &success);

			if (!success)
			{
				std::cout << "Vertex shader failed to compile: " << m_vertpath << std::endl;
				throw std::exception();
			}


			GLuint f_id = glCreateShader(GL_FRAGMENT_SHADER);
			const GLchar* GLfragsrc = m_fragsrc.c_str();
			glShaderSource(f_id, 1, &GLfragsrc, NULL);
			glCompileShader(f_id);
			glGetShaderiv(f_id, GL_COMPILE_STATUS, &success);

			if (!success)
			{
				std::cout << "Fragment shader failed to compile: " << m_fragpath << std::endl;
				throw std::exception();
			}


			m_id = glCreateProgram();

			glAttachShader(m_id, v_id);
			glAttachShader(m_id, f_id);

			glLinkProgram(m_id);
			glGetProgramiv(m_id, GL_LINK_STATUS, &success);

			if (!success)
			{
				std::cout << "Shader id failed to be created: " << std::endl << m_vertpath << std::endl << m_fragpath << std::endl;
				throw std::exception();
			}

			glDetachShader(m_id, v_id);
			glDeleteShader(v_id);
			glDetachShader(m_id, f_id);
			glDeleteShader(f_id);

			m_dirty = false;
		}

		return m_id;
	}

	void Shader::uniform(const std::string& _name, bool value)
	{
		// Find uniform locations
		GLint loc = glGetUniformLocation(id(), _name.c_str());

		glUseProgram(id());
		glUniform1i(loc, value);
		glUseProgram(0);
	}

	void Shader::uniform(const std::string& _name, int value)
	{
		// Find uniform locations
		GLint loc = glGetUniformLocation(id(), _name.c_str());

		glUseProgram(id());
		glUniform1i(loc, value);
		glUseProgram(0);
	}

	void Shader::uniform(const std::string& _name, float value)
	{
		// Find uniform locations
		GLint loc = glGetUniformLocation(id(), _name.c_str());

		glUseProgram(id());
		glUniform1f(loc, value);
		glUseProgram(0);
	}

	void Shader::uniform(const std::string& _name, const glm::mat4& value)
	{
		// Find uniform locations
		GLint loc = glGetUniformLocation(id(), _name.c_str());

		glUseProgram(id());
		glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(value));
		glUseProgram(0);
	}

	void Shader::uniform(const std::string& _name, const glm::vec3& value)
	{
		// Find uniform locations
		GLint loc = glGetUniformLocation(id(), _name.c_str());

		glUseProgram(id());
		glUniform3fv(loc, 1, glm::value_ptr(value));
		glUseProgram(0);
	}

	void Shader::uniform(const std::string& _name, const glm::vec4& value)
	{
		// Find uniform locations
		GLint loc = glGetUniformLocation(id(), _name.c_str());

		glUseProgram(id());
		glUniform4fv(loc, 1, glm::value_ptr(value));
		glUseProgram(0);
	}

	void Shader::uniform(const std::string& _name, std::vector<int> value)
	{
		// Find uniform locations
		GLint loc = glGetUniformLocation(id(), _name.c_str());
		glUseProgram(id());
		glUniform1iv(loc, value.size(), value.data());
		glUseProgram(0);
	}

	void Shader::uniform(const std::string& _name, std::vector<float> value)
	{
		// Find uniform locations
		GLint loc = glGetUniformLocation(id(), _name.c_str());
		glUseProgram(id());
		glUniform1fv(loc, value.size(), value.data());
		glUseProgram(0);
	}

	void Shader::uniform(const std::string& _name, const std::vector<glm::vec2>& value)
	{
		// Find uniform locations
		GLint loc = glGetUniformLocation(id(), _name.c_str());
		glUseProgram(id());
		glUniform2fv(loc, value.size(), glm::value_ptr(value[0]));
		glUseProgram(0);
	}

	void Shader::uniform(const std::string& _name, const std::vector<glm::vec3>& value)
	{
		// Find uniform locations
		GLint loc = glGetUniformLocation(id(), _name.c_str());
		glUseProgram(id());
		glUniform3fv(loc, value.size(), glm::value_ptr(value[0]));
		glUseProgram(0);
	}

	void Shader::uniform(const std::string& _name, const std::vector<glm::mat4>& values)
	{
		GLint loc = glGetUniformLocation(id(), _name.c_str());
		glUseProgram(id());
		glUniformMatrix4fv(loc, static_cast<GLsizei>(values.size()), GL_FALSE, glm::value_ptr(values[0]));
		glUseProgram(0);
	}

	// Hard coded to cube map for now
	void Shader::uniform(const std::string& name, const std::shared_ptr<Texture> texture, int startingTextureUnit)
	{
		glUseProgram(id());

		glActiveTexture(GL_TEXTURE0 + startingTextureUnit);
		glBindTexture(GL_TEXTURE_CUBE_MAP, texture->id());

		GLint loc = glGetUniformLocation(id(), name.c_str());
		glUniform1i(loc, startingTextureUnit);

		glActiveTexture(GL_TEXTURE0);
		glUseProgram(0);
	}

	void Shader::uniform(const std::string& name, const std::vector<std::shared_ptr<Renderer::RenderTexture>>& textures, int startingTextureUnit)
	{
		glUseProgram(id());

		for (size_t i = 0; i < textures.size(); ++i)
		{
			int texUnit = startingTextureUnit + static_cast<int>(i);

			glActiveTexture(GL_TEXTURE0 + texUnit);
			glBindTexture(GL_TEXTURE_2D, textures[i]->getTextureId());

			std::string arrayName = name + "[" + std::to_string(i) + "]";
			GLint loc = glGetUniformLocation(id(), arrayName.c_str());
			glUniform1i(loc, texUnit);
		}

		glActiveTexture(GL_TEXTURE0);
		glUseProgram(0);
	}


	void Shader::draw(Model* _model, std::vector<Texture*>& _textures)
	{
		glUseProgram(id());

		if (!_model->usesMaterials())
		{
			if (!_textures.empty())
			{
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, _textures[0]->id());
				glUniform1i(glGetUniformLocation(id(), "u_AlbedoMap"), 0);
				glUniform1i(glGetUniformLocation(id(), "u_HasAlbedoMap"), true);

				glUniform1i(glGetUniformLocation(id(), "u_HasNormalMap"), false);
				glUniform1i(glGetUniformLocation(id(), "u_HasMetallicRoughnessMap"), false);
				glUniform1i(glGetUniformLocation(id(), "u_HasOcclusionMap"), false);
				glUniform1i(glGetUniformLocation(id(), "u_HasEmissiveMap"), false);

				// legacy path = opaque
				glUniform1i(glGetUniformLocation(id(), "u_AlphaMode"), 0);
				glUniform1f(glGetUniformLocation(id(), "u_AlphaCutoff"), 0.5f);
			}
			GLuint legacyVAO = _model->vao_id();
			glBindVertexArray(legacyVAO);
			glDrawArrays(GL_TRIANGLES, 0, _model->vertex_count());
		}
		else
		{
			const auto& embeddedTextures = _model->GetEmbeddedTextures();
			bool useEmbedded = !embeddedTextures.empty();
			const auto& groups = _model->GetMaterialGroups();

			// Pass 1: draw Opaque/Mask (depth writes on, blending off)
			glDisable(GL_BLEND);
			glDepthMask(GL_TRUE);

			for (size_t i = 0; i < groups.size(); ++i)
			{
				const auto& group = groups[i];

				// skip blend materials in pass 1
				if (group.pbr.alphaMode == Renderer::Model::PBRMaterial::AlphaMode::AlphaBlend)
					continue;

				if (useEmbedded)
				{
					const auto& pbr = group.pbr;

					// Albedo (base color)
					if (pbr.baseColorTexIndex >= 0 && pbr.baseColorTexIndex < embeddedTextures.size())
					{
						glActiveTexture(GL_TEXTURE0);
						glBindTexture(GL_TEXTURE_2D, embeddedTextures[pbr.baseColorTexIndex].id());
						glUniform1i(glGetUniformLocation(id(), "u_AlbedoMap"), 0);
						glUniform1i(glGetUniformLocation(id(), "u_HasAlbedoMap"), true);
					}
					else
						glUniform1i(glGetUniformLocation(id(), "u_HasAlbedoMap"), false);

					// Normal map
					if (pbr.normalTexIndex >= 0 && pbr.normalTexIndex < embeddedTextures.size())
					{
						glActiveTexture(GL_TEXTURE1);
						glBindTexture(GL_TEXTURE_2D, embeddedTextures[pbr.normalTexIndex].id());
						glUniform1i(glGetUniformLocation(id(), "u_NormalMap"), 1);
						glUniform1i(glGetUniformLocation(id(), "u_HasNormalMap"), 1);
					}
					else
						glUniform1i(glGetUniformLocation(id(), "u_HasNormalMap"), false);

					// Metallic-Roughness
					if (pbr.metallicRoughnessTexIndex >= 0 && pbr.metallicRoughnessTexIndex < embeddedTextures.size())
					{
						glActiveTexture(GL_TEXTURE2);
						glBindTexture(GL_TEXTURE_2D, embeddedTextures[pbr.metallicRoughnessTexIndex].id());
						glUniform1i(glGetUniformLocation(id(), "u_MetallicRoughnessMap"), 2);
						glUniform1i(glGetUniformLocation(id(), "u_HasMetallicRoughnessMap"), true);
					}
					else
						glUniform1i(glGetUniformLocation(id(), "u_HasMetallicRoughnessMap"), false);

					// Occlusion
					if (pbr.occlusionTexIndex >= 0 && pbr.occlusionTexIndex < embeddedTextures.size())
					{
						glActiveTexture(GL_TEXTURE3);
						glBindTexture(GL_TEXTURE_2D, embeddedTextures[pbr.occlusionTexIndex].id());
						glUniform1i(glGetUniformLocation(id(), "u_OcclusionMap"), 3);
						glUniform1i(glGetUniformLocation(id(), "u_HasOcclusionMap"), true);
					}
					else
						glUniform1i(glGetUniformLocation(id(), "u_HasOcclusionMap"), false);

					// Emissive
					if (pbr.emissiveTexIndex >= 0 && pbr.emissiveTexIndex < embeddedTextures.size())
					{
						glActiveTexture(GL_TEXTURE4);
						glBindTexture(GL_TEXTURE_2D, embeddedTextures[pbr.emissiveTexIndex].id());
						glUniform1i(glGetUniformLocation(id(), "u_EmissiveMap"), 4);
						glUniform1i(glGetUniformLocation(id(), "u_HasEmissiveMap"), true);
					}
					else
						glUniform1i(glGetUniformLocation(id(), "u_HasEmissiveMap"), false);

					// Material factors
					glUniform4fv(glGetUniformLocation(id(), "u_BaseColorFactor"), 1, glm::value_ptr(pbr.baseColorFactor));
					glUniform1f(glGetUniformLocation(id(), "u_MetallicFactor"), pbr.metallicFactor);
					glUniform1f(glGetUniformLocation(id(), "u_RoughnessFactor"), pbr.roughnessFactor);

					glUniform4fv(glGetUniformLocation(id(), "u_AlbedoFallback"), 1, glm::value_ptr(glm::vec4(1, 1, 1, 1)));
					glUniform1f(glGetUniformLocation(id(), "u_MetallicFallback"), 0.f);
					glUniform1f(glGetUniformLocation(id(), "u_RoughnessFallback"), 1.f);
					glUniform1f(glGetUniformLocation(id(), "u_AOFallback"), 1.f);
					glUniform3fv(glGetUniformLocation(id(), "u_EmissiveFallback"), 1, glm::value_ptr(glm::vec3(0, 0, 0)));

					// Alpha uniforms (Opaque/Mask -> non-blend)
					int alphaMode = 0;
					if (pbr.alphaMode == Renderer::Model::PBRMaterial::AlphaMode::AlphaMask) alphaMode = 1;
					// AlphaBlend is skipped in this pass
					glUniform1i(glGetUniformLocation(id(), "u_AlphaMode"), alphaMode);
					glUniform1f(glGetUniformLocation(id(), "u_AlphaCutoff"), pbr.alphaCutoff);
				}
				else if (i < _textures.size())
				{
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, _textures[i]->id());
					glUniform1i(glGetUniformLocation(id(), "u_AlbedoMap"), 0);
					glUniform1i(glGetUniformLocation(id(), "u_HasAlbedoMap"), true);

					glUniform1i(glGetUniformLocation(id(), "u_HasNormalMap"), false);
					glUniform1i(glGetUniformLocation(id(), "u_HasMetallicRoughnessMap"), false);
					glUniform1i(glGetUniformLocation(id(), "u_HasOcclusionMap"), false);
					glUniform1i(glGetUniformLocation(id(), "u_HasEmissiveMap"), false);

					// default opaque for non-embedded path
					glUniform1i(glGetUniformLocation(id(), "u_AlphaMode"), 0);
					glUniform1f(glGetUniformLocation(id(), "u_AlphaCutoff"), 0.5f);
				}

				glBindVertexArray(group.vao);
				glDrawArrays(GL_TRIANGLES, 0, group.faces.size() * 3);
			}

			// Pass 2: draw Blend (depth write off, blending on)
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glDepthMask(GL_FALSE);

			for (size_t i = 0; i < groups.size(); ++i)
			{
				const auto& group = groups[i];

				// only blend materials in pass 2
				if (group.pbr.alphaMode != Renderer::Model::PBRMaterial::AlphaMode::AlphaBlend)
					continue;

				if (useEmbedded)
				{
					const auto& pbr = group.pbr;

					// Albedo (base color)
					if (pbr.baseColorTexIndex >= 0 && pbr.baseColorTexIndex < embeddedTextures.size())
					{
						glActiveTexture(GL_TEXTURE0);
						glBindTexture(GL_TEXTURE_2D, embeddedTextures[pbr.baseColorTexIndex].id());
						glUniform1i(glGetUniformLocation(id(), "u_AlbedoMap"), 0);
						glUniform1i(glGetUniformLocation(id(), "u_HasAlbedoMap"), true);
					}
					else
						glUniform1i(glGetUniformLocation(id(), "u_HasAlbedoMap"), false);

					// Normal map
					if (pbr.normalTexIndex >= 0 && pbr.normalTexIndex < embeddedTextures.size())
					{
						glActiveTexture(GL_TEXTURE1);
						glBindTexture(GL_TEXTURE_2D, embeddedTextures[pbr.normalTexIndex].id());
						glUniform1i(glGetUniformLocation(id(), "u_NormalMap"), 1);
						glUniform1i(glGetUniformLocation(id(), "u_HasNormalMap"), 1);
					}
					else
						glUniform1i(glGetUniformLocation(id(), "u_HasNormalMap"), false);

					// Metallic-Roughness
					if (pbr.metallicRoughnessTexIndex >= 0 && pbr.metallicRoughnessTexIndex < embeddedTextures.size())
					{
						glActiveTexture(GL_TEXTURE2);
						glBindTexture(GL_TEXTURE_2D, embeddedTextures[pbr.metallicRoughnessTexIndex].id());
						glUniform1i(glGetUniformLocation(id(), "u_MetallicRoughnessMap"), 2);
						glUniform1i(glGetUniformLocation(id(), "u_HasMetallicRoughnessMap"), true);
					}
					else
						glUniform1i(glGetUniformLocation(id(), "u_HasMetallicRoughnessMap"), false);

					// Occlusion
					if (pbr.occlusionTexIndex >= 0 && pbr.occlusionTexIndex < embeddedTextures.size())
					{
						glActiveTexture(GL_TEXTURE3);
						glBindTexture(GL_TEXTURE_2D, embeddedTextures[pbr.occlusionTexIndex].id());
						glUniform1i(glGetUniformLocation(id(), "u_OcclusionMap"), 3);
						glUniform1i(glGetUniformLocation(id(), "u_HasOcclusionMap"), true);
					}
					else
						glUniform1i(glGetUniformLocation(id(), "u_HasOcclusionMap"), false);

					// Emissive
					if (pbr.emissiveTexIndex >= 0 && pbr.emissiveTexIndex < embeddedTextures.size())
					{
						glActiveTexture(GL_TEXTURE4);
						glBindTexture(GL_TEXTURE_2D, embeddedTextures[pbr.emissiveTexIndex].id());
						glUniform1i(glGetUniformLocation(id(), "u_EmissiveMap"), 4);
						glUniform1i(glGetUniformLocation(id(), "u_HasEmissiveMap"), true);
					}
					else
						glUniform1i(glGetUniformLocation(id(), "u_HasEmissiveMap"), false);

					// Material factors
					glUniform4fv(glGetUniformLocation(id(), "u_BaseColorFactor"), 1, glm::value_ptr(pbr.baseColorFactor));
					glUniform1f(glGetUniformLocation(id(), "u_MetallicFactor"), pbr.metallicFactor);
					glUniform1f(glGetUniformLocation(id(), "u_RoughnessFactor"), pbr.roughnessFactor);

					glUniform4fv(glGetUniformLocation(id(), "u_AlbedoFallback"), 1, glm::value_ptr(glm::vec4(1, 1, 1, 1)));
					glUniform1f(glGetUniformLocation(id(), "u_MetallicFallback"), 0.f);
					glUniform1f(glGetUniformLocation(id(), "u_RoughnessFallback"), 1.f);
					glUniform1f(glGetUniformLocation(id(), "u_AOFallback"), 1.f);
					glUniform3fv(glGetUniformLocation(id(), "u_EmissiveFallback"), 1, glm::value_ptr(glm::vec3(0, 0, 0)));

					// Alpha uniforms (Blend)
					glUniform1i(glGetUniformLocation(id(), "u_AlphaMode"), 2);
					glUniform1f(glGetUniformLocation(id(), "u_AlphaCutoff"), pbr.alphaCutoff);
				}
				else if (i < _textures.size())
				{
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, _textures[i]->id());
					glUniform1i(glGetUniformLocation(id(), "u_AlbedoMap"), 0);
					glUniform1i(glGetUniformLocation(id(), "u_HasAlbedoMap"), true);

					glUniform1i(glGetUniformLocation(id(), "u_HasNormalMap"), false);
					glUniform1i(glGetUniformLocation(id(), "u_HasMetallicRoughnessMap"), false);
					glUniform1i(glGetUniformLocation(id(), "u_HasOcclusionMap"), false);
					glUniform1i(glGetUniformLocation(id(), "u_HasEmissiveMap"), false);

					// treat non-embedded as opaque by default; if you truly want it blended, set uniforms before calling draw
					glUniform1i(glGetUniformLocation(id(), "u_AlphaMode"), 0);
					glUniform1f(glGetUniformLocation(id(), "u_AlphaCutoff"), 0.5f);
				}

				glBindVertexArray(group.vao);
				glDrawArrays(GL_TRIANGLES, 0, group.faces.size() * 3);
			}
		}

		glEnable(GL_MULTISAMPLE);
		glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);

		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);

		glBindVertexArray(0);
		glUseProgram(0);
	}

	void Shader::draw(Model* _model, Texture* _tex)
	{
		glUseProgram(id());
		glBindVertexArray(_model->vao_id());
		glBindTexture(GL_TEXTURE_2D, _tex->id());
		glUniform1i(glGetUniformLocation(id(), "u_Texture"), 0);
		glDrawArrays(GL_TRIANGLES, 0, _model->vertex_count());
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glUseProgram(0);
	}

	void Shader::draw(Mesh* _mesh)
	{
		glUseProgram(id());
		glBindVertexArray(_mesh->id());
		glDrawArrays(GL_TRIANGLES, 0, _mesh->vertex_count());
		glBindVertexArray(0);
		glUseProgram(0);
	}

	void Shader::draw(Mesh* _mesh, Texture* _tex)
	{
		glUseProgram(id());
		glBindVertexArray(_mesh->id());
		glBindTexture(GL_TEXTURE_2D, _tex->id());
		glUniform1i(glGetUniformLocation(id(), "u_Texture"), 0);
		glDrawArrays(GL_TRIANGLES, 0, _mesh->vertex_count());
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glUseProgram(0);
	}

	void Shader::draw(Mesh& _mesh, Texture& _tex)
	{
		glUseProgram(id());
		glBindVertexArray(_mesh.id());
		glBindTexture(GL_TEXTURE_2D, _tex.id());
		glUniform1i(glGetUniformLocation(id(), "u_Texture"), 0);
		glDrawArrays(GL_TRIANGLES, 0, _mesh.vertex_count());
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glUseProgram(0);
	}

	void Shader::draw(Mesh* _mesh, GLuint _texId)
	{
		glUseProgram(id());
		glBindVertexArray(_mesh->id());
		glBindTexture(GL_TEXTURE_2D, _texId);
		glUniform1i(glGetUniformLocation(id(), "u_Texture"), 0);
		glDrawArrays(GL_TRIANGLES, 0, _mesh->vertex_count());
		glBindVertexArray(0);
		glUseProgram(0);
	}

	void Shader::draw(Mesh& _mesh, GLuint _texId)
	{
		glUseProgram(id());
		glBindVertexArray(_mesh.id());
		glBindTexture(GL_TEXTURE_2D, _texId);
		glUniform1i(glGetUniformLocation(id(), "u_Texture"), 0);
		glDrawArrays(GL_TRIANGLES, 0, _mesh.vertex_count());
		glUseProgram(0);
	}

	void Shader::draw(Model& _model, Texture& _tex)
	{
		glUseProgram(id());
		glBindVertexArray(_model.vao_id());
		glBindTexture(GL_TEXTURE_2D, _tex.id());
		glUniform1i(glGetUniformLocation(id(), "u_Texture"), 0);
		glDrawArrays(GL_TRIANGLES, 0, _model.vertex_count());
		glUseProgram(0);
	}

	void Shader::draw(Model& _model, GLuint _texId)
	{
		glUseProgram(id());
		glBindVertexArray(_model.vao_id());
		glBindTexture(GL_TEXTURE_2D, _texId);
		glUniform1i(glGetUniformLocation(id(), "u_Texture"), 0);
		glDrawArrays(GL_TRIANGLES, 0, _model.vertex_count());
		glUseProgram(0);
	}

	void Shader::draw(Model& _model, Texture& _tex, RenderTexture& _renderTex)
	{
		_renderTex.bind();

		int viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);

		glViewport(0, 0, _renderTex.getWidth(), _renderTex.getHeight());

		glUseProgram(id());
		glBindVertexArray(_model.vao_id());
		glBindTexture(GL_TEXTURE_2D, _tex.id());
		glDrawArrays(GL_TRIANGLES, 0, _model.vertex_count());
		glUseProgram(0);

		_renderTex.unbind();

		glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
	}

	void Shader::draw(Mesh& _mesh, Texture& _tex, RenderTexture& _renderTex)
	{
		_renderTex.bind();

		int viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);

		glViewport(0, 0, _renderTex.getWidth(), _renderTex.getHeight());

		glUseProgram(id());
		glBindVertexArray(_mesh.id());
		glBindTexture(GL_TEXTURE_2D, _tex.id());
		glDrawArrays(GL_TRIANGLES, 0, _mesh.vertex_count());
		glUseProgram(0);

		_renderTex.unbind();

		glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
	}

	void Shader::drawSkybox(Mesh& _skyboxMesh, Texture& _tex)
	{
		// Change depth function so depth test passes when values are equal to depth buffer's content
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_FALSE);

		glUseProgram(id());
		glBindTexture(GL_TEXTURE_CUBE_MAP, _tex.id());
		GLuint textureLocation = glGetUniformLocation(id(), "uTexEnv");
		glUniform1i(textureLocation, 0);
		glBindVertexArray(_skyboxMesh.id());
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);
		glUseProgram(0);

		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
	}

	void Shader::drawSkybox(Mesh* _skyboxMesh, Texture* _tex)
	{
		// Change depth function so depth test passes when values are equal to depth buffer's content
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_FALSE);

		glUseProgram(id());
		glBindTexture(GL_TEXTURE_CUBE_MAP, _tex->id());
		GLuint textureLocation = glGetUniformLocation(id(), "uTexEnv");
		glUniform1i(textureLocation, 0);
		glBindVertexArray(_skyboxMesh->id());
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);
		glUseProgram(0);

		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
	}

	void Shader::drawText(Mesh& _mesh, Font& _font, const std::string& _text, float _x, float _y, float _scale)
	{
		// Call id incase mesh hasn't been generated yet
		_mesh.id();

		glUseProgram(id());
		glActiveTexture(GL_TEXTURE0);
		glBindVertexArray(_mesh.vao());

		float copyX = _x;
		float currentLineWidth = 0.0f;
		float widestLineWidth = 0.0f;

		// iterate through all characters
		std::string::const_iterator c;
		for (c = _text.begin(); c != _text.end(); c++)
		{
			Character* ch = _font.GetCharacter(c);

			if (*c == '\n')
			{
				_y -= ((ch->Size.y)) * 1.3 * _scale;
				_x = copyX;
				currentLineWidth = 0.0f;
			}
			else
			{
				float xpos = _x + ch->Bearing.x * _scale;
				float ypos = _y - (ch->Size.y - ch->Bearing.y) * _scale;

				float w = ch->Size.x * _scale;
				float h = ch->Size.y * _scale;

				currentLineWidth += w;
				if (currentLineWidth > widestLineWidth) widestLineWidth = currentLineWidth;

				// update VBO for each character
				float vertices[6][4] = {
					{ xpos,     ypos + h,   0.0f, 0.0f },
					{ xpos,     ypos,       0.0f, 1.0f },
					{ xpos + w, ypos,       1.0f, 1.0f },

					{ xpos,     ypos + h,   0.0f, 0.0f },
					{ xpos + w, ypos,       1.0f, 1.0f },
					{ xpos + w, ypos + h,   1.0f, 0.0f }
				};
				// render glyph texture over quad
				glBindTexture(GL_TEXTURE_2D, ch->TextureID);
				// update content of VBO memory
				glBindBuffer(GL_ARRAY_BUFFER, _mesh.vbo());
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				// render quad
				glDrawArrays(GL_TRIANGLES, 0, 6);
				// now advance cursors for next glyph (note that advance is number of 1/64 pixels)
				_x += (ch->Advance >> 6) * _scale; // bitshift by 6 to get value in pixels (2^6 = 64)
			}
		}

		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glUseProgram(0);
	}

	void Shader::drawOutline(Model* _model)
	{
		glUseProgram(id());
		glBindVertexArray(_model->vao_id());
		glDrawArrays(GL_LINE_LOOP, 0, _model->vertex_count());
		glBindVertexArray(0);
		glUseProgram(0);
	}
}