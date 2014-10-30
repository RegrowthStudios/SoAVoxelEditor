#include <iostream>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>

#include "RenderUtil.h"
#include "Voxel.h"
#include "VoxelEditor.h"
#include "ModelLoading.h"



using namespace std;

const int firstByte = 0xFF;
const int secondByte = 0xFF00;
const int thirdByte = 0xFF0000;
const int fourthByte = 0xFF000000;

ModelData::ModelData(Brush *brush, BoundingBox bb, string f){
	_brush = *brush;
	_bBox = bb;
	_fileName = f;
	//create the model mesh and calculate the bounding box
}

Model::Model(string name, glm::vec3 pos, ModelData *mDat){
	_name = name;
	_pos = pos;
	_mDat = mDat;
}

void ModelLoader::initialize(VoxelEditor *ve){
	_voxelEditor = ve;
}

inline Voxel setColor(UINT32 c, Voxel tv){
	tv.color[0] = ((c >> 24) & firstByte);//r
	tv.color[1] = ((c >> 16) & firstByte);//b
	tv.color[2] = ((c >> 8) & firstByte);//g
	tv.color[3] = (c & firstByte);//a
	if (tv.color[3] == 0)
		tv.type = '\0';
	else{
		tv.color[3] = 255;
		tv.type = 'm';
	}
	return tv;
}

ModelData *ModelLoader::loadQuibicalBinary(string file){
	for (int i = 0; i < (int)_loadedModels.size(); i++){
		if (file == _loadedModels[i]->getFileName()){
			_voxelEditor->setBrush(_loadedModels[i]->getBrush());
			printf("File %s already in memory\n", file);
			return NULL;
		}
	}

	int uiBuff;
	char version[4], ext;
	UINT32 colorFormat = 0, zAxisOrientation = 0, compression = 0, vMaskEncoded = 0, numMatrices = 0;
	const int CODEFLAG = 2;
	const int NEXTSLICEFLAG = 6;
	FILE *in;

	fopen_s(&in, file.c_str(), "rb");
	if (in == NULL){
		printf("Failed to open .qb file.\n");
		cin >> uiBuff;
		exit(1);
	}

	fread(version, sizeof(char), 4, in);
	printf("version check: %d.%d.%d.%d\n", version[0], version[1], version[2], version[3]);
	fread(&colorFormat, sizeof(UINT32), 1, in);
	fread(&zAxisOrientation, sizeof(UINT32), 1, in);
	fread(&compression, sizeof(UINT32), 1, in);
	fread(&vMaskEncoded, sizeof(UINT32), 1, in);
	fread(&numMatrices, sizeof(UINT32), 1, in);
	printf("colorFormat: %u\nzAxisOrientation: %u\ncompression: %u\nvMaskEncoded: %u\nnumMatrices: %u\n", colorFormat, zAxisOrientation, compression, vMaskEncoded, numMatrices);


	int nameLength = 0;
	char *nBuff;
	int nLength = 0, sizeX = 0, sizeY = 0, sizeZ = 0;
	int posX = 0, posY = 0, posZ = 0;
	UINT32 *matrix;
	vector <UINT32 *> matrixList;
	UINT32 index, count = 0, data = 0;
	numMatrices = 1;

	for (UINT32 i = 0; i < numMatrices; i++){
		fread(&nLength, sizeof(char), 1, in);
		nBuff = new char[nLength + 1];
		nBuff[nLength] = '\0';
		fread(nBuff, sizeof(char), nLength, in);
		printf("Model name Length: %u\nModel name: %s\n", nLength, nBuff);
		
		fread(&sizeX, sizeof(UINT32), 1, in);
		fread(&sizeY, sizeof(UINT32), 1, in);
		fread(&sizeZ, sizeof(UINT32), 1, in);
		
		fread(&posX, sizeof(UINT32), 1, in);
		fread(&posY, sizeof(UINT32), 1, in);
		fread(&posZ, sizeof(UINT32), 1, in);

		printf("Size: <%u,%u,%u>\nPosition: <%d,%d,%d>\n", sizeX, sizeY, sizeZ, posX, posY, posZ);

		matrix = new UINT32[(int)sizeX*(int)sizeY*(int)sizeZ];
		for (int j = 0; j < sizeX*sizeY*sizeZ; j++){
			matrix[j] = 0;
		}
		matrixList.push_back(matrix);

		if (!compression){
			for (int z = 0; z < (int)sizeZ; z++){
				for (int y = 0; y < (int)sizeY; y++){
					for (int x = 0; x < (int)sizeX; x++){
						fread(&matrix[z*sizeX*sizeY + y*sizeX + x], sizeof(UINT32), 1, in);
					}
				}
			}
		}
		else{
			int x = 0, y = 0, z = 0;
			
			while (z < (int)sizeZ){
				z++;
				index = 0;

				while (1){
					fread(&data, sizeof(UINT32), 1, in);
					if (data == NEXTSLICEFLAG){
						printf("Next slice hit\n");
						break;
					}
					else if (data == CODEFLAG){
						fread(&count, sizeof(UINT32), 1, in);
						fread(&data, sizeof(UINT32), 1, in);
						
						for (int j = 0; j < (int)count; j++){
							x = index % sizeX;
							y = index / sizeX;
							index++;
							if (z*sizeX*sizeY + y*sizeX + x >= sizeX*sizeY*sizeZ){
								printf("Attempt to access matrix element at index: %d, <%u,%u,%u>\n", (int)(z*sizeX*sizeY + y*sizeX + x), x, y, z);
								//cin >> checker;
								break;
							}
							matrix[z*sizeX*sizeY + y*sizeX + x] = data;
						}
					}
					else{
						x = index % sizeX;
						y = index / sizeX;
						index++;
						if (z*sizeX*sizeY + y*sizeX + x >= sizeX*sizeY*sizeZ){
							printf("Attempt to access matrix element at index: %d, <%u,%u,%u>\n", (int)(z*sizeX*sizeY + y*sizeX + x), x, y, z);
							//cin >> checker;
							break;
						}
						matrix[z*sizeX*sizeY + y*sizeX + x] = data;
					}
				}
			}
		}
		if (colorFormat){//changes format from BGRA to RGBA
			UINT32 tempContainer = 0;
			for (int i = 0; i < (int)(sizeX*sizeY*sizeZ); i++){
				tempContainer = (matrix[i] >> 16) & secondByte;//assigns B to the second byte
				tempContainer = (tempContainer & secondByte) | ((matrix[i] << 16) & fourthByte);//assigns R to the fourth byte and maintains B
				tempContainer = (tempContainer & (secondByte | fourthByte)) | (matrix[i] & (firstByte | thirdByte));//assigns G and A while maintaining R and B
				matrix[i] = tempContainer;
			}
		}
	}
	int iterator = 0;
	while (fread(&ext, sizeof(char), 1, in) > 0){
		printf("%c", ext);
		if (iterator > 100)
			break;
		iterator++;
	}
	cout << endl;

	fclose(in);

	BoundingBox bb;
	bb.c1 = glm::vec3(abs(posX), abs(posY), abs(posZ));
	bb.c2 = glm::vec3(abs(posX) + sizeX, abs(posY) + sizeY, abs(posZ) + sizeY);
	Brush *tb;
	tb = new Brush();
	tb->width = sizeX;
	tb->height = sizeY;
	tb->length = sizeZ;
	Voxel tv;
	tv.type = 'm';
	tb->voxels.resize(sizeX*sizeY*sizeZ);

	for (int z = 0; z < sizeZ; z++){
		for (int y = 0; y < sizeY; y++){
			for (int x = 0; x < sizeX; x++){
				tv = setColor(matrix[z*sizeX*sizeY + y*sizeX + x], tv);
				tb->voxels[z*sizeX*sizeY + y*sizeX + x] = tv;
				/*if (x < 5)
					printf("model color: <%d,%d,%d,%d>, type: %c\n", tv.color[0], tv.color[1], tv.color[2], tv.color[3], tv.type);*/
			}
		}
	}
	for (int i = 0; i < 10; i++){
		printf("Voxel %d color: <%d,%d,%d,%d> type: %c\n", i, tb->voxels[i].color[0], tb->voxels[i].color[1], tb->voxels[i].color[2], tb->voxels[i].color[3], tb->voxels[i].type);
	}
	printf("Model Loaded\n");
	_loadedModels.push_back(new ModelData(tb, bb, file));

	_voxelEditor->setBrush(tb);

	return NULL;
}