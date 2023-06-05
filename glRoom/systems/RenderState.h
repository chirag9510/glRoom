#pragma once
#include <GL/glew.h>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <vector>
#include <string>

typedef struct
{
	GLuint count;
	GLuint instanceCount;
	GLuint firstIndex;
	GLuint baseVertex;
	GLuint baseInstance;
}DrawElementsIndirectCommand;

enum class RSType
{
	BASIC_KD,								//mesh without texture basic color using Kd
	TEXTURED,
	EMISSIVE
};

struct RenderState
{
	GLuint primCount;
	GLuint vao;
	GLuint ebo;
	GLuint ssboFrag;										//ssbo for bindless textures OR normal mesh colors for fragment shader
	void* drawCmdOffset;
	RenderState() : vao(0), ssboFrag(0), ebo(0), drawCmdOffset(0), primCount(0) {}
};

struct RSLoader
{
	RSType rsType;
	std::vector<DrawElementsIndirectCommand> vcDrawCmd;
	std::vector<float> vertices;
	std::vector<unsigned int> indices;
	std::vector<std::string> vcTexNames;												//access the textures map using these keys
	std::vector<glm::vec4> vcMeshKdColors;												//basic Kd colors, only used by BASIC_KD
	GLuint baseVertex;
	GLuint firstIndex;
	GLuint drawID;																	
	RSLoader() : baseVertex(0), firstIndex(0), drawID(0) {}
	RSLoader(RSType rsType) : rsType(rsType),  baseVertex(0), firstIndex(0) , drawID(0){}
};

//used by stencil testing models like room model / foreground quad, direct draw 
struct GeometryState
{
	GLuint vao;
	GLuint ebo;
	GLuint count;
	GLuint ssboFrag;											//texture / color
	GeometryState() : vao(0), ebo(0), count(0), ssboFrag(0) {}
};

struct VertexTupple
{
	//represents the indexes stored in obj file vertex/normal/texcoords
	//used in unordered map to avoid duplication of data in buffer
	unsigned int v, n, t;
	VertexTupple() : v(0), n(0), t(0) {}
	VertexTupple(unsigned int v = 0, unsigned int n = 0, unsigned int t = 0) : v(v), n(n), t(t) {}

	bool operator==(const VertexTupple& other) const
	{
		return (v == other.v && n == other.n && t == other.t);
	}
};

struct VertexTuppleHash
{
	size_t operator()(const VertexTupple& tupple) const
	{
		return size_t(tupple.v * 100 + tupple.n * 10 + tupple.t);
	}
};
