///source for shader program - bbased on sf::Shader///

#include <Mesh/ShaderProgram.h>
#include <Mesh/GL/GLCheck.hpp>

#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/InputStream.hpp>
#include <SFML/System/Err.hpp>

#include <fstream>
#include <vector>


namespace
{
    // Retrieve the maximum number of texture units available
    GLint getMaxTextureUnits()
    {
        GLint maxUnits;
        glCheck(glGetIntegerv(GL_MAX_TEXTURE_COORDS_ARB, &maxUnits));
        return maxUnits;
    }

    // Read the contents of a file into an array of char
    bool getFileContents(const std::string& filename, std::vector<char>& buffer)
    {
        std::ifstream file(filename.c_str(), std::ios_base::binary);
        if (file)
        {
            file.seekg(0, std::ios_base::end);
            std::streamsize size = file.tellg();
            if (size > 0)
            {
                file.seekg(0, std::ios_base::beg);
                buffer.resize(static_cast<std::size_t>(size));
                file.read(&buffer[0], size);
            }
            buffer.push_back('\0');
            return true;
        }
        else
        {
            return false;
        }
    }

    // Read the contents of a stream into an array of char
    bool getStreamContents(sf::InputStream& stream, std::vector<char>& buffer)
    {
        bool success = true;
        sf::Int64 size = stream.getSize();
        if (size > 0)
        {
            buffer.resize(static_cast<std::size_t>(size));
            stream.seek(0);
            sf::Int64 read = stream.read(&buffer[0], size);
            success = (read == size);
        }
        buffer.push_back('\0');
        return success;
    }
}


namespace ml
{
	////////////////////////////////////////////////////////////
	ShaderProgram::CurrentTextureType ShaderProgram::CurrentTexture;


	////////////////////////////////////////////////////////////
	ShaderProgram::ShaderProgram() :
	m_shaderProgram (0),
	m_currentTexture(-1),
	m_textures (),
	m_params ()
	{
	}


	////////////////////////////////////////////////////////////
	ShaderProgram::~ShaderProgram()
	{
		ensureGlContext();

		// Destroy effect program
		if (m_shaderProgram)
			glCheck(glDeleteObjectARB(m_shaderProgram));
	}


	////////////////////////////////////////////////////////////
	bool ShaderProgram::loadFromFile(const std::string& filename, Type type)
	{
		// Read the file
		std::vector<char> shader;
		if (!getFileContents(filename, shader))
		{
			sf::err() << "Failed to open shader file \"" << filename << "\"" << std::endl;
			return false;
		}

		// Compile the shader program
		if (type == Vertex)
			return compile(&shader[0], NULL);
		else
			return compile(NULL, &shader[0]);
	}


	////////////////////////////////////////////////////////////
	bool ShaderProgram::loadFromFile(const std::string& vertexShaderFilename, const std::string& fragmentShaderFilename)
	{
		// Read the vertex shader file
		std::vector<char> vertexShader;
		if (!getFileContents(vertexShaderFilename, vertexShader))
		{
			sf::err() << "Failed to open vertex shader file \"" << vertexShaderFilename << "\"" << std::endl;
			return false;
		}

		// Read the fragment shader file
		std::vector<char> fragmentShader;
		if (!getFileContents(fragmentShaderFilename, fragmentShader))
		{
			sf::err() << "Failed to open fragment shader file \"" << fragmentShaderFilename << "\"" << std::endl;
			return false;
		}

		// Compile the shader program
		return compile(&vertexShader[0], &fragmentShader[0]);
	}


	////////////////////////////////////////////////////////////
	bool ShaderProgram::loadFromMemory(const std::string& shader, Type type)
	{
		// Compile the shader program
		if (type == Vertex)
			return compile(shader.c_str(), NULL);
		else
			return compile(NULL, shader.c_str());
	}


	////////////////////////////////////////////////////////////
	bool ShaderProgram::loadFromMemory(const std::string& vertexShader, const std::string& fragmentShader)
	{
		// Compile the shader program
		return compile(vertexShader.c_str(), fragmentShader.c_str());
	}


	////////////////////////////////////////////////////////////
	bool ShaderProgram::loadFromStream(sf::InputStream& stream, Type type)
	{
		// Read the shader code from the stream
		std::vector<char> shader;
		if (!getStreamContents(stream, shader))
		{
			sf::err() << "Failed to read shader from stream" << std::endl;
			return false;
		}

		// Compile the shader program
		if (type == Vertex)
			return compile(&shader[0], NULL);
		else
			return compile(NULL, &shader[0]);
	}


	////////////////////////////////////////////////////////////
	bool ShaderProgram::loadFromStream(sf::InputStream& vertexShaderStream, sf::InputStream& fragmentShaderStream)
	{
		// Read the vertex shader code from the stream
		std::vector<char> vertexShader;
		if (!getStreamContents(vertexShaderStream, vertexShader))
		{
			sf::err() << "Failed to read vertex shader from stream" << std::endl;
			return false;
		}

		// Read the fragment shader code from the stream
		std::vector<char> fragmentShader;
		if (!getStreamContents(fragmentShaderStream, fragmentShader))
		{
			sf::err() << "Failed to read fragment shader from stream" << std::endl;
			return false;
		}

		// Compile the shader program
		return compile(&vertexShader[0], &fragmentShader[0]);
	}


	////////////////////////////////////////////////////////////
	void ShaderProgram::setParameter(const std::string& name, float x)
	{
		if (m_shaderProgram)
		{
			ensureGlContext();

			// Enable program
			GLhandleARB program = glGetHandleARB(GL_PROGRAM_OBJECT_ARB);
			glCheck(glUseProgramObjectARB(m_shaderProgram));

			// Get parameter location and assign it new values
			GLint location = getParamLocation(name);
			if (location != -1)
				glCheck(glUniform1fARB(location, x));

			// Disable program
			glCheck(glUseProgramObjectARB(program));
		}
	}


	////////////////////////////////////////////////////////////
	void ShaderProgram::setParameter(const std::string& name, float x, float y)
	{
		if (m_shaderProgram)
		{
			ensureGlContext();

			// Enable program
			GLhandleARB program = glGetHandleARB(GL_PROGRAM_OBJECT_ARB);
			glCheck(glUseProgramObjectARB(m_shaderProgram));

			// Get parameter location and assign it new values
			GLint location = getParamLocation(name);
			if (location != -1)
				glCheck(glUniform2fARB(location, x, y));

			// Disable program
			glCheck(glUseProgramObjectARB(program));
		}
	}


	////////////////////////////////////////////////////////////
	void ShaderProgram::setParameter(const std::string& name, float x, float y, float z)
	{
		if (m_shaderProgram)
		{
			ensureGlContext();

			// Enable program
			GLhandleARB program = glGetHandleARB(GL_PROGRAM_OBJECT_ARB);
			glCheck(glUseProgramObjectARB(m_shaderProgram));

			// Get parameter location and assign it new values
			GLint location = getParamLocation(name);
			if (location != -1)
				glCheck(glUniform3fARB(location, x, y, z));

			// Disable program
			glCheck(glUseProgramObjectARB(program));
		}
	}


	////////////////////////////////////////////////////////////
	void ShaderProgram::setParameter(const std::string& name, float x, float y, float z, float w)
	{
		if (m_shaderProgram)
		{
			ensureGlContext();

			// Enable program
			GLhandleARB program = glGetHandleARB(GL_PROGRAM_OBJECT_ARB);
			glCheck(glUseProgramObjectARB(m_shaderProgram));

			// Get parameter location and assign it new values
			GLint location = getParamLocation(name);
			if (location != -1)
				glCheck(glUniform4fARB(location, x, y, z, w));

			// Disable program
			glCheck(glUseProgramObjectARB(program));
		}
	}


	////////////////////////////////////////////////////////////
	void ShaderProgram::setParameter(const std::string& name, const sf::Vector2f& v)
	{
		setParameter(name, v.x, v.y);
	}


	////////////////////////////////////////////////////////////
	void ShaderProgram::setParameter(const std::string& name, const sf::Vector3f& v)
	{
		setParameter(name, v.x, v.y, v.z);
	}


	////////////////////////////////////////////////////////////
	void ShaderProgram::setParameter(const std::string& name, const sf::Color& color)
	{
		setParameter(name, color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f);
	}


	////////////////////////////////////////////////////////////
	void ShaderProgram::setParameter(const std::string& name, const sf::Transform& transform)
	{
		setParameter(name, transform.getMatrix());
	}

	void ShaderProgram::setParameter(const std::string& name, const float* matrix)
	{
		if (m_shaderProgram)
		{
			ensureGlContext();

			// Enable program
			GLhandleARB program = glGetHandleARB(GL_PROGRAM_OBJECT_ARB);
			glCheck(glUseProgramObjectARB(m_shaderProgram));

			// Get parameter location and assign it new values
			GLint location = getParamLocation(name);
			if (location != -1)
				glCheck(glUniformMatrix4fvARB(location, 1, GL_FALSE, matrix));

			// Disable program
			glCheck(glUseProgramObjectARB(program));
		}
	}


	////////////////////////////////////////////////////////////
	void ShaderProgram::setParameter(const std::string& name, const sf::Texture& texture)
	{
		if (m_shaderProgram)
		{
			ensureGlContext();

			// Find the location of the variable in the shader
			int location = getParamLocation(name);
			if (location != -1)
			{
				// Store the location -> texture mapping
				TextureTable::iterator it = m_textures.find(location);
				if (it == m_textures.end())
				{
					// New entry, make sure there are enough texture units
					static const GLint maxUnits = getMaxTextureUnits();
					if (m_textures.size() + 1 >= static_cast<std::size_t>(maxUnits))
					{
					   sf::err() << "Impossible to use texture \"" << name << "\" for shader: all available texture units are used" << std::endl;
						return;
					}

					m_textures[location] = &texture;
				}
				else
				{
					// Location already used, just replace the texture
					it->second = &texture;
				}
			}
		}
	}


	////////////////////////////////////////////////////////////
	void ShaderProgram::setParameter(const std::string& name, CurrentTextureType)
	{
		if (m_shaderProgram)
		{
			ensureGlContext();

			// Find the location of the variable in the shader
			m_currentTexture = getParamLocation(name);
		}
	}

	////////////////////////////////////////////////////////////
	void ShaderProgram::setVertexAttribute(const std::string& name, GLint dataSize, const void* dataBuffer)
	{
		if(m_shaderProgram)
		{
			ensureGlContext();

			GLhandleARB program = glGetHandleARB(GL_PROGRAM_OBJECT_ARB);
			glCheck(glUseProgramObjectARB(m_shaderProgram));

			GLint location = getParamLocation(name);
			if(location != -1)
				glCheck(glVertexAttribPointerARB(location, dataSize, GL_FLOAT, GL_FALSE, 0, dataBuffer));

			glCheck(glUseProgramObjectARB(program));
		}
	}

	////////////////////////////////////////////////////////////
	void ShaderProgram::bind(const ShaderProgram* shader)
	{
		ensureGlContext();

		if (shader && shader->m_shaderProgram)
		{
			// Enable the program
			glCheck(glUseProgramObjectARB(shader->m_shaderProgram));

			// Bind the textures
			shader->bindTextures();

			// Bind the current texture
			if (shader->m_currentTexture != -1)
				glCheck(glUniform1iARB(shader->m_currentTexture, 0));
		}
		else
		{
			// Bind no shader
			glCheck(glUseProgramObjectARB(0));
		}
	}


	////////////////////////////////////////////////////////////
	bool ShaderProgram::isAvailable()
	{
		ensureGlContext();

		// Make sure that GLEW is initialized
		sf::priv::ensureGlewInit();

		return GLEW_ARB_shading_language_100 &&
			   GLEW_ARB_shader_objects &&
			   GLEW_ARB_vertex_shader &&
			   GLEW_ARB_fragment_shader;
	}


	////////////////////////////////////////////////////////////
	bool ShaderProgram::compile(const char* vertexShaderCode, const char* fragmentShaderCode)
	{
		ensureGlContext();

		// First make sure that we can use shaders
		if (!isAvailable())
		{
			sf::err() << "Failed to create a shader: your system doesn't support shaders "
				  << "(you should test Shader::isAvailable() before trying to use the Shader class)" << std::endl;
			return false;
		}

		// Destroy the shader if it was already created
		if (m_shaderProgram)
			glCheck(glDeleteObjectARB(m_shaderProgram));

		// Reset the internal state
		m_currentTexture = -1;
		m_textures.clear();
		m_params.clear();

		// Create the program
		m_shaderProgram = glCreateProgramObjectARB();

		// Create the vertex shader if needed
		if (vertexShaderCode)
		{
			// Create and compile the shader
			GLhandleARB vertexShader = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
			glCheck(glShaderSourceARB(vertexShader, 1, &vertexShaderCode, NULL));
			glCheck(glCompileShaderARB(vertexShader));

			// Check the compile log
			GLint success;
			glCheck(glGetObjectParameterivARB(vertexShader, GL_OBJECT_COMPILE_STATUS_ARB, &success));
			if (success == GL_FALSE)
			{
				char log[1024];
				glCheck(glGetInfoLogARB(vertexShader, sizeof(log), 0, log));
				sf::err() << "Failed to compile vertex shader:" << std::endl
					  << log << std::endl;
				glCheck(glDeleteObjectARB(vertexShader));
				glCheck(glDeleteObjectARB(m_shaderProgram));
				m_shaderProgram = 0;
				return false;
			}

			// Attach the shader to the program, and delete it (not needed anymore)
			glCheck(glAttachObjectARB(m_shaderProgram, vertexShader));
			glCheck(glDeleteObjectARB(vertexShader));
		}

		// Create the fragment shader if needed
		if (fragmentShaderCode)
		{
			// Create and compile the shader
			GLhandleARB fragmentShader = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
			glCheck(glShaderSourceARB(fragmentShader, 1, &fragmentShaderCode, NULL));
			glCheck(glCompileShaderARB(fragmentShader));

			// Check the compile log
			GLint success;
			glCheck(glGetObjectParameterivARB(fragmentShader, GL_OBJECT_COMPILE_STATUS_ARB, &success));
			if (success == GL_FALSE)
			{
				char log[1024];
				glCheck(glGetInfoLogARB(fragmentShader, sizeof(log), 0, log));
				sf::err() << "Failed to compile fragment shader:" << std::endl
					  << log << std::endl;
				glCheck(glDeleteObjectARB(fragmentShader));
				glCheck(glDeleteObjectARB(m_shaderProgram));
				m_shaderProgram = 0;
				return false;
			}

			// Attach the shader to the program, and delete it (not needed anymore)
			glCheck(glAttachObjectARB(m_shaderProgram, fragmentShader));
			glCheck(glDeleteObjectARB(fragmentShader));
		}

		// Link the program
		glCheck(glLinkProgramARB(m_shaderProgram));

		// Check the link log
		GLint success;
		glCheck(glGetObjectParameterivARB(m_shaderProgram, GL_OBJECT_LINK_STATUS_ARB, &success));
		if (success == GL_FALSE)
		{
			char log[1024];
			glCheck(glGetInfoLogARB(m_shaderProgram, sizeof(log), 0, log));
			sf::err() << "Failed to link shader:" << std::endl
				  << log << std::endl;
			glCheck(glDeleteObjectARB(m_shaderProgram));
			m_shaderProgram = 0;
			return false;
		}

		// Force an OpenGL flush, so that the shader will appear updated
		// in all contexts immediately (solves problems in multi-threaded apps)
		glCheck(glFlush());

		return true;
	}


	////////////////////////////////////////////////////////////
	void ShaderProgram::bindTextures() const
	{
		TextureTable::const_iterator it = m_textures.begin();
		for (std::size_t i = 0; i < m_textures.size(); ++i)
		{
			GLint index = static_cast<GLsizei>(i + 1);
			glCheck(glUniform1iARB(it->first, index));
			glCheck(glActiveTextureARB(GL_TEXTURE0_ARB + index));
			sf::Texture::bind(it->second);
			++it;
		}

		// Make sure that the texture unit which is left active is the number 0
		glCheck(glActiveTextureARB(GL_TEXTURE0_ARB));
	}


	////////////////////////////////////////////////////////////
	int ShaderProgram::getParamLocation(const std::string& name)
	{
		// Check the cache
		ParamTable::const_iterator it = m_params.find(name);
		if (it != m_params.end())
		{
			// Already in cache, return it
			return it->second;
		}
		else
		{
			// Not in cache, request the location from OpenGL
			int location = glGetUniformLocationARB(m_shaderProgram, name.c_str());
			if (location != -1)
			{
				// Location found: add it to the cache
				m_params.insert(std::make_pair(name, location));
			}
			else
			{
				// Error: location not found
				sf::err() << "Parameter \"" << name << "\" not found in shader" << std::endl;
			}

			return location;
		}
	}
} 