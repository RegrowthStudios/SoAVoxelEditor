#pragma once
#include <GL\glew.h>
#include <string>
#include <vector>

using namespace std;

//Describes a voxel, for flyweight programming pattern
struct VoxelType {
    unsigned short id; // This value should only be changed by the VoxelTypeRegistry
    string name;
};

//Grid voxel
struct Voxel {
    unsigned short type;
	GLubyte color[4];
};

struct Brush{
	int height, width, length;
	vector <Voxel> voxels;
};