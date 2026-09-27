#pragma once
#include <GL/glew.h>
#include <string>

class Shader {
public:
    bool Compile(const char* a_vertexSrc, const char* a_fragmentSrc);
    void Use() const;
    GLint Uniform(const char* a_name) const;
    ~Shader();

private:
    GLuint m_program = 0;
};
