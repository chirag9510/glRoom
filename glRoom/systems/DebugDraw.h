#pragma once
#include <LinearMath/btIDebugDraw.h>
#include <vector>
#include <GL/glew.h>

class DebugDraw : public btIDebugDraw
{
public:
	DebugDraw() : iDebugMode(0), vao(0), vbo(0)
	{
		vecDraw.reserve(100000);

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0));									//aPos
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));					//aColor
		glEnableVertexAttribArray(1);
	}

	~DebugDraw()
	{
		glDeleteVertexArrays(1, &vao);
	}

	void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override
	{
		//fill up the vector with location layout position = 0, color = 1
			//then draw them all at once in flushLines()
		vecDraw.emplace_back(from.x());
		vecDraw.emplace_back(from.y());
		vecDraw.emplace_back(from.z());
		vecDraw.emplace_back(color.x());
		vecDraw.emplace_back(color.y());
		vecDraw.emplace_back(color.z());
		vecDraw.emplace_back(to.x());
		vecDraw.emplace_back(to.y());
		vecDraw.emplace_back(to.z());
		vecDraw.emplace_back(color.x());
		vecDraw.emplace_back(color.y());
		vecDraw.emplace_back(color.z());

	}

	void flushLines() override
	{
		glNamedBufferData(vbo, vecDraw.size() * sizeof(float), &vecDraw[0], GL_STREAM_DRAW);

		glBindVertexArray(vao);
		for (int i = 0; i < vecDraw.size(); i += 2)
			glDrawArrays(GL_LINES, i, 2);
	}

	void clearLines() override
	{
		//clear vector so the lines can be drawn again next frame
		vecDraw.clear();
	}

	void setDebugMode(int debugMode) override
	{
		this->iDebugMode = debugMode;
	}
	int getDebugMode() const override
	{
		return iDebugMode;
	}

	void reportErrorWarning(const char* warningString) override {}

	void draw3dText(const btVector3& location, const char* textString) override {}

	void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color)
	{}

private:
	int iDebugMode;
	std::vector<float> vecDraw;
	unsigned int vbo, vao;
};