#include <iostream>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

#include "RenderUtil.h"
#include "Voxel.h"
#include "VoxelEditor.h"
#include "ModelLoading.h"



using namespace std;

const int firstByte = 0xFF;
const int secondByte = 0xFF00;
const int thirdByte = 0xFF0000;
const int fourthByte = 0xFF000000;

ModelData::ModelData(Brush *brush, BoundingBox bb){
	_brush = *brush;
	_bBox = bb;
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

inline void setColor(unsigned int c, Voxel tv){
	tv.color[0] = (c >> 24) & firstByte;//r
	tv.color[1] = (c >> 16) & firstByte;//b
	tv.color[2] = (c >> 8) & firstByte;//g
	tv.color[3] = c & firstByte;//a
}

ModelData *ModelLoader::loadQuibicalBinary(string file){
	int uiBuff, checker;
	char version[4];
	unsigned int colorFormat = 0, zAxisOrientation = 0, compression = 0, vMaskEncoded = 0, numMatrices = 0;
	const int CODEFLAG = 2;
	const int NEXTSLICEFLAG = 6;
	int fCheck[6];
	char ui[4];
	FILE *in;
	//int fd;

	//fd = open(file.c_str(), O_RDONLY);

	fopen_s(&in, file.c_str(), "r");
	if (in == NULL){
		printf("Failed to open .qb file.\n");
		cin >> uiBuff;
		exit(1);
	}

	fread(version, sizeof(char), 4, in);
	printf("version check: %d.%d.%d.%d\n", version[0], version[1], version[2], version[3]);
	fread(&colorFormat, sizeof(unsigned int), 1, in);
	fread(&zAxisOrientation, sizeof(unsigned int), 1, in);
	fread(&compression, sizeof(unsigned int), 1, in);
	fread(&vMaskEncoded, sizeof(unsigned int), 1, in);
	fread(&numMatrices, sizeof(unsigned int), 1, in);
	printf("colorFormat: %u\nzAxisOrientation: %u\ncompression: %u\nvMaskEncoded: %u\nnumMatrices: %u\n", colorFormat, zAxisOrientation, compression, vMaskEncoded, numMatrices);


	int nameLength = 0;
	char *nBuff;
	unsigned int nLength = 0, sizeX = 0, sizeY = 0, sizeZ = 0;
	int posX = 0, posY = 0, posZ = 0;
	unsigned int *matrix;
	vector <unsigned int *> matrixList;
	unsigned int index, count = 0, data = 0;
	for (unsigned int i = 0; i < numMatrices; i++){
		fread(&nLength, sizeof(char), 1, in);
		nBuff = new char[nLength];
		fread(nBuff, sizeof(char), nLength, in);
		printf("Model name Length: %u\nModel name: %s\n", nLength, &nBuff);
		
		fread(&sizeX, sizeof(unsigned int), 1, in);
		fread(&sizeY, sizeof(unsigned int), 1, in);
		fread(&sizeZ, sizeof(unsigned int), 1, in);
		
		fread(&posX, sizeof(int), 1, in);
		fread(&posY, sizeof(int), 1, in);
		fread(&posZ, sizeof(int), 1, in);

		printf("Size: <%u,%u,%u>\nPosition: <%d,%d,%d>\n", sizeX, sizeY, sizeZ, posX, posY, posZ);

		matrix = new unsigned int[(int)sizeX*(int)sizeY*(int)sizeZ];
		matrixList.push_back(matrix);

		if (!compression){
			for (unsigned int z = 0; z < sizeZ; z++){
				for (unsigned int y = 0; y < sizeY; y++){
					for (unsigned int x = 0; x < sizeX; x++){
						fread(&matrix[z*sizeX*sizeY + y*sizeX + x], sizeof(unsigned int), 1, in);
					}
				}
			}
		}
		else{
			unsigned int x = 0, y = 0, z = 0;
			
			while (z < (int)sizeZ){
				z++;
				index = 0;

				while (1){
					fread(&data, sizeof(unsigned int), 1, in);
					//printf("Data: %u\n", data);
					if (data == NEXTSLICEFLAG){
						printf("Next slice hit\n");
						break;
					}
					else if (data == CODEFLAG){
						fread(&count, sizeof(unsigned int), 1, in);
						fread(&data, sizeof(unsigned int), 1, in);
						//printf("Count: %u\n", count);
						
						for (unsigned int j = 0; j < count; j++){
							x = index % sizeX + 1;
							y = index / sizeX + 1;
							index++;
							/*if (x >= sizeX || y >= sizeY || z >= sizeZ){
								printf("Out of bounds: <%d,%d,%d>\n", x,y,z);
							}*/
							/*if (x >= sizeX){
								printf("X:%d out of bounds, sizeX=%u\n", x, sizeX);
								cin >> checker;
							}
							if (y >= sizeY){
								printf("Y:%d out of bounds, sizeY=%u\n", y, sizeY);
								cin >> checker;
							}
							if (z >= sizeZ){
								printf("Z:%d out of bounds, sizeZ=%u\n", z, sizeZ);
								cin >> checker;
							}*/
							if (z*sizeX*sizeY + y*sizeX + x >= sizeX*sizeY*sizeZ){
								printf("Attempt to access matrix element at index: %d, <%u,%u,%u>\n", (int)(z*sizeX*sizeY + y*sizeX + x), x, y, z);
								cin >> checker;
							}
							matrix[z*sizeX*sizeY + y*sizeX + x] = data;
						}
					}
					else{
						x = index % sizeX + 1;
						y = index / sizeX + 1;
						index++;
						matrix[z*sizeX*sizeY + y*sizeX + x] = data;
					}
				}
			}
		}
		if (colorFormat){//changes format from BGRA to RGBA
			unsigned int tempContainer = 0;
			for (unsigned int i = 0; i < sizeX*sizeY*sizeZ; i++){
				tempContainer = (matrix[i] >> 16) & secondByte;//assigns B to the second byte
				tempContainer = (tempContainer & secondByte) | ((matrix[i] << 16) & fourthByte);//assigns R to the fourth byte and maintains B
				tempContainer = (tempContainer & (secondByte | fourthByte)) | (matrix[i] & (firstByte | thirdByte));//assigns G and A while maintaining R and B
				matrix[i] = tempContainer;
			}
		}
	}

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

	for (unsigned int z = 0; z < sizeZ; z++){
		for (unsigned int y = 0; y < sizeY; y++){
			for (unsigned int x = 0; x < sizeX; x++){
				setColor(matrix[z*sizeX*sizeY + y*sizeX + x], tv);
				tb->voxels[z*sizeX*sizeY + y*sizeX + x] = tv;
				printf("model color <%d,%d,%d,%d>\n", tv.color[0], tv.color[1], tv.color[2], tv.color[3]);
			}
		}
	}
	printf("Model Loaded\n");

	//_voxelEditor->setBrush(tb);

	return NULL;
}