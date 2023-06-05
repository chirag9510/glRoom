#pragma once
#include "Components.h"
#include <glm/mat4x4.hpp>

enum class ShaderType
{
    VERTEX,
    FRAGMENT,
    PROGRAM
};

//basic shader
class Shader
{
public:
    Shader(const char* szVSPath, const char* szFSPath);
    ~Shader();
    unsigned int programID;
private:
    unsigned int compileShader(ShaderType type, const char* szShader);
    void checkCompileErrors(unsigned int shader, ShaderType type);
};


//Render pass shader
class RenderShader : public Shader
{
public:
    RenderShader(const char* szVSPath, const char* szFSPath);
    ~RenderShader();
    unsigned int uniLocPass;

    //subroutines
    unsigned int subBasic;
    unsigned int subTextured;
    unsigned int subEmissive;
    unsigned int subBasicEmissive;
    unsigned int subBrightPass;
    unsigned int subBlurVert;
    unsigned int subBlurHor;
};
