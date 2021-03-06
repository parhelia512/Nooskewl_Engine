#include "Nooskewl_Engine/engine.h"
#include "Nooskewl_Engine/error.h"
#include "Nooskewl_Engine/image.h"
#include "Nooskewl_Engine/internal.h"
#include "Nooskewl_Engine/shader.h"

using namespace Nooskewl_Engine;

std::vector<Shader::Internal *> Shader::loaded_shaders;

void Shader::release_all()
{
	for (size_t i = 0; i < loaded_shaders.size(); i++) {
		loaded_shaders[i]->release();
	}
}

void Shader::reload_all()
{
	for (size_t i = 0; i < loaded_shaders.size(); i++) {
		loaded_shaders[i]->reload();
	}
}

Shader::Shader(bool opengl, std::string vertex_source, std::string fragment_source) :
	global_alpha(1.0f)
{
	internal = new Internal();

	internal->opengl = opengl;
	internal->vertex_source = vertex_source;
	internal->fragment_source = fragment_source;

	loaded_shaders.push_back(internal);

	internal->reload();
}

Shader::~Shader()
{
	internal->release();
	for (size_t i = 0; i < loaded_shaders.size(); i++) {
		if (loaded_shaders[i] == internal) {
			loaded_shaders.erase(loaded_shaders.begin()+i);
			break;
		}
	}
	delete internal;
}

void Shader::use()
{
	if (internal->opengl) {
		glUseProgram(internal->opengl_shader);
		printGLerror("glUseProgram");
	}

	noo.update_projection();
}

void Shader::set_texture(std::string name, Image *image)
{
	if (internal->opengl) {
		if (image) {
			glActiveTexture(GL_TEXTURE0);
			printGLerror("glActiveTexture");

			glBindTexture(GL_TEXTURE_2D, image->internal->texture);
			printGLerror("glBindTexture");

			GLint loc = glGetUniformLocation(internal->opengl_shader, "tex");
			glUniform1i(loc, 0);
		}
	}
#ifdef NOOSKEWL_ENGINE_WINDOWS
	else {
		if (image != 0) {
			internal->d3d_effect->SetTexture("tex", image->internal->video_texture);
			noo.d3d_device->SetTexture(0, image->internal->video_texture);
		}
		else {
			noo.d3d_device->SetTexture(0, 0);
		}
	}
#endif
}

void Shader::set_matrix(std::string name, const float *matrix)
{
	if (internal->opengl) {
		GLint uni = glGetUniformLocation(internal->opengl_shader, name.c_str());
		printGLerror("glGetUniformLocation");
		if (uni != -1) {
			glUniformMatrix4fv(uni, 1, GL_FALSE, matrix);
			printGLerror("glUniformMatrix4fv");
		}
	}
#ifdef NOOSKEWL_ENGINE_WINDOWS
	else {
		internal->d3d_effect->SetMatrix(name.c_str(), (D3DXMATRIX *)matrix);
	}
#endif
}

void Shader::set_float(std::string name, float value)
{
	if (internal->opengl) {
		GLint uni = glGetUniformLocation(internal->opengl_shader, name.c_str());
		printGLerror("glGetUniformLocation");
		if (uni != -1) {
			glUniform1f(uni, value);
			printGLerror("glUniform1f");
		}
	}
#ifdef NOOSKEWL_ENGINE_WINDOWS
	else {
		internal->d3d_effect->SetFloat(name.c_str(), value);
	}
#endif
}

// Taken from Allegro
bool Shader::set_float_vector(std::string name, int num_components, float *vector, int num_elements)
{
	if (noo.opengl) {
		GLint handle;

		handle = glGetUniformLocation(internal->opengl_shader, name.c_str());

		if (handle < 0) {
			return false;
		}

		switch (num_components) {
			case 1:
				glUniform1fv(handle, num_elements, vector);
				break;
			case 2:
				glUniform2fv(handle, num_elements, vector);
				break;
			case 3:
				glUniform3fv(handle, num_elements, vector);
				break;
			case 4:
				glUniform4fv(handle, num_elements, vector);
				break;
			default:
				return false;
		}
	}
#ifdef NOOSKEWL_ENGINE_WINDOWS
	else {
		return internal->d3d_effect->SetFloatArray(name.c_str(), vector, num_components * num_elements) == D3D_OK;
	}
#endif

	return true;
}

void Shader::set_bool(std::string name, bool value)
{
	if (internal->opengl) {
		GLint uni = glGetUniformLocation(internal->opengl_shader, name.c_str());
		printGLerror("glGetUniformLocation");
		if (uni != -1) {
			glUniform1i(uni, value);
			printGLerror("glUniform1i");
		}
	}
#ifdef NOOSKEWL_ENGINE_WINDOWS
	else {
		internal->d3d_effect->SetBool(name.c_str(), value);
	}
#endif
}

GLuint Shader::get_opengl_shader()
{
	return internal->opengl_shader;
}

#ifdef NOOSKEWL_ENGINE_WINDOWS
LPD3DXEFFECT Shader::get_d3d_effect()
{
	return internal->d3d_effect;
}
#endif

void Shader::Internal::release()
{
	if (opengl) {
		glDeleteShader(opengl_vertex_shader);
		printGLerror("glDeleteShader");
		glDeleteShader(opengl_fragment_shader);
		printGLerror("glDeleteShader");
		glDeleteProgram(opengl_shader);
		printGLerror("glDeleteProgram");
	}
#ifdef NOOSKEWL_ENGINE_WINDOWS
	else {
		d3d_effect->Release();
	}
#endif
}

void Shader::Internal::reload()
{

	if (opengl) {
		GLint status;

		const GLchar *p = (const GLchar *)vertex_source.c_str();

		opengl_vertex_shader = glCreateShader(GL_VERTEX_SHADER);
		printGLerror("glCreateShader");
		glShaderSource(opengl_vertex_shader, 1, &p, 0);
		printGLerror("glShaderSource");
		glCompileShader(opengl_vertex_shader);
		printGLerror("glCompileShader");
		glGetShaderiv(opengl_vertex_shader, GL_COMPILE_STATUS, &status);
		printGLerror("glGetShaderiv");
		if (status != GL_TRUE) {
			char buffer[512];
			glGetShaderInfoLog(opengl_vertex_shader, 512, 0, buffer);
			errormsg("Vertex shader error: %s\n", buffer);
		}

		p = (const GLchar *)fragment_source.c_str();

		opengl_fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
		printGLerror("glCreateShader");
		glShaderSource(opengl_fragment_shader, 1, &p, 0);
		printGLerror("glShaderSource");
		glCompileShader(opengl_fragment_shader);
		printGLerror("glCompileShader");
		glGetShaderiv(opengl_fragment_shader, GL_COMPILE_STATUS, &status);
		printGLerror("glGetShaderiv");
		if (status != GL_TRUE) {
			char buffer[512];
			glGetShaderInfoLog(opengl_fragment_shader, 512, 0, buffer);
			errormsg("Fragment shader error: %s\n", buffer);
		}

		opengl_shader = glCreateProgram();
		glAttachShader(opengl_shader, opengl_vertex_shader);
		printGLerror("glAttachShader");
		glAttachShader(opengl_shader, opengl_fragment_shader);
		printGLerror("glAttachShader");
		glLinkProgram(opengl_shader);
		printGLerror("glLinkProgram");
	}
#ifdef NOOSKEWL_ENGINE_WINDOWS
	else {
		LPD3DXBUFFER errors;

		std::string shader_source = vertex_source + fragment_source;

		shader_source +=
			"technique TECH"
			"{"
			"		pass p1"
			"		{"
			"				VertexShader = compile vs_2_0 vs_main();"
			"				PixelShader = compile ps_2_0 ps_main();"
			"		}"
			"}";

		DWORD result = D3DXCreateEffect(noo.d3d_device, shader_source.c_str(), shader_source.length(), 0, 0, D3DXSHADER_PACKMATRIX_ROWMAJOR, 0, &d3d_effect, &errors);

		if (result != D3D_OK) {
			char *msg = (char *)errors->GetBufferPointer();
			throw Error("Shader error: " + std::string(msg));
		}

		d3d_technique = d3d_effect->GetTechniqueByName("TECH");
		d3d_effect->ValidateTechnique(d3d_technique);
		d3d_effect->SetTechnique(d3d_technique);
	}
#endif
}

float Shader::get_global_alpha()
{
	return global_alpha;
}

void Shader::set_global_alpha(float global_alpha)
{
	this->global_alpha = global_alpha;
	set_float("global_alpha", global_alpha);
}
