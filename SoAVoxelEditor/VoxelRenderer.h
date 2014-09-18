#pragma once
#include "GlobalStructs.h"

using namespace std;


//Class that handles the graphics and rendering for voxels
struct Voxel;
class VoxelRenderer
{
public:
    static void initialize(int w, int h, int l);
    static void drawVoxels(class Camera* camera);
	static void drawChunks(class Camera* camera);
    static void selectVoxel(int x, int y, int z, bool selected);
	static void remesh(Voxel *voxels, int width, int height, int length, vector<int> chunks);

private:
	static int _cWidth, _cHeight;
    static bool _changed;
    static vector <BlockVertex> _currentVerts;
    static GLuint *_currentIndices;
    static BlockMesh _baseMesh;
    static Mesh _mesh;
	static vector <vector<BlockVertex>> _chunkVerts;
	static GLuint *_chunkIndices;
	static vector<Mesh> _chunkMeshs;
	static vector<bool> _chunkChanged;
};