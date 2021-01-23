#include "utils.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <string_view>
#include <vector>

void check_compile_errors(std::string_view file, GLuint shader, std::string_view type);

GLuint load_shader(std::string_view vert, std::string_view frag)
{
    const GLuint program = glCreateProgram();
    const GLuint vert_shader = glCreateShader(GL_VERTEX_SHADER);
    const GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER);

    std::string vert_string = read_file(vert);
    std::string frag_string = read_file(frag);

    const char *vert_source = vert_string.c_str();
    const char *frag_source = frag_string.c_str();

    glShaderSource(vert_shader, 1, &vert_source, nullptr);
    glCompileShader(vert_shader);
    check_compile_errors(vert, vert_shader, "VERTEX");

    glShaderSource(frag_shader, 1, &frag_source, nullptr);
    glCompileShader(frag_shader);
    check_compile_errors(frag, frag_shader, "FRAGMENT");

    glAttachShader(program, vert_shader);
    glAttachShader(program, frag_shader);

    glLinkProgram(program);
    glValidateProgram(program);
    check_compile_errors(vert, program, "PROGRAM");

    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);

    return program;
}

std::string read_file(std::string_view file)
{
    std::ifstream f = std::ifstream(file.data());
    std::string line;
    std::string text;

    assert(f.is_open());
    while (std::getline(f, line))
        text += line + "\n";

    return text;
}

void check_compile_errors(std::string_view file, GLuint shader, std::string_view type)
{
    GLint success;
    std::vector<GLchar> infoLog(2048);
    if (type != "PROGRAM")
    {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            GLint maxLength = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
            if (maxLength > 0)
                infoLog.resize(maxLength);
            glGetShaderInfoLog(shader, (GLsizei)infoLog.size(), nullptr, infoLog.data());
            std::cout << "Error compiling shader: " << file << std::endl;
            std::cout << "shader compilation error of type: " << type.data() << std::endl
                      << infoLog.data() << std::endl;
        }
    }
    else
    {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success)
        {
            GLint maxLength = 0;
            glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
            if (maxLength > 0)
                infoLog.resize(maxLength);
            glGetProgramInfoLog(shader, (GLsizei)infoLog.size(), nullptr, infoLog.data());
            std::cout << "Error compiling shader: " << file << std::endl;
            std::cout << "shader compilation error of type: " << type.data() << std::endl
                      << infoLog.data() << std::endl;
        }
    }
}

void error_callback(int code, const char *message)
{
    std::cout << "GLFW Error: " << code << " :: " << message << std::endl;
}

void GLAPIENTRY message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                                 const GLchar *message, const void *userParam)
{
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
            (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity, message);
}