#include "render/opengl/gl_pipeline.hpp"

#include "render/api/shader.hpp"
#include "render/opengl/gl_shader.hpp"


namespace mam
{
  static UniformType ToUniformType(GLenum type) {
    switch (type)
    {
    case GL_FLOAT:      return UniformType::Float1;
    case GL_FLOAT_VEC2: return UniformType::Float2;
    case GL_FLOAT_VEC3: return UniformType::Float3;
    case GL_FLOAT_VEC4: return UniformType::Float4;

    case GL_INT:      return UniformType::Int1;
    case GL_INT_VEC2: return UniformType::Int2;
    case GL_INT_VEC3: return UniformType::Int3;
    case GL_INT_VEC4: return UniformType::Int4;

    case GL_UNSIGNED_INT:      return UniformType::UInt1;
    case GL_UNSIGNED_INT_VEC2: return UniformType::UInt2;
    case GL_UNSIGNED_INT_VEC3: return UniformType::UInt3;
    case GL_UNSIGNED_INT_VEC4: return UniformType::UInt4;

    case GL_BOOL: return UniformType::Bool;

    case GL_FLOAT_MAT2: return UniformType::Mat2;
    case GL_FLOAT_MAT3: return UniformType::Mat3;
    case GL_FLOAT_MAT4: return UniformType::Mat4;
    }

    return UniformType::Float1;
  }

  GLPipeline::GLPipeline() {
    id_ = glCreateProgram();
  }

  GLPipeline::~GLPipeline() {
    if (glIsProgram(id_))
    {
      glDeleteProgram(id_);
    }
  }

  void GLPipeline::addShader(Shader* shader) {
    if (shader)
    {
      glAttachShader(id_, shader->id());
    }
  }

  std::optional<std::string> GLPipeline::create() {
    glLinkProgram(id_);
    GLint program_linked = 0;
    GLsizei log_length = 0;

    glGetProgramiv(id_, GL_LINK_STATUS, &program_linked);
    glGetProgramiv(id_, GL_INFO_LOG_LENGTH, &log_length);

    if (program_linked != GL_TRUE) {
      std::string errorLog(static_cast<size_t>(log_length) + 1, '\0');
      GLsizei ll = 0;
      glGetProgramInfoLog(id_, log_length, &ll, errorLog.data());
      errorLog.resize(static_cast<size_t>(ll));
      return errorLog;
    }

    GLint count = 0;
    glGetProgramiv(id_, GL_ACTIVE_UNIFORMS, &count);

    char name[256];
    for (GLint i = 0; i < count; ++i) {
      GLsizei length;
      GLint size;
      GLenum type;

      glGetActiveUniform(id_, i, sizeof(name), &length, &size, &type, name);
      GLint loc = glGetUniformLocation(id_, name);
      uniforms[name] = { loc, ToUniformType(type) };
    }

    return std::nullopt;
  }

  void GLPipeline::use()  {
    glUseProgram(id_);
  }

  int GLPipeline::getUniformLocation(const char* name) const {
    auto it = uniforms.find(name);
    if (it != uniforms.end())
      return it->second.location;
    
    return glGetUniformLocation(id_, name);
  }

  void GLPipeline::setUniform(const char* name, const void* value) {
    auto it = uniforms.find(name);
    if (it == uniforms.end())
      return;

    GLint loc = getUniformLocation(name);
    if (loc == -1)
      return;

    const GLUniform& u = it->second;

    switch (u.type)
    {
    case UniformType::Double1:
    case UniformType::Float1: glProgramUniform1fv(id_, loc, 1, static_cast<const GLfloat*>(value)); break;
    case UniformType::Double2:
    case UniformType::Float2: glProgramUniform2fv(id_, loc, 1, static_cast<const GLfloat*>(value)); break;
    case UniformType::Double3:
    case UniformType::Float3: glProgramUniform3fv(id_, loc, 1, static_cast<const GLfloat*>(value)); break;
    case UniformType::Double4:
    case UniformType::Float4: glProgramUniform4fv(id_, loc, 1, static_cast<const GLfloat*>(value)); break;

    case UniformType::Byte1:
    case UniformType::Short1:
    case UniformType::Int1: glProgramUniform1iv(id_, loc, 1, static_cast<const GLint*>(value)); break;
    case UniformType::Byte2:
    case UniformType::Short2:
    case UniformType::Int2: glProgramUniform2iv(id_, loc, 1, static_cast<const GLint*>(value)); break;
    case UniformType::Byte3:
    case UniformType::Short3:
    case UniformType::Int3: glProgramUniform3iv(id_, loc, 1, static_cast<const GLint*>(value)); break;
    case UniformType::Byte4:
    case UniformType::Short4:
    case UniformType::Int4: glProgramUniform4iv(id_, loc, 1, static_cast<const GLint*>(value)); break;

    case UniformType::UByte1:
    case UniformType::UShort1:
    case UniformType::UInt1: glProgramUniform1uiv(id_, loc, 1, static_cast<const GLuint*>(value)); break;
    case UniformType::UByte2:
    case UniformType::UShort2:
    case UniformType::UInt2: glProgramUniform2uiv(id_, loc, 1, static_cast<const GLuint*>(value)); break;
    case UniformType::UByte3:
    case UniformType::UShort3:
    case UniformType::UInt3: glProgramUniform3uiv(id_, loc, 1, static_cast<const GLuint*>(value)); break;
    case UniformType::UByte4:
    case UniformType::UShort4:
    case UniformType::UInt4: glProgramUniform4uiv(id_, loc, 1, static_cast<const GLuint*>(value)); break;

    case UniformType::Mat2: glProgramUniformMatrix2fv(id_, loc, 1, GL_FALSE, static_cast<const GLfloat*>(value)); break;
    case UniformType::Mat3: glProgramUniformMatrix3fv(id_, loc, 1, GL_FALSE, static_cast<const GLfloat*>(value)); break;
    case UniformType::Mat4: glProgramUniformMatrix4fv(id_, loc, 1, GL_FALSE, static_cast<const GLfloat*>(value)); break;

    case UniformType::Bool: glProgramUniform1iv(id_, loc, 1, static_cast<const GLint*>(value)); break;

    default: break;
    }
  }

  void GLPipeline::setSamplerUniform(const char* name, int unit)
  {
    GLint loc = getUniformLocation(name);
    if (loc != -1) {
      glProgramUniform1i(id_, loc, unit);
    }
  }

  u32 GLPipeline::id() const {
    return static_cast<u32>(id_);
  }
}