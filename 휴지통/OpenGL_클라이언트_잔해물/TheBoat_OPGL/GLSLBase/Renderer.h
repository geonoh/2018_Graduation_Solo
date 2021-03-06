#pragma once

#include <string>
#include <cstdlib>
#include <fstream>
#include <iostream>

#include "Dependencies\glew.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/euler_angles.hpp"

class Renderer
{
private:
	// Making Normalizsed Vector
	// From Model to Camera Position
	glm::vec3 MakingNormalizedLookVector(glm::vec3& eye, glm::vec3& object);
	float GetVectorSize(glm::vec3& input_vec);
	
public:
	Renderer(int windowSizeX, int windowSizeY);
	~Renderer();

	bool IsInitialized();
	void Test(float* centers, float time);
	void Radar(float* centers, float time);
	void FillAll(float r, float g, float b, float a);
	void LineAni(); 
	void Triangle();
	void Texture(GLuint texID, GLuint x, GLuint y, GLuint width, GLuint height);
	void TextureApp();
	void TextureAnim();
	void Bogang();
	void Q1();
	void Q2();
	void Cube();
	void SetRotation(float rX, float rY);
	void Test1();

	void ProxyGeo();

	void DrawSTParticle(float sx, float sy, float tx, float ty, float time);
	void DrawParticle();

	float m_targetPointX, m_targetPointY;

	GLuint CreatePngTexture(char * filePath);

	void BloomPass1();
	void BloomPass2(GLuint texEmissive);
	void BloomPass3(GLuint baseTex, GLuint bloomTex);

	// Camera Setting 
	void SetCameraPos(float x, float y, float z);
	
	void DrawHeightMap();
	void DrawSkyBox();
	void DrawTexture(GLuint texID, GLuint x, GLuint y, GLuint width, GLuint height);
	
	void SetCameraLook(float x, float y, float z);
private:
	void Initialize(int windowSizeX, int windowSizeY);
	bool ReadFile(char* filename, std::string *target);
	void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType);
	GLuint CompileShaders(char* filenameVS, char* filenameFS);
	void CreateVertexBufferObjects();
	void CreateProxyGeo();
	void CreateParticle();
	void GenFBOs();

	bool m_Initialized = false;
	
	unsigned int m_WindowSizeX = 0;
	unsigned int m_WindowSizeY = 0;

	GLuint m_VBORect = 0;
	GLuint m_VBOLine = 0;
	GLuint m_VBOPoints = 0;
	GLuint m_VBORadar = 0;
	GLuint m_VBOFillAll = 0;

	GLuint m_VBOTrianglePos = 0;
	GLuint m_VBOTriangleCol = 0;
	GLuint m_VBOTrianglePosCol = 0;
	GLuint m_VBOPosTex = 0;
	
	GLuint m_SolidRectShader = 0;
	GLuint m_STParticleShader = 0;
	GLuint m_TestShader = 0;
	GLuint m_RadarShader = 0;
	GLuint m_FillAllShader = 0;
	GLuint m_TextureShader = 0;
	GLuint m_TextureAnimShader = 0;

	GLuint m_TEXCheckerboard = 0;

	GLuint m_VBO_Q1 = 0;
	GLuint m_Shader_Q1 = 0;
	GLuint m_VBO_Q2 = 0;
	GLuint m_Shader_Q2 = 0;

	GLuint m_VBO_TextureApp = 0;
	GLuint m_Shader_TextureApp = 0;
	GLuint m_Tex_Smile = 0;
	GLuint m_Tex_Smile1 = 0;
	GLuint m_Tex_Smile2 = 0;
	GLuint m_Tex_Smile3 = 0;
	GLuint m_Tex_Smile4 = 0;
	GLuint m_Tex_Smile5 = 0;
	GLuint m_Tex_SmileTotal = 0;


	GLuint m_Tex_BTS = 0;
	GLuint m_Tex_Twice = 0;
	GLuint m_Tex_Brick = 0;
	GLuint m_Tex_Particle = 0;


	GLuint m_Shader_ProxyGeo = 0;
	GLuint m_VBO_ProxyGeo = 0;
	GLuint m_Count_ProxyGeo = 0;

	GLuint m_Shader_Particle = 0;
	GLuint m_VBO_Particle = 0;
	GLuint m_Count_Particle = 0;

	//camera position
	glm::vec3 m_v3Camera_Position;
	//camera lookat position
	glm::vec3 m_v3Camera_Lookat;
	//camera up vector
	glm::vec3 m_v3Camera_Up;

	glm::mat4 m_m4OrthoProj;
	glm::mat4 m_m4PersProj;
	glm::mat4 m_m4Model;
	glm::mat4 m_m4View;
	glm::mat4 m_m4ProjView;

	glm::vec3 m_v3ModelTranslation;
	glm::vec3 m_v3ModelRotation;
	glm::vec3 m_v3ModelScaling;

	glm::mat4 m_m4ModelTranslation;
	glm::mat4 m_m4ModelRotation;
	glm::mat4 m_m4ModelScaling;

	GLuint m_VBO_Cube = 0;
	GLuint m_Shader_Proj;

	GLuint m_Shader_Test1;
	GLuint m_VBO_Test1;

	GLuint m_FBO_P1 = 0;
	GLuint m_Tex_P1_Color = 0;
	GLuint m_Tex_P1_Emissive = 0;
	GLuint m_Tex_P1_Depth = 0;

	GLuint m_FBO_P2 = 0;
	GLuint m_Tex_P2_Bloom = 0;
	GLuint m_Tex_P2_Depth = 0;
	
	GLuint m_Shader_Bloom_P2;
	GLuint m_Shader_Bloom_P3;

	//HeightMap resources
	void InitializeHeightMapGridMesh(int nWidth, int nHeight); 
	void InitializeHeightMap();
	void LoadHeightMapImage(char* pFileName, int nWidth, int nHeight);
	glm::vec3 GetHeightMapNormal(int x, int y);
	int m_HeightMapImageWidth;
	int m_HeightMapImageHeight;
	unsigned char* m_pHeightMapImage;
	GLuint m_VBO_HeightMapVertices;
	GLuint m_VBO_HeightMapIndices;
	GLuint m_Count_HeightMapIndices;
	GLuint m_Shader_HeightMap;
	GLuint m_Tex_HeightMap;
	GLuint m_Tex_HeightMapDetail;
	GLuint m_Tex_HeightMapWater;

	//SkyBox resources
	void InitializeSkyBox();
	GLuint m_Shader_SkyBox;
	GLuint m_VBO_SkyBox;
	GLuint m_Tex_Cube_SkyBox;
};

