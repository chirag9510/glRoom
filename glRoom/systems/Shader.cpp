#include "Shader.h"

#include <GL/glew.h>
#include <fstream>
#include <sstream>
#include <string>
#include <spdlog/spdlog.h>
#include <glm/gtc/type_ptr.hpp>

//DIRECTIONAL LIGHT SHADER
RenderShader::RenderShader(const char* szVSPath, const char* szFSPath) :
    Shader(szVSPath, szFSPath)
{
    uniLocPass = glGetUniformLocation(programID, "Pass");

    subBasic = glGetSubroutineIndex(programID, GL_FRAGMENT_SHADER, "basicKd");
    subEmissive = glGetSubroutineIndex(programID, GL_FRAGMENT_SHADER, "emissive");
    subBasicEmissive = glGetSubroutineIndex(programID, GL_FRAGMENT_SHADER, "basicEmissive");
    subTextured = glGetSubroutineIndex(programID, GL_FRAGMENT_SHADER, "textured");
    subBrightPass = glGetSubroutineIndex(programID, GL_FRAGMENT_SHADER, "brightPass");
    subBlurVert = glGetSubroutineIndex(programID, GL_FRAGMENT_SHADER, "blurPassVertical");
    subBlurHor = glGetSubroutineIndex(programID, GL_FRAGMENT_SHADER, "blurPassHorizontal");
}
RenderShader::~RenderShader()
{
    glDeleteProgram(programID);
}


Shader::Shader(const char* szVSPath, const char* szFSPath) :
    programID(0)
{
    std::string strVertexCode, strFragmentCode;
    std::ifstream vShaderFile, fShaderFile;
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        vShaderFile.open(szVSPath);
        fShaderFile.open(szFSPath);
        std::stringstream vShaderStream, fShaderStream;
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        vShaderFile.close();
        fShaderFile.close();
        strVertexCode = vShaderStream.str();
        strFragmentCode = fShaderStream.str();
    }
    catch (std::ifstream::failure& e)
    {
       spdlog::error("Failed to read Shader file: " + std::string(e.what()));
    }

    //program
    unsigned int vshader = compileShader(ShaderType::VERTEX, strVertexCode.c_str());
    unsigned int fshader = compileShader(ShaderType::FRAGMENT, strFragmentCode.c_str());

    programID = glCreateProgram();
    glAttachShader(programID, vshader);
    glAttachShader(programID, fshader);
    glLinkProgram(programID);
    checkCompileErrors(programID, ShaderType::PROGRAM);

    glDeleteShader(vshader);
    glDeleteShader(fshader);
}

Shader::~Shader()
{
}

unsigned int Shader::compileShader(ShaderType type, const char* szShader)
{
    unsigned int shader;
    if(type == ShaderType::VERTEX)
        shader = glCreateShader(GL_VERTEX_SHADER);
    else
        shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(shader, 1, &szShader, NULL);
    glCompileShader(shader);
    checkCompileErrors(shader, type);
    return shader;
}

void Shader::checkCompileErrors(unsigned int shader, ShaderType type)
{
    int success;
    char infoLog[1024];
    if (type != ShaderType::PROGRAM)
    {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            if (type == ShaderType::VERTEX)
                spdlog::error("Compilation error of type: VERTEX");
            else
                spdlog::error("Compilation error of type: FRAGMENT");

            spdlog::error(infoLog);
        }
    }
    else
    {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            spdlog::error("Program linking error of type: PROGRAM");
            spdlog::error(infoLog);
        }
    }
}