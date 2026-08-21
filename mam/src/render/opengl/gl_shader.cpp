#include "render/opengl/gl_shader.hpp"

mam::GLShader::GLShader()
{
  is_compiled_ = false;
  type_ = Shader::Invalid;
  id_ = 0;
}

mam::GLShader::~GLShader()
{
  glDeleteShader(id_);
}

void mam::GLShader::loadSource(const Type shader_type,
                               const char *source,
                               const unsigned int source_size)
{
  type_ = shader_type;
  switch (shader_type)
  {
  case Shader::Type::Vertex:
    id_ = glCreateShader(GL_VERTEX_SHADER);
    break;
  case Shader::Type::Fragment:
    id_ = glCreateShader(GL_FRAGMENT_SHADER);
    break;
  case Shader::Type::Geometry:
    id_ = glCreateShader(GL_GEOMETRY_SHADER);
    break;
  }

  GLint size = source_size;
  glShaderSource(id_, 1, &source, &size);
}

void mam::GLShader::loadSourceFromFile(const Type shader_type, const char *path)
{
  std::ifstream shaderSource;
  u32 sourceLength;
  shaderSource.open(path);
  shaderSource.seekg(0, std::ios::end);
  sourceLength = (u32)shaderSource.tellg();
  shaderSource.seekg(0, std::ios::beg);
  std::unique_ptr<char[]> buffer = std::make_unique<char[]>(sourceLength + 1);
  shaderSource.read(buffer.get(), sourceLength);
  buffer[sourceLength] = '\0';
  shaderSource.close();

  type_ = shader_type;
  switch (shader_type)
  {
  case Shader::Type::Vertex:
    id_ = glCreateShader(GL_VERTEX_SHADER);
    break;
  case Shader::Type::Fragment:
    id_ = glCreateShader(GL_FRAGMENT_SHADER);
    break;
  case Shader::Type::Geometry:
    id_ = glCreateShader(GL_GEOMETRY_SHADER);
    break;
  }

  const GLchar *src = buffer.get();
  GLint size = sourceLength;
  glShaderSource(id_, 1, &src, &size);
}

bool mam::GLShader::compile()
{
  glCompileShader(id_);

  GLint isCompiled = GL_FALSE;
  glGetShaderiv(id_, GL_COMPILE_STATUS, &isCompiled);
  if (isCompiled == GL_FALSE)
  {
    GLint maxLength = 0;
    glGetShaderiv(id_, GL_INFO_LOG_LENGTH, &maxLength);

    std::unique_ptr<char[]> errorLog = std::make_unique<char[]>(maxLength + 1);
    glGetShaderInfoLog(id_, maxLength, &maxLength, errorLog.get());
    errorLog[maxLength] = '\0';
    std::fprintf(stderr, "[GLShader] compile error:\n%s\n", errorLog.get());


    is_compiled_ = false;
    return false;
  }

  is_compiled_ = true;
  return true;
}

bool mam::GLShader::is_compiled() const
{
  return is_compiled_;
}

mam::Shader::Type mam::GLShader::type() const
{
  return type_;
}

uint32_t mam::GLShader::id() const
{
  return id_;
}
