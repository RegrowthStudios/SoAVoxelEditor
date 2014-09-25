#pragma once
#include <Windows.h>
#include <GL\glew.h>
#include <glm\glm.hpp>

#include "GlobalStructs.h"

struct Voxel;
struct Brush;

// Static class that provides various helpful rendering utilities
class RenderUtil
{
public:
    static bool checkGlError();
    static void drawLine(class Camera *camera, glm::vec3 p1, glm::vec3 p2, GLubyte r, GLubyte g, GLubyte b, int thickness);
    static void uploadMesh(GLuint* vboID, GLuint* iboID, const BlockVertex* blockVertices, const int numVertices, const GLuint* indices, const int numIndices);
    static void uploadMesh(GLuint* vboID, GLuint* iboID, const glm::vec3* vertices, const int numVertices, const GLuint* indices, const int numIndices);
    static void initializeWireframeBox();
    static void drawWireframeBox(class Camera* camera, const glm::vec3& position, const glm::vec3& size, const glm::vec4& color);
    static void releaseWireframeBox();
	static void initializeReferenceVoxel();
	static void drawReferenceVoxel(class Camera* camera, glm::vec3 position, Brush *brush);
	static void changeReferenceColor(glm::vec4 color);
	static void meshBrush(Brush *brush);
	static inline void addFace(int start, int end, glm::vec3 pos);

private:
	static Mesh *_mesh, *_referenceCubeMesh;
	static BlockMesh _voxVerts, _voxBaseVerts;
	static GLuint *_referenceCubeIndices, *_brushIndices;
	static glm::vec3 _lastPosition;
	static vector <BlockVertex> _brushVerts;
};

