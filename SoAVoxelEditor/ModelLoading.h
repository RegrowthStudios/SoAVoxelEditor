#include <glm\glm.hpp>

#include "GlobalStructs.h"

struct Voxel;
struct Brush;
class VoxeEditor;

struct BoundingBox{
	glm::vec3 c1, c2;
};

class ModelData{
public:
	ModelData(Brush *brush, BoundingBox bb, string f);
	Brush *getBrush(){ return &_brush; }
	BoundingBox getBoundingBox(){ return _bBox; }
	string getFileName(){ return _fileName; }

private:
	Brush _brush;
	BoundingBox _bBox;
	string _fileName;
};

class Model{
public:
	Model(string name, glm::vec3 pos, ModelData *mDat);

	string getName(){ return _name; }
	glm::vec3 getPos(){ return _pos; }
	ModelData *getModelData(){ return _mDat; }

private:
	string _name;
	glm::vec3 _pos;
	ModelData *_mDat;
};

class ModelLoader{
public:
	void initialize(VoxelEditor *ve);
	ModelData *loadQuibicalBinary(string file);

private:
	vector <ModelData*> _loadedModels;
	VoxelEditor *_voxelEditor;
};