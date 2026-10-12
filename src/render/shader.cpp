#include "shader.h"
#include <cstdio>
#include <vector>

static GLuint CompileStage(GLenum a_stage, const char* a_src) {
    GLuint id = glCreateShader(a_stage);
    glShaderSource(id, 1, &a_src, nullptr);
    glCompileShader(id);
    GLint ok = GL_FALSE;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len + 1);
        glGetShaderInfoLog(id, len, nullptr, log.data());
        std::fprintf(stderr, "Shader compile error: %s\n", log.data());
        glDeleteShader(id);
        return 0;
    }
    return id;
}

bool Shader::Compile(const char* a_vertexSrc, const char* a_fragmentSrc) {
    GLuint vs = CompileStage(GL_VERTEX_SHADER, a_vertexSrc);
    GLuint fs = CompileStage(GL_FRAGMENT_SHADER, a_fragmentSrc);
    if (!vs || !fs) return false;

    m_program = glCreateProgram();
    glAttachShader(m_program, vs);
    glAttachShader(m_program, fs);
    glLinkProgram(m_program);

    GLint ok = GL_FALSE;
    glGetProgramiv(m_program, GL_LINK_STATUS, &ok);
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len + 1);
        glGetProgramInfoLog(m_program, len, nullptr, log.data());
        std::fprintf(stderr, "Shader link error: %s\n", log.data());
        return false;
    }
    return true;
}

void Shader::Use() const {
    glUseProgram(m_program);
}

GLint Shader::Uniform(const char* a_name) const {
    return glGetUniformLocation(m_program, a_name);
}

Shader::~Shader() {
    Destroy();
}

void Shader::Destroy() {
    if (m_program) glDeleteProgram(m_program);
    m_program = 0;
}
