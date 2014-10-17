#include "VoxelRenderer.h"
#include "Shader.h"
#include "Camera.h"
#include "RenderUtil.h"
#include "TextureManager.h"
#include "Voxel.h"
#include "TextureManager.h"
#include "VoxelGrid.h"
#include <iostream>

bool VoxelRenderer::_changed;
vector <BlockVertex> VoxelRenderer::_currentVerts;
GLuint *VoxelRenderer::_currentIndices;
BlockMesh VoxelRenderer::_baseMesh;
Mesh VoxelRenderer::_mesh;
vector <vector<BlockVertex>> VoxelRenderer::_chunkVerts;
GLuint *VoxelRenderer::_chunkIndices;
vector<Mesh> VoxelRenderer::_chunkMeshs;
vector<bool> VoxelRenderer::_chunkChanged;
int VoxelRenderer::_cWidth;
int VoxelRenderer::_cHeight;

void VoxelRenderer::initialize(int w, int h, int l) {
    _changed = true;

    int cubeTot = w*h*l;

    _currentVerts.reserve(24 * cubeTot);
    _currentIndices = new GLuint[cubeTot * 36];

	int xc, yc, zc;
	xc = w / 32;
	if (w % 32 != 0)
		xc++;
	yc = h / 32;
	if (h % 32 != 0)
		yc++;
	zc = l / 32;
	if (h % 32 != 0)
		zc++;
	_cWidth = xc;
	_cHeight = yc;
	_chunkIndices = new GLuint[32 * 32 * 32 * 36];
	_chunkMeshs.resize(xc * yc * zc);
	_chunkChanged.resize(xc * yc * zc, 0);
	_chunkVerts.resize(xc * yc * zc);
	/*for (int i = 0; i < _chunkVerts.size(); i++){
		_chunkVerts[i].reserve(24 * 32 * 32 * 32);
	}*/

	for (int i = 0, j = 0; i < 32 * 32 * 32 * 36; i += 6, j += 4){
		_chunkIndices[i] = j;
		_chunkIndices[i + 1] = j + 1;
		_chunkIndices[i + 2] = j + 2;
		_chunkIndices[i + 3] = j + 2;
		_chunkIndices[i + 4] = j + 3;
		_chunkIndices[i + 5] = j;
	}

    for (int i = 0, j = 0; i < w*h*l * 36; i += 6, j+=4){
        _currentIndices[i] = j;
        _currentIndices[i + 1] = j + 1;
        _currentIndices[i + 2] = j + 2;
        _currentIndices[i + 3] = j + 2;
        _currentIndices[i + 4] = j + 3;
        _currentIndices[i + 5] = j;
    }

    for (int i = 0; i < 24; i++){
        _baseMesh.verts[i].position.x = cubeVertices[i * 3];
        _baseMesh.verts[i].position.y = cubeVertices[i * 3 + 1];
        _baseMesh.verts[i].position.z = cubeVertices[i * 3 + 2];

        _baseMesh.verts[i].normal.x = cubeNormals[i * 3];
        _baseMesh.verts[i].normal.y = cubeNormals[i * 3 + 1];
        _baseMesh.verts[i].normal.z = cubeNormals[i * 3 + 2];

        _baseMesh.verts[i].color[0] = 255;
        _baseMesh.verts[i].color[1] = 0;
        _baseMesh.verts[i].color[2] = 0;
        _baseMesh.verts[i].color[3] = 255;
    }

	for (int i = 0; i < (int)_chunkMeshs.size(); i++){
		_chunkMeshs[i] = Mesh();
		_chunkMeshs[i].iboID = 0;
		_chunkMeshs[i].vboID = 0;
	}
    _mesh = Mesh();
    _mesh.iboID = 0;
    _mesh.vboID = 0;
}

void VoxelRenderer::drawVoxels(Camera *camera) {
    blockShader.bind();

    const glm::vec3 &position = camera->getPosition();

    glm::mat4 modelMatrix(1);
    modelMatrix[3][0] = -position.x;
    modelMatrix[3][1] = -position.y;
    modelMatrix[3][2] = -position.z;

    glm::mat4 MVP = camera->getProjectionMatrix() * camera->getViewMatrix() * modelMatrix;

    //send our uniform data, the matrix, the light position, and the texture data
    glUniformMatrix4fv(blockShader.mvpID, 1, GL_FALSE, &MVP[0][0]);

    glm::vec3 lightPos = position;
    lightPos = glm::normalize(lightPos);
    glUniform3f(blockShader.lightPosID, lightPos.x, lightPos.y, lightPos.z);

    const int numIndices = (6 * _currentVerts.size()) / 4;

    //initialize the buffer, only happens once
    if (_changed){
        RenderUtil::uploadMesh(&_mesh.vboID, &_mesh.iboID, &_currentVerts[0], _currentVerts.size(), _currentIndices, numIndices);
        _changed = false;
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, _mesh.vboID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _mesh.iboID);
    }

    //set our attribute pointers using our interleaved vertex data. Last parameter is offset into the vertex
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(BlockVertex), (void *)0); //vertexPosition
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(BlockVertex), (void *)12); //vertexColor
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(BlockVertex), (void *)16); //vertexNormal

    //Finally, draw our data. The last parameter is the offset into the bound buffer
    glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, NULL);

    blockShader.unBind();

    if (drawDebugLine){
        RenderUtil::drawLine(camera, debugP1, debugP2, 255, 0, 255, 5);
    }
}

void VoxelRenderer::drawChunks(Camera *camera) {
	blockShader.bind();

	const glm::vec3 &position = camera->getPosition();

	glm::mat4 modelMatrix(1);
	modelMatrix[3][0] = -position.x;
	modelMatrix[3][1] = -position.y;
	modelMatrix[3][2] = -position.z;

	glm::mat4 MVP = camera->getProjectionMatrix() * camera->getViewMatrix() * modelMatrix;

	//send our uniform data, the matrix, the light position, and the texture data
	glUniformMatrix4fv(blockShader.mvpID, 1, GL_FALSE, &MVP[0][0]);

	glm::vec3 lightPos = position;
	lightPos = glm::normalize(lightPos);
	glUniform3f(blockShader.lightPosID, lightPos.x, lightPos.y, lightPos.z);

	//const int numIndices = (6 * _currentVerts.size()) / 4;
	for (int i = 0; i < (int)_chunkMeshs.size(); i++){
		int numIndices = (6 * _chunkVerts[i].size()) / 4;
		//initialize the buffer, only happens once
		if (_chunkChanged[i]){
			RenderUtil::uploadMesh(&_chunkMeshs[i].vboID, &_chunkMeshs[i].iboID, &_chunkVerts[i][0], _chunkVerts[i].size(), _chunkIndices, numIndices);
			_chunkChanged[i] = false;
			//_changed = false;
		}
		else {
			glBindBuffer(GL_ARRAY_BUFFER, _chunkMeshs[i].vboID);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _chunkMeshs[i].iboID);
		}

		//set our attribute pointers using our interleaved vertex data. Last parameter is offset into the vertex
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(BlockVertex), (void *)0); //vertexPosition
		glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(BlockVertex), (void *)12); //vertexColor
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(BlockVertex), (void *)16); //vertexNormal

		//Finally, draw our data. The last parameter is the offset into the bound buffer
		glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, NULL);
	}
	blockShader.unBind();

	if (drawDebugLine){
		RenderUtil::drawLine(camera, debugP1, debugP2, 255, 0, 255, 5);
	}
}

void VoxelRenderer::selectVoxel(int x, int y, int z, bool selected) {
	for (int i = 0; i < (int)_currentVerts.size(); i++){
        if ((int)_currentVerts[i].position.x - _baseMesh.verts[i % 24].position.x == x && (int)_currentVerts[i].position.y - _baseMesh.verts[i % 24].position.y == y && (int)_currentVerts[i].position.z - _baseMesh.verts[i % 24].position.z == z){
            //_currentVerts[i].selected = selected;
            break;
        }
    }
}

void VoxelRenderer::remesh(Voxel *voxels, int width, int height, int length, vector<int> chunks){
	
	
	//_currentVerts.clear();
	for (int i = 0; i < (int)chunks.size(); i++){
		_chunkVerts[chunks[i]].clear();
	}
	BlockVertex tv;
	Voxel currentVox;

	/*int _cWidth, _cHeight;
	_cWidth = width / 32;
	if (width % 32 != 0)
		_cWidth++;
	_cHeight = height / 32;
	if (height % 32 != 0)
		_cHeight++;*/
	
	int layerSize = width * length;
	
	cout << chunks.size() << endl;
	for (int c = 0; c < (int)chunks.size(); c++){
		int cx, cy, cz;
		cz = chunks[c] / (_cWidth * _cHeight);
		cy = (chunks[c] % (_cWidth * _cHeight)) / _cWidth;
		cx = (chunks[c] % (_cWidth * _cHeight)) % _cWidth;
		printf("Remeshing chunk: %d at <%d,%d,%d>\n", chunks[c], cx, cy, cz);
		for (int z = cz*32; z < cz*32+32 && z < length; z++){
			for (int y = cy*32; y < cy*32+32 && y < height; y++){
				for (int x = cx*32; x < cx*32+32 && x < width; x++){
					currentVox = voxels[z*layerSize + y*width + x];
					if (currentVox.type != '/0' && currentVox.type != 0){
						if (z < length - 1){//front face
							if (voxels[(z + 1)*layerSize + y*width + x].type == '\0' || voxels[(z + 1)*layerSize + y*width + x].type == 0){
								for (int i = 0; i < 4; i++){
									tv = _baseMesh.verts[i];
									tv.position += glm::vec3(x, y, z);
									for (int j = 0; j < 4; j++)
										tv.color[j] = currentVox.color[j];
									_chunkVerts[cz*_cWidth*_cHeight + cy*_cWidth + cx].push_back(tv);
									//_currentVerts.push_back(tv);
								}
								_chunkChanged[cz*_cWidth*_cHeight + cy*_cWidth + cx] = true;
							}
						}
						else{
							for (int i = 0; i < 4; i++){
								tv = _baseMesh.verts[i];
								tv.position += glm::vec3(x, y, z);
								for (int j = 0; j < 4; j++)
									tv.color[j] = currentVox.color[j];
								_chunkVerts[cz*_cWidth*_cHeight + cy*_cWidth + cx].push_back(tv);
								//_currentVerts.push_back(tv);
							}
							_chunkChanged[cz*_cWidth*_cHeight + cy*_cWidth + cx] = true;
						}
						if (x < width - 1){//right face
							if (voxels[z*layerSize + y*width + x + 1].type == '\0' || voxels[z*layerSize + y*width + x + 1].type == '\0'){
								for (int i = 4; i < 8; i++){
									tv = _baseMesh.verts[i];
									tv.position += glm::vec3(x, y, z);
									for (int j = 0; j < 4; j++)
										tv.color[j] = currentVox.color[j];
									_chunkVerts[cz*_cWidth*_cHeight + cy*_cWidth + cx].push_back(tv);
									//_currentVerts.push_back(tv);
								}
								_chunkChanged[cz*_cWidth*_cHeight + cy*_cWidth + cx] = true;
							}
						}
						else{
							for (int i = 4; i < 8; i++){
								tv = _baseMesh.verts[i];
								tv.position += glm::vec3(x, y, z);
								for (int j = 0; j < 4; j++)
									tv.color[j] = currentVox.color[j];
								_chunkVerts[cz*_cWidth*_cHeight + cy*_cWidth + cx].push_back(tv);
								//_currentVerts.push_back(tv);
							}
							_chunkChanged[cz*_cWidth*_cHeight + cy*_cWidth + cx] = true;
						}
						if (y < width - 1){//top face
							if (voxels[z*layerSize + (y + 1)*width + x].type == '\0' || voxels[z*layerSize + (y + 1)*width + x].type == 0){
								for (int i = 8; i < 12; i++){
									tv = _baseMesh.verts[i];
									tv.position += glm::vec3(x, y, z);
									for (int j = 0; j < 4; j++)
										tv.color[j] = currentVox.color[j];
									_chunkVerts[cz*_cWidth*_cHeight + cy*_cWidth + cx].push_back(tv);
									//_currentVerts.push_back(tv);
								}
								_chunkChanged[cz*_cWidth*_cHeight + cy*_cWidth + cx] = true;
							}
						}
						else{
							for (int i = 8; i < 12; i++){
								tv = _baseMesh.verts[i];
								tv.position += glm::vec3(x, y, z);
								for (int j = 0; j < 4; j++)
									tv.color[j] = currentVox.color[j];
								_chunkVerts[cz*_cWidth*_cHeight + cy*_cWidth + cx].push_back(tv);
								//_currentVerts.push_back(tv);
							}
							_chunkChanged[cz*_cWidth*_cHeight + cy*_cWidth + cx] = true;
						}
						if (x > 0){//left face
							if (voxels[z*layerSize + y*width + x - 1].type == '\0' || voxels[z*layerSize + y*width + x - 1].type == 0){
								for (int i = 12; i < 16; i++){
									tv = _baseMesh.verts[i];
									tv.position += glm::vec3(x, y, z);
									for (int j = 0; j < 4; j++)
										tv.color[j] = currentVox.color[j];
									_chunkVerts[cz*_cWidth*_cHeight + cy*_cWidth + cx].push_back(tv);
									//_currentVerts.push_back(tv);
								}
								_chunkChanged[cz*_cWidth*_cHeight + cy*_cWidth + cx] = true;
							}
						}
						else{
							for (int i = 12; i < 16; i++){
								tv = _baseMesh.verts[i];
								tv.position += glm::vec3(x, y, z);
								for (int j = 0; j < 4; j++)
									tv.color[j] = currentVox.color[j];
								_chunkVerts[cz*_cWidth*_cHeight + cy*_cWidth + cx].push_back(tv);
								//_currentVerts.push_back(tv);
							}
							_chunkChanged[cz*_cWidth*_cHeight + cy*_cWidth + cx] = true;
						}
						if (y > 0){//bottom face
							if (voxels[z*layerSize + (y - 1)*width + x].type == '\0' || voxels[z*layerSize + (y - 1)*width + x].type == 0){
								for (int i = 16; i < 20; i++){
									tv = _baseMesh.verts[i];
									tv.position += glm::vec3(x, y, z);
									for (int j = 0; j < 4; j++)
										tv.color[j] = currentVox.color[j];
									_chunkVerts[cz*_cWidth*_cHeight + cy*_cWidth + cx].push_back(tv);
									//_currentVerts.push_back(tv);
								}
								_chunkChanged[cz*_cWidth*_cHeight + cy*_cWidth + cx] = true;
							}
						}
						else{
							for (int i = 16; i < 20; i++){
								tv = _baseMesh.verts[i];
								tv.position += glm::vec3(x, y, z);
								for (int j = 0; j < 4; j++)
									tv.color[j] = currentVox.color[j];
								_chunkVerts[cz*_cWidth*_cHeight + cy*_cWidth + cx].push_back(tv);
								//_currentVerts.push_back(tv);
							}
							_chunkChanged[cz*_cWidth*_cHeight + cy*_cWidth + cx] = true;
						}
						if (z > 0){//back face
							if (voxels[(z - 1)*layerSize + y*width + x].type == '\0' || voxels[(z - 1)*layerSize + y*width + x].type == 0){
								for (int i = 20; i < 24; i++){
									tv = _baseMesh.verts[i];
									tv.position += glm::vec3(x, y, z);
									for (int j = 0; j < 4; j++)
										tv.color[j] = currentVox.color[j];
									_chunkVerts[cz*_cWidth*_cHeight + cy*_cWidth + cx].push_back(tv);
									//_currentVerts.push_back(tv);
								}
								_chunkChanged[cz*_cWidth*_cHeight + cy*_cWidth + cx] = true;
							}
						}
						else{
							for (int i = 20; i < 24; i++){
								tv = _baseMesh.verts[i];
								tv.position += glm::vec3(x, y, z);
								for (int j = 0; j < 4; j++)
									tv.color[j] = currentVox.color[j];
								_chunkVerts[cz*_cWidth*_cHeight + cy*_cWidth + cx].push_back(tv);
								//_currentVerts.push_back(tv);
							}
							_chunkChanged[cz*_cWidth*_cHeight + cy*_cWidth + cx] = true;
						}
					}
				}
			}
		}
	}
}