#include "stdafx.h"
#include "Renderer.h"
#include "LoadPng.h"
#include <Windows.h>
#include <cstdlib>

unsigned char * loadBMPRaw(const char * imagepath, unsigned int& outWidth, unsigned int& outHeight)
{
	printf("Reading image %s\n", imagepath);
	outWidth = -1;
	outHeight = -1;
	// Data read from the header of the BMP file
	unsigned char header[54];
	unsigned int dataPos;
	unsigned int imageSize;
	// Actual RGB data
	unsigned char * data;

	// Open the file
	FILE * file = NULL;
	fopen_s(&file, imagepath, "rb");
	if (!file)
	{
		printf("Image could not be opened\n");
		return NULL;
	}

	if (fread(header, 1, 54, file) != 54)
	{
		printf("Not a correct BMP file\n");
		return NULL;
	}

	if (header[0] != 'B' || header[1] != 'M')
	{
		printf("Not a correct BMP file\n");
		return NULL;
	}

	if (*(int*)&(header[0x1E]) != 0)
	{
		printf("Not a correct BMP file\n");
		return NULL;
	}

	if (*(int*)&(header[0x1C]) != 24)
	{
		printf("Not a correct BMP file\n");
		return NULL;
	}

	dataPos = *(int*)&(header[0x0A]);
	imageSize = *(int*)&(header[0x22]);
	outWidth = *(int*)&(header[0x12]);
	outHeight = *(int*)&(header[0x16]);

	if (imageSize == 0)
		imageSize = outWidth*outHeight * 3;

	if (dataPos == 0)
		dataPos = 54;

	data = new unsigned char[imageSize];

	fread(data, 1, imageSize, file);

	fclose(file);

	return data;
}

Renderer::Renderer(int windowSizeX, int windowSizeY)
{
	Initialize(windowSizeX, windowSizeY);
}


Renderer::~Renderer()
{
}

void Renderer::SetCameraLook(float x, float y, float z) {
	m_v3Camera_Lookat = glm::vec3(x, y, z);
	m_m4View = glm::lookAt(
		m_v3Camera_Position,
		m_v3Camera_Lookat,
		m_v3Camera_Up
	);
	m_m4PersProj = glm::perspectiveRH((float)VIEW_ANGLE, 1.f, 1.f, 1000.f);
	m_m4ProjView = m_m4PersProj * m_m4View;
}

void Renderer::SetCameraPos(float x, float y, float z) {
	m_v3Camera_Position = glm::vec3(x, y, z);
	m_m4View = glm::lookAt(
		m_v3Camera_Position,
		m_v3Camera_Lookat,
		m_v3Camera_Up
	);
	m_m4PersProj = glm::perspectiveRH((float)VIEW_ANGLE, 1.f, 1.f, 1000.f);
	m_m4ProjView = m_m4PersProj * m_m4View;
}

void Renderer::Initialize(int windowSizeX, int windowSizeY)
{
	//Set window size
	m_WindowSizeX = windowSizeX;
	m_WindowSizeY = windowSizeY;


	GenFBOs();

	float vertPosTex[30] =
	{
		-0.5f, 0.5f, 0.0f, 0.0f, 1.0f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.5f, 0.5f, 0.0f, 1.0f, 1.0f,
		0.5f, 0.5f, 0.0f, 1.0f, 1.0f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f
	};


	glGenBuffers(1, &m_VBO_Test1);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Test1);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertPosTex), vertPosTex, GL_STATIC_DRAW);


	//Create VBOs
	CreateVertexBufferObjects();

	//Create Proxy Geometry
	CreateProxyGeo();

	//Create Particles
	CreateParticle();

	// 
	float denom = sqrt(0.f*0.f + 30.f*30.f + 1.f*1.f);
	
	//Initialize camera settings
	m_v3Camera_Position = glm::vec3(0.f, 60.f, 10.5f);
	m_v3Camera_Lookat = glm::vec3(0.f, 0, 10.5f);
	glm::vec3 model_pos = glm::vec3(-150.f, 90, 0.f);


	glm::mat4 rot_mat;
	glm::dot(m_v3Camera_Position, model_pos);
	float between_cos = glm::dot(m_v3Camera_Position, model_pos) / (GetVectorSize(m_v3Camera_Position) * GetVectorSize(model_pos));
	
	//glm::rotate()
	//glm::rotate()
	//m_v3Camera_Lookat = MakingNormalizedLookVector(m_v3Camera_Position, model_pos);
	printf("%f %f %f\n", m_v3Camera_Lookat.x, m_v3Camera_Lookat.y, m_v3Camera_Lookat.z);
	m_v3Camera_Up = glm::vec3(1.f, 0.f, 0.f);
	m_m4View = glm::lookAt(
		m_v3Camera_Position,
		m_v3Camera_Lookat,
		m_v3Camera_Up
	);

	//Initialize projection matrix
	m_m4OrthoProj = glm::ortho(-1.f, 1.f, -1.f, 1.f, 0.f, 2.f);
	m_m4PersProj = glm::perspectiveRH(45.f, 1.f, 1.f, 1000.f);

	//Initialize projection-view matrix
	//m_m4ProjView = m_m4OrthoProj * m_m4View;
	m_m4ProjView = m_m4PersProj * m_m4View;

	//Initialize transforms
	m_v3ModelTranslation = glm::vec3(0.f, 0.f, 0.f);
	m_v3ModelRotation = glm::vec3(0.f, 0.f, 0.f);
	m_v3ModelScaling = glm::vec3(1.f, 1.f, 1.f);
	m_m4ModelTranslation
		=
		glm::translate(glm::mat4(1.f), m_v3ModelTranslation);
	m_m4ModelRotation
		=
		glm::eulerAngleXYZ(m_v3ModelRotation.x, m_v3ModelRotation.y, m_v3ModelRotation.z);
	m_m4ModelScaling
		=
		glm::scale(glm::mat4(1.f), m_v3ModelScaling);
	m_m4Model
		= m_m4ModelTranslation * m_m4ModelRotation*m_m4ModelScaling;


	m_Tex_Particle = CreatePngTexture("./Textures/particle.png");

	//Height Map Initialization
	InitializeHeightMap();

	//SkyBox Initialization
	InitializeSkyBox();

	m_Initialized = true;
}

bool Renderer::IsInitialized()
{
	return m_Initialized;
}

GLuint Renderer::CreatePngTexture(char * filePath)
{

	GLuint temp;
	glGenTextures(1, &temp);

	//Load Pngs
	// Load file and decode image.
	std::vector<unsigned char> image;
	unsigned width, height;
	unsigned error = lodepng::decode(image, width, height, filePath);

	glBindTexture(GL_TEXTURE_2D, temp);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &image[0]);

	return temp;
}

void Renderer::CreateVertexBufferObjects()
{
	float temp = 0.5f;

	float cube[] = {
		-temp, -temp, temp, 0.f, 0.f, 1.f, 1.f, 0.f, 0.f, 1.f, //x, y, z, nx, ny, nz, r, g, b, a
		temp, temp, temp, 0.f, 0.f, 1.f, 1.f, 0.f, 0.f, 1.f,
		-temp, temp, temp, 0.f, 0.f, 1.f, 1.f, 0.f, 0.f, 1.f,
		-temp, -temp, temp, 0.f, 0.f, 1.f, 1.f, 0.f, 0.f, 1.f,
		temp, -temp, temp, 0.f, 0.f, 1.f, 1.f, 0.f, 0.f, 1.f,
		temp, temp, temp, 0.f, 0.f, 1.f, 1.f, 0.f, 0.f, 1.f, // first face : R

		temp, -temp, temp, 1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 1.f, //x, y, z, nx, ny, nz, r, g, b, a
		temp, temp, -temp, 1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 1.f,
		temp, temp, temp, 1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 1.f,
		temp, -temp, temp, 1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 1.f, //x, y, z, nx, ny, nz, r, g, b, a
		temp, -temp, -temp, 1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 1.f,
		temp, temp, -temp, 1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 1.f, //second face : G

		-temp, temp, temp, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, //x, y, z, nx, ny, nz, r, g, b, a
		temp, temp, -temp, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f,
		-temp, temp, -temp, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f,
		-temp, temp, temp, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, //x, y, z, nx, ny, nz, r, g, b, a
		temp, temp, temp, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f,
		temp, temp, -temp, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, //third face : B

		-temp, -temp, -temp, 0.f, 0.f, -1.f, 1.f, 1.f, 0.f, 1.f, //x, y, z, nx, ny, nz, r, g, b, a
		-temp, temp, -temp, 0.f, 0.f, -1.f, 1.f, 1.f, 0.f, 1.f,
		temp, temp, -temp, 0.f, 0.f, -1.f, 1.f, 1.f, 0.f, 1.f,
		-temp, -temp, -temp, 0.f, 0.f, -1.f, 1.f, 1.f, 0.f, 1.f, //x, y, z, nx, ny, nz, r, g, b, a
		temp, temp, -temp, 0.f, 0.f, -1.f, 1.f, 1.f, 0.f, 1.f,
		temp, -temp, -temp, 0.f, 0.f, -1.f, 1.f, 1.f, 0.f, 1.f, //fourth face : R+G (yellow)

		-temp, -temp, temp, -1.f, 0.f, 0.f, 1.f, 0.f, 1.f, 1.f, //x, y, z, nx, ny, nz, r, g, b, a 
		-temp, temp, temp, -1.f, 0.f, 0.f, 1.f, 0.f, 1.f, 1.f,
		-temp, temp, -temp, -1.f, 0.f, 0.f, 1.f, 0.f, 1.f, 1.f,
		-temp, -temp, temp, -1.f, 0.f, 0.f, 1.f, 0.f, 1.f, 1.f, //x, y, z, nx, ny, nz, r, g, b, a 
		-temp, temp, -temp, -1.f, 0.f, 0.f, 1.f, 0.f, 1.f, 1.f,
		-temp, -temp, -temp, -1.f, 0.f, 0.f, 1.f, 0.f, 1.f, 1.f, // fifth face : R+B (purple)

		-temp, -temp, temp, 0.f, -1.f, 0.f, 0.f, 1.f, 1.f, 1.f, //x, y, z, nx, ny, nz, r, g, b, a 
		temp, -temp, -temp, 0.f, -1.f, 0.f, 0.f, 1.f, 1.f, 1.f,
		temp, -temp, temp, 0.f, -1.f, 0.f, 0.f, 1.f, 1.f, 1.f,
		-temp, -temp, temp, 0.f, -1.f, 0.f, 0.f, 1.f, 1.f, 1.f, //x, y, z, nx, ny, nz, r, g, b, a 
		-temp, -temp, -temp, 0.f, -1.f, 0.f, 0.f, 1.f, 1.f, 1.f,
		temp, -temp, -temp, 0.f, -1.f, 0.f, 0.f, 1.f, 1.f, 1.f, //sixth face : G+B (Cyan)
	};

	glGenBuffers(1, &m_VBO_Cube);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Cube);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube), cube, GL_STATIC_DRAW);

	float size = 1.f;
	float rect[]
		=
	{
		-size, -size, 0.f,
		-size, size, 0.f,
		size, size, 0.f, //Triangle1
		-size, -size, 0.f,
		size, size, 0.f,
		size, -size, 0.f, //Triangle2
	};

	glGenBuffers(1, &m_VBORect);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBORect);
	glBufferData(GL_ARRAY_BUFFER, sizeof(rect), rect, GL_STATIC_DRAW);


	float lines[]
		=
	{
		-0.5, -0.5, 0.f, 0.f,
		-0.5,  0.5, 0.f, 1.f/3.f,
		 0.5,  0.5, 0.f, 2.f/3.f,
		-0.5, -0.5, 0.f, 1.f
	};
	glGenBuffers(1, &m_VBOLine);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBOLine);
	glBufferData(GL_ARRAY_BUFFER, sizeof(lines), lines, GL_STATIC_DRAW);

	float Position[9] = { 
		0.0, 1.0, 0.0, 
		-1.0, -1.0, 0.0, 
		1.0, -1.0, 0.0 
	};
	glGenBuffers(1, &m_VBOTrianglePos);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBOTrianglePos);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Position), Position, GL_STATIC_DRAW);

	float Color[12] = { 
		1.0, 1.0, 1.0, 1.0, 
		1.0, 0.0, 0.0, 1.0, 
		0.0, 0.0, 1.0, 1.0 
	};
	glGenBuffers(1, &m_VBOTriangleCol);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBOTriangleCol);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Color), Color, GL_STATIC_DRAW);

	float PositionColor[] = { 
		0.0,  1.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0, //v0
	   -1.0, -1.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, //v1
		1.0, -1.0, 0.0, 1.0, 0.0, 0.0, 1.0, 1.0  //v2
	};
	glGenBuffers(1, &m_VBOTrianglePosCol);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBOTrianglePosCol);
	glBufferData(GL_ARRAY_BUFFER, sizeof(PositionColor), PositionColor, GL_STATIC_DRAW);

	int pointCount = 500;
	float * Points = new float[(pointCount + 1) * 4];
	for (int i = 0; i <= pointCount; i++)
	{
		Points[i * 4 + 0] = (float)i / (float)pointCount * 2; //start time
		Points[i * 4 + 1] = (float)rand() / (float)RAND_MAX; //period
		Points[i * 4 + 2] = (float)rand() / (float)RAND_MAX; //width
		Points[i * 4 + 3] = 1;
		if ((float)rand() / (float)RAND_MAX > 0.5)
		{
			Points[i * 4 + 2] *= -1.f;
		}
	}

	glGenBuffers(1, &m_VBOPoints);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBOPoints);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*(pointCount + 1)*4, Points, GL_STATIC_DRAW);

	size = 1.f;
	float rectRadar[]
		=
	{
		-size, -size, 0.f, 1.f,
		-size, size, 0.f, 1.f,
		size, size, 0.f, 1.f, //Triangle1
		-size, -size, 0.f, 1.f,
		size, size, 0.f, 1.f,
		size, -size, 0.f, 1.f, //Triangle2
	}; 
	glGenBuffers(1, &m_VBORadar);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBORadar);
	glBufferData(GL_ARRAY_BUFFER, sizeof(rectRadar), rectRadar, GL_STATIC_DRAW);
	
	size = 1.f;
	float rectFillAll[]
		=
	{
		-size, -size, 0.f, 1.f,
		-size, size, 0.f, 1.f,
		size, size, 0.f, 1.f, //Triangle1
		-size, -size, 0.f, 1.f,
		size, size, 0.f, 1.f,
		size, -size, 0.f, 1.f, //Triangle2
	};
	glGenBuffers(1, &m_VBOFillAll);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBOFillAll);
	glBufferData(GL_ARRAY_BUFFER, sizeof(rectFillAll), rectFillAll, GL_STATIC_DRAW);
	
	size = 1.f;
	float rectPosTex[]
		=
	{
		-size, -size, 0.f, 0.f, 0.f, //x, y, z, s, t
		-size, size, 0.f, 0.f, 1.f,
		size, size, 0.f, 1.f, 1.f,//Triangle1
		-size, -size, 0.f, 0.f, 0.f,
		size, size, 0.f, 1.f, 1.f,
		size, -size, 0.f, 1.f, 0.f,//Triangle2
	};

	glGenBuffers(1, &m_VBOPosTex);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBOPosTex);
	glBufferData(GL_ARRAY_BUFFER, sizeof(rectPosTex), rectPosTex, GL_STATIC_DRAW);

	//Create Texture
	static const GLulong checkerboard[] =
	{
		0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
		0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
		0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
		0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
		0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
		0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
		0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
		0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF
	};
	
	glGenTextures(1, &m_TEXCheckerboard);
	glBindTexture(GL_TEXTURE_2D, m_TEXCheckerboard);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 
		8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, 
		checkerboard);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	float q1[] = { 0, 0, 0, 1, 0, 0, 0, 0.5 };
	glGenBuffers(1, &m_VBO_Q1);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Q1);
	glBufferData(GL_ARRAY_BUFFER, sizeof(q1), q1, GL_STATIC_DRAW);
	
	/*float q2[] 
		= 
	{ 
		-1.f, -1.f, 0.f, 
		-1.f, 1.f, 0.f, 
		1.f, 1.f, 0.f
		-1.f, -1.f, 0.f,
		1.f, 1.f, 0.f,
		1.f, -1.f, 0.f, 
	};	*/
	size = 1.f;
	float q2[]
		=
	{
		-size, -size, 0.f,
		-size, size, 0.f,
		size, size, 0.f, //Triangle1
		-size, -size, 0.f,
		size, size, 0.f,
		size, -size, 0.f, //Triangle2
	};
	glGenBuffers(1, &m_VBO_Q2);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Q2);
	glBufferData(GL_ARRAY_BUFFER, sizeof(q2), q2, GL_STATIC_DRAW);
	
	size = 0.5f;
	float textureApp[]
		=
	{
		-size, -size, 0.f, 0, 0,
		-size, size, 0.f, 0, 1,
		size, size, 0.f, 1, 1,//Triangle1
		-size, -size, 0.f, 0, 0,
		size, size, 0.f, 1, 1,
		size, -size, 0.f, 1, 0,//Triangle2
	};
	glGenBuffers(1, &m_VBO_TextureApp);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_TextureApp);
	glBufferData(GL_ARRAY_BUFFER, sizeof(textureApp), textureApp, GL_STATIC_DRAW);

	//Create Texture
	GLulong textureSmile[]
		=
	{
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFFFFFFFF,
		0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00,
		0xFF00FF00, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF00FF00,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFF0000FF, 0xFF0000FF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF0000, 0xFFFF0000,
		0xFF0000FF, 0xFF0000FF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF0000, 0xFFFF0000,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF
	};

	glGenTextures(1, &m_Tex_Smile);
	glBindTexture(GL_TEXTURE_2D, m_Tex_Smile);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
		8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE,
		textureSmile);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	GLulong textureSmile1[]
		=
	{
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00,
		0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFF0000FF, 0xFF0000FF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF0000, 0xFFFF0000,
		0xFF0000FF, 0xFF0000FF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF0000, 0xFFFF0000,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF
	};
	glGenTextures(1, &m_Tex_Smile1);
	glBindTexture(GL_TEXTURE_2D, m_Tex_Smile1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureSmile1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	GLulong textureSmile2[]
		=
	{
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF00FF00, 0xFF00FF00, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00,
		0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFF0000FF, 0xFF0000FF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF0000, 0xFFFF0000,
		0xFF0000FF, 0xFF0000FF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF0000, 0xFFFF0000,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF
	};
	glGenTextures(1, &m_Tex_Smile2);
	glBindTexture(GL_TEXTURE_2D, m_Tex_Smile2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureSmile2);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	GLulong textureSmile3[]
		=
	{
		0xFF00FF00, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF00FF00,
		0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00,
		0xFFFFFFFF, 0xFF00FF00, 0xFF00FF00, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF00FF00, 0xFF00FF00, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFF0000FF, 0xFF0000FF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF0000, 0xFFFF0000,
		0xFF0000FF, 0xFF0000FF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF0000, 0xFFFF0000,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF
	};
	glGenTextures(1, &m_Tex_Smile3);
	glBindTexture(GL_TEXTURE_2D, m_Tex_Smile3);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureSmile3);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	GLulong textureSmile4[]
		=
	{
		0xFF00FF00, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF00FF00,
		0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00,
		0xFFFFFFFF, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFF0000FF, 0xFF0000FF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF0000, 0xFFFF0000,
		0xFF0000FF, 0xFF0000FF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF0000, 0xFFFF0000,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF
	};
	glGenTextures(1, &m_Tex_Smile4);
	glBindTexture(GL_TEXTURE_2D, m_Tex_Smile4);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureSmile4);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	GLulong textureSmile5[]
		=
	{
		0xFF00FF00, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF00FF00,
		0xFF00FF00, 0xFF00FF00, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF00FF00, 0xFF00FF00,
		0xFFFFFFFF, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFF0000FF, 0xFF0000FF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF0000, 0xFFFF0000,
		0xFF0000FF, 0xFF0000FF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF0000, 0xFFFF0000,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF
	};
	glGenTextures(1, &m_Tex_Smile5);
	glBindTexture(GL_TEXTURE_2D, m_Tex_Smile5);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureSmile5);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	GLulong textureSmileTotal[]
		=
	{
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFFFFFFFF,
		0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00,
		0xFF00FF00, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF00FF00,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFF0000FF, 0xFF0000FF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF0000, 0xFFFF0000,
		0xFF0000FF, 0xFF0000FF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF0000, 0xFFFF0000,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,

		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00,
		0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFF0000FF, 0xFF0000FF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF0000, 0xFFFF0000,
		0xFF0000FF, 0xFF0000FF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF0000, 0xFFFF0000,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,

		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF00FF00, 0xFF00FF00, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00,
		0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFF0000FF, 0xFF0000FF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF0000, 0xFFFF0000,
		0xFF0000FF, 0xFF0000FF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF0000, 0xFFFF0000,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,

		0xFF00FF00, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF00FF00,
		0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00,
		0xFFFFFFFF, 0xFF00FF00, 0xFF00FF00, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF00FF00, 0xFF00FF00, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFF0000FF, 0xFF0000FF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF0000, 0xFFFF0000,
		0xFF0000FF, 0xFF0000FF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF0000, 0xFFFF0000,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,

		0xFF00FF00, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF00FF00,
		0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00,
		0xFFFFFFFF, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFF0000FF, 0xFF0000FF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF0000, 0xFFFF0000,
		0xFF0000FF, 0xFF0000FF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF0000, 0xFFFF0000,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,

		0xFF00FF00, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF00FF00,
		0xFF00FF00, 0xFF00FF00, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF00FF00, 0xFF00FF00,
		0xFFFFFFFF, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFF0000FF, 0xFF0000FF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF0000, 0xFFFF0000,
		0xFF0000FF, 0xFF0000FF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF0000, 0xFFFF0000,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF
	};
	glGenTextures(1, &m_Tex_SmileTotal);
	glBindTexture(GL_TEXTURE_2D, m_Tex_SmileTotal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 8, 48, 0, GL_RGB, GL_UNSIGNED_BYTE, textureSmileTotal);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	unsigned int x, y;
	unsigned char * temp1 = loadBMPRaw("./textures/twice.bmp", x, y);
	glGenTextures(1, &m_Tex_Twice);
	glBindTexture(GL_TEXTURE_2D, m_Tex_Twice);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, temp1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	unsigned int x1, y1;
	unsigned char * temp2 = loadBMPRaw("./textures/bts.bmp", x1, y1);
	glGenTextures(1, &m_Tex_BTS);
	glBindTexture(GL_TEXTURE_2D, m_Tex_BTS);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x1, y1, 0, GL_RGB, GL_UNSIGNED_BYTE, temp2);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	unsigned int x2, y2;
	unsigned char * temp3 = loadBMPRaw("./textures/brick.bmp", x2, y2);
	glGenTextures(1, &m_Tex_Brick);
	glBindTexture(GL_TEXTURE_2D, m_Tex_Brick);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x2, y2, 0, GL_RGB, GL_UNSIGNED_BYTE, temp3);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
}

void Renderer::CreateParticle()
{
	int particleCount = 500;
	float particleSize = 0.05f;

	float* particleVertices = new float[particleCount * 2 * 3 * (3 + 2 + 4)];
	int particleFloatCount = particleCount * 2 * 3 * (3 + 2 + 4);
	m_Count_Particle = particleCount * 2 * 3;

	int particleVertIndex = 0;

	for (int i = 0; i < particleCount; i++)
	{
		float randomValueX = 0.f;
		float randomValueY = 0.f;
		float randomValueZ = 0.f;
		float randomStartTime = 0.f;
		float velocityScale = 0.1f;
		float particleInitPosX = 0.0f;
		float particleInitPosY = 0.0f;

		randomValueX = (rand() / (float)RAND_MAX - 0.5)*velocityScale;
		randomValueY = (rand() / (float)RAND_MAX - 0.5)*velocityScale;
		randomValueZ = 0.f;
		randomStartTime = (rand() / (float)RAND_MAX)*3.f;
		particleInitPosX = (rand() / (float)RAND_MAX - 0.5)*2;
		particleInitPosY = (rand() / (float)RAND_MAX - 0.5)*2;

		particleVertices[particleVertIndex] = -particleSize / 2.f + particleInitPosX;
		particleVertIndex++;
		particleVertices[particleVertIndex] = -particleSize / 2.f + particleInitPosY;
		particleVertIndex++;
		particleVertices[particleVertIndex] = 0.f;
		particleVertIndex++;
		particleVertices[particleVertIndex] = 0.f;
		particleVertIndex++;
		particleVertices[particleVertIndex] = 0.f;
		particleVertIndex++;
		particleVertices[particleVertIndex] = randomValueX;
		particleVertIndex++;
		particleVertices[particleVertIndex] = randomValueY;
		particleVertIndex++;
		particleVertices[particleVertIndex] = randomValueZ;
		particleVertIndex++;
		particleVertices[particleVertIndex] = randomStartTime;
		particleVertIndex++;

		particleVertices[particleVertIndex] = particleSize / 2.f + particleInitPosX;
		particleVertIndex++;
		particleVertices[particleVertIndex] = -particleSize / 2.f + particleInitPosY;
		particleVertIndex++;
		particleVertices[particleVertIndex] = 0.f;
		particleVertIndex++;
		particleVertices[particleVertIndex] = 1.f;
		particleVertIndex++;
		particleVertices[particleVertIndex] = 0.f;
		particleVertIndex++;
		particleVertices[particleVertIndex] = randomValueX;
		particleVertIndex++;
		particleVertices[particleVertIndex] = randomValueY;
		particleVertIndex++;
		particleVertices[particleVertIndex] = randomValueZ;
		particleVertIndex++;
		particleVertices[particleVertIndex] = randomStartTime;
		particleVertIndex++;

		particleVertices[particleVertIndex] = particleSize / 2.f + particleInitPosX;
		particleVertIndex++;
		particleVertices[particleVertIndex] = particleSize / 2.f + particleInitPosY;
		particleVertIndex++;
		particleVertices[particleVertIndex] = 0.f;
		particleVertIndex++;
		particleVertices[particleVertIndex] = 1.f;
		particleVertIndex++;
		particleVertices[particleVertIndex] = 1.f;
		particleVertIndex++;
		particleVertices[particleVertIndex] = randomValueX;
		particleVertIndex++;
		particleVertices[particleVertIndex] = randomValueY;
		particleVertIndex++;
		particleVertices[particleVertIndex] = randomValueZ;
		particleVertIndex++;
		particleVertices[particleVertIndex] = randomStartTime;
		particleVertIndex++;

		particleVertices[particleVertIndex] = -particleSize / 2.f + particleInitPosX;
		particleVertIndex++;
		particleVertices[particleVertIndex] = -particleSize / 2.f + particleInitPosY;
		particleVertIndex++;
		particleVertices[particleVertIndex] = 0.f;
		particleVertIndex++;
		particleVertices[particleVertIndex] = 0.f;
		particleVertIndex++;
		particleVertices[particleVertIndex] = 0.f;
		particleVertIndex++;
		particleVertices[particleVertIndex] = randomValueX;
		particleVertIndex++;
		particleVertices[particleVertIndex] = randomValueY;
		particleVertIndex++;
		particleVertices[particleVertIndex] = randomValueZ;
		particleVertIndex++;
		particleVertices[particleVertIndex] = randomStartTime;
		particleVertIndex++;

		particleVertices[particleVertIndex] = particleSize / 2.f + particleInitPosX;
		particleVertIndex++;
		particleVertices[particleVertIndex] = particleSize / 2.f + particleInitPosY;
		particleVertIndex++;
		particleVertices[particleVertIndex] = 0.f;
		particleVertIndex++;
		particleVertices[particleVertIndex] = 1.f;
		particleVertIndex++;
		particleVertices[particleVertIndex] = 1.f;
		particleVertIndex++;
		particleVertices[particleVertIndex] = randomValueX;
		particleVertIndex++;
		particleVertices[particleVertIndex] = randomValueY;
		particleVertIndex++;
		particleVertices[particleVertIndex] = randomValueZ;
		particleVertIndex++;
		particleVertices[particleVertIndex] = randomStartTime;
		particleVertIndex++;

		particleVertices[particleVertIndex] = -particleSize / 2.f + particleInitPosX;
		particleVertIndex++;
		particleVertices[particleVertIndex] = particleSize / 2.f + particleInitPosY;
		particleVertIndex++;
		particleVertices[particleVertIndex] = 0.f;
		particleVertIndex++;
		particleVertices[particleVertIndex] = 0.f;
		particleVertIndex++;
		particleVertices[particleVertIndex] = 1.f;
		particleVertIndex++;
		particleVertices[particleVertIndex] = randomValueX;
		particleVertIndex++;
		particleVertices[particleVertIndex] = randomValueY;
		particleVertIndex++;
		particleVertices[particleVertIndex] = randomValueZ;
		particleVertIndex++;
		particleVertices[particleVertIndex] = randomStartTime;
		particleVertIndex++;
	}

	glGenBuffers(1, &m_VBO_Particle);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Particle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*particleFloatCount, particleVertices, GL_STATIC_DRAW);
}

void Renderer::CreateProxyGeo()
{
	float basePosX = -0.5f;
	float basePosY = -0.5f; //left bottom
	float targetPosX = 0.5f;
	float targetPosY = 0.5f; //right top

	int pointCountX = 32;
	int pointCountY = 32; //resolution

	float width = targetPosX - basePosX;
	float height = targetPosY - basePosY;

	float* point = new float[pointCountX*pointCountY * 2];
	float* vertices = new float[(pointCountX - 1)*(pointCountY - 1) * 2 * 3 * 3];
	m_Count_ProxyGeo = (pointCountX - 1)*(pointCountY - 1) * 2 * 3;

	//Prepare points
	for (int x = 0; x < pointCountX; x++)
	{
		for (int y = 0; y < pointCountY; y++)
		{
			point[(y*pointCountX + x) * 2 + 0] = basePosX + width * (x / (float)(pointCountX - 1));
			point[(y*pointCountX + x) * 2 + 1] = basePosY + height * (y / (float)(pointCountY - 1));
		}
	}

	//Make triangles
	int vertIndex = 0;
	for (int x = 0; x < pointCountX - 1; x++)
	{
		for (int y = 0; y < pointCountY - 1; y++)
		{
			//Triangle part 1
			vertices[vertIndex] = point[(y*pointCountX + x) * 2 + 0];
			vertIndex++;
			vertices[vertIndex] = point[(y*pointCountX + x) * 2 + 1];
			vertIndex++;
			vertices[vertIndex] = 0.f;
			vertIndex++;
			vertices[vertIndex] = point[((y + 1)*pointCountX + (x + 1)) * 2 + 0];
			vertIndex++;
			vertices[vertIndex] = point[((y + 1)*pointCountX + (x + 1)) * 2 + 1];
			vertIndex++;
			vertices[vertIndex] = 0.f;
			vertIndex++;
			vertices[vertIndex] = point[((y + 1)*pointCountX + x) * 2 + 0];
			vertIndex++;
			vertices[vertIndex] = point[((y + 1)*pointCountX + x) * 2 + 1];
			vertIndex++;
			vertices[vertIndex] = 0.f;
			vertIndex++;

			//Triangle part 2
			vertices[vertIndex] = point[(y*pointCountX + x) * 2 + 0];
			vertIndex++;
			vertices[vertIndex] = point[(y*pointCountX + x) * 2 + 1];
			vertIndex++;
			vertices[vertIndex] = 0.f;
			vertIndex++;
			vertices[vertIndex] = point[(y*pointCountX + (x + 1)) * 2 + 0];
			vertIndex++;
			vertices[vertIndex] = point[(y*pointCountX + (x + 1)) * 2 + 1];
			vertIndex++;
			vertices[vertIndex] = 0.f;
			vertIndex++;
			vertices[vertIndex] = point[((y + 1)*pointCountX + (x + 1)) * 2 + 0];
			vertIndex++;
			vertices[vertIndex] = point[((y + 1)*pointCountX + (x + 1)) * 2 + 1];
			vertIndex++;
			vertices[vertIndex] = 0.f;
			vertIndex++;
		}
	}

	glGenBuffers(1, &m_VBO_ProxyGeo);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_ProxyGeo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*(pointCountX - 1)*(pointCountY - 1) * 2 * 3 * 3, vertices, GL_STATIC_DRAW);
}

void Renderer::AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	//쉐이더 오브젝트 생성
	GLuint ShaderObj = glCreateShader(ShaderType);

	if (ShaderObj == 0) {
		fprintf(stderr, "Error creating shader type %d\n", ShaderType);
	}

	const GLchar* p[1];
	p[0] = pShaderText;
	GLint Lengths[1];
	Lengths[0] = strlen(pShaderText);
	//쉐이더 코드를 쉐이더 오브젝트에 할당
	glShaderSource(ShaderObj, 1, p, Lengths);

	//할당된 쉐이더 코드를 컴파일
	glCompileShader(ShaderObj);

	GLint success;
	// ShaderObj 가 성공적으로 컴파일 되었는지 확인
	glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar InfoLog[1024];

		//OpenGL 의 shader log 데이터를 가져옴
		glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
		fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
		printf("%s \n", pShaderText);
	}

	// ShaderProgram 에 attach!!
	glAttachShader(ShaderProgram, ShaderObj);
}

bool Renderer::ReadFile(char* filename, std::string *target)
{
	std::ifstream file(filename);
	if (file.fail())
	{
		std::cout << filename << " file loading failed.. \n";
		file.close();
		return false;
	}
	std::string line;
	while (getline(file, line)) {
		target->append(line.c_str());
		target->append("\n");
	}
	return true;
}

GLuint Renderer::CompileShaders(char* filenameVS, char* filenameFS)
{
	GLuint ShaderProgram = glCreateProgram(); //빈 쉐이더 프로그램 생성

	if (ShaderProgram == 0) { //쉐이더 프로그램이 만들어졌는지 확인
		fprintf(stderr, "Error creating shader program\n");
	}

	std::string vs, fs;

	//shader.vs 가 vs 안으로 로딩됨
	if (!ReadFile(filenameVS, &vs)) {
		printf("Error compiling vertex shader\n");
		return -1;
	};

	//shader.fs 가 fs 안으로 로딩됨
	if (!ReadFile(filenameFS, &fs)) {
		printf("Error compiling fragment shader\n");
		return -1;
	};

	// ShaderProgram 에 vs.c_str() 버텍스 쉐이더를 컴파일한 결과를 attach함
	AddShader(ShaderProgram, vs.c_str(), GL_VERTEX_SHADER);

	// ShaderProgram 에 fs.c_str() 프레그먼트 쉐이더를 컴파일한 결과를 attach함
	AddShader(ShaderProgram, fs.c_str(), GL_FRAGMENT_SHADER);

	GLint Success = 0;
	GLchar ErrorLog[1024] = { 0 };

	//Attach 완료된 shaderProgram 을 링킹함
	glLinkProgram(ShaderProgram);

	//링크가 성공했는지 확인
	glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &Success);

	if (Success == 0) {
		// shader program 로그를 받아옴
		glGetProgramInfoLog(ShaderProgram, sizeof(ErrorLog), NULL, ErrorLog);
		std::cout << filenameVS << ", " << filenameFS << " Error linking shader program\n" << ErrorLog;
		return -1;
	}

	glValidateProgram(ShaderProgram);
	glGetProgramiv(ShaderProgram, GL_VALIDATE_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(ShaderProgram, sizeof(ErrorLog), NULL, ErrorLog);
		std::cout << filenameVS << ", " << filenameFS << " Error validating shader program\n" << ErrorLog;
		return -1;
	}

	glUseProgram(ShaderProgram);
	std::cout << filenameVS << ", " << filenameFS << " Shader compiling is done. \n";

	return ShaderProgram;
}

float g_time = 0.f;

void Renderer::Test(float* centers, float time) //input 4
{
	GLuint shader = m_TestShader;

	glUseProgram(shader);

	int uniformTime = glGetUniformLocation(shader, "u_Time");
	glUniform1f(uniformTime, time);

	int uniformCenters = glGetUniformLocation(shader, "u_Centers");
	glUniform2fv(uniformCenters, 4, centers);

	int attribPosition = glGetAttribLocation(shader, "a_Position");

	glEnableVertexAttribArray(attribPosition);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBORect);
	glVertexAttribPointer(attribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(attribPosition);
}

void Renderer::LineAni()
{
	glUseProgram(m_SolidRectShader);

	int attribPosition = glGetAttribLocation(m_SolidRectShader, "a_Position");
	int uniformLocation = glGetUniformLocation(m_SolidRectShader, "u_Time");

	glEnableVertexAttribArray(attribPosition);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBOLine);
	glVertexAttribPointer(attribPosition, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, 0);

	glUniform1f(uniformLocation, g_time);

	g_time += 0.02;

	glDrawArrays(GL_LINE_STRIP, 0, 4);

	glDisableVertexAttribArray(attribPosition);
}


void Renderer::Triangle()
{
	glUseProgram(m_SolidRectShader);

	GLuint attribPos = glGetAttribLocation(m_SolidRectShader, "a_Position");
	GLuint attribCol = glGetAttribLocation(m_SolidRectShader, "a_Color");

	glEnableVertexAttribArray(attribPos);
	glEnableVertexAttribArray(attribCol);

	/*glBindBuffer(GL_ARRAY_BUFFER, m_VBOTrianglePos);
	glVertexAttribPointer(attribPos, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBOTriangleCol);
	glVertexAttribPointer(attribCol, 4, GL_FLOAT, GL_FALSE, 0, 0);*/

	glBindBuffer(GL_ARRAY_BUFFER, m_VBOTrianglePosCol);
	glVertexAttribPointer(attribPos, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), 0);
	glVertexAttribPointer(attribCol, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (GLvoid*)(3*sizeof(float)));

	glDrawArrays(GL_TRIANGLES, 0, 3);
}

void Renderer::DrawSTParticle(float sx, float sy, float tx, float ty, float time)
{
	GLuint shader = m_STParticleShader;
	glUseProgram(shader);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GLuint attribPos = glGetAttribLocation(shader, "a_Position");
	glEnableVertexAttribArray(attribPos);

	GLuint uniformTime = glGetUniformLocation(shader, "u_Time");
	glUniform1f(uniformTime, time);
	GLuint uniformS = glGetUniformLocation(shader, "u_S");
	glUniform2f(uniformS, sx, sy);
	GLuint uniformE = glGetUniformLocation(shader, "u_E");
	glUniform2f(uniformE, tx, ty);
	GLuint uniformTex = glGetUniformLocation(shader, "u_Texture");
	glUniform1i(uniformTex, 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_Tex_Particle);
	
	glBindBuffer(GL_ARRAY_BUFFER, m_VBOPoints);
	glVertexAttribPointer(attribPos, 4, GL_FLOAT, GL_FALSE, 0, 0);
	
	glEnable(GL_PROGRAM_POINT_SIZE);
	glEnable(GL_POINT_SPRITE);

	glDrawArrays(GL_POINTS, 0, 500);
}

void Renderer::Radar(float* centers, float time) //input 4
{
	GLuint shader = m_RadarShader;

	glUseProgram(shader);

	int uniformTime = glGetUniformLocation(shader, "u_Time");
	glUniform1f(uniformTime, time);

	int uniformCenters = glGetUniformLocation(shader, "u_Centers");
	glUniform2fv(uniformCenters, 4, centers);

	int attribPosition = glGetAttribLocation(shader, "a_Position");

	glEnableVertexAttribArray(attribPosition);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBORadar);
	glVertexAttribPointer(attribPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(attribPosition);
}

void Renderer::FillAll(float r, float g, float b, float a)
{
	GLuint shader = m_FillAllShader;

	glUseProgram(shader);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	int uniformColor = glGetUniformLocation(shader, "u_Color");
	glUniform4f(uniformColor, r, g, b, a);

	int attribPosition = glGetAttribLocation(shader, "a_Position");

	glEnableVertexAttribArray(attribPosition);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBOFillAll);
	glVertexAttribPointer(attribPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(attribPosition);
	glDisable(GL_BLEND);
}


void Renderer::Q1()
{
	static float gTime = 0.f;
	GLuint shader = m_Shader_Q1;
	glUseProgram(shader);

	GLuint uTime = glGetUniformLocation(shader, "u_Time");
	glUniform1f(uTime, gTime);

	GLuint aPos = glGetAttribLocation(shader, "a_Position");
	GLuint aInfo = glGetAttribLocation(shader, "a_Info");

	glEnableVertexAttribArray(aPos);
	glEnableVertexAttribArray(aInfo);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Q1);
	glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE,
		sizeof(float) * 4, 0);
	glVertexAttribPointer(aInfo, 1, GL_FLOAT, GL_FALSE,
		sizeof(float) * 4, (GLvoid*)(3 * sizeof(float)));
	
	glPointSize(10.f);

	glDrawArrays(GL_POINTS, 0, 2);
	gTime += 0.01f;

	glDisableVertexAttribArray(aPos);
	glDisableVertexAttribArray(aInfo);
}


void Renderer::Q2()
{
	GLuint shader = m_Shader_Q2;
	glUseProgram(shader);

	GLuint aPos = glGetAttribLocation(shader, "a_Position");
	glEnableVertexAttribArray(aPos);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Q2);
	glVertexAttribPointer(aPos, 
		3, GL_FLOAT, GL_FALSE, 
		0, 0);
	
	glDrawArrays(GL_TRIANGLES,0, 6);

	glDisableVertexAttribArray(aPos);
}

GLuint idd = 0;
float gTime1 = 0.f;

void Renderer::TextureApp()
{
	idd++;

	GLuint shader = m_Shader_TextureApp;

	glUseProgram(shader);

	int uniformSampler = glGetUniformLocation(shader, "u_TextureSlot");
	glUniform1i(uniformSampler, 0);
	int uniformSampler1 = glGetUniformLocation(shader, "u_TextureSlot1");
	glUniform1i(uniformSampler1, 1);

	int uniformTime = glGetUniformLocation(shader, "u_Time");
	glUniform1f(uniformTime, gTime1);
	gTime1 += 0.1;
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_Tex_Brick);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_Tex_BTS);

	int attribPosition = glGetAttribLocation(shader, "a_Position");
	int attribTexPos = glGetAttribLocation(shader, "a_TexPos");

	glEnableVertexAttribArray(attribPosition);
	glEnableVertexAttribArray(attribTexPos);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_TextureApp);
	glVertexAttribPointer(attribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, 0);
	glVertexAttribPointer(attribTexPos, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (GLvoid*)(3 * sizeof(float)));

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(attribPosition);
	glDisableVertexAttribArray(attribTexPos);
}

void Renderer::TextureAnim()
{
	GLuint shader = m_TextureAnimShader;

	glUseProgram(shader);

	int uniformSampler = glGetUniformLocation(shader, "u_TextureSlot");
	glUniform1i(uniformSampler, 0);

	if (idd > 5)
		idd = 0;
	Sleep(200);

	int uniformAnimStep = glGetUniformLocation(shader, "u_AnimStep");
	glUniform1f(uniformAnimStep, idd);
	idd += 1.f;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_Tex_SmileTotal);

	int attribPosition = glGetAttribLocation(shader, "a_Position");
	int attribTexPos = glGetAttribLocation(shader, "a_TexPos");

	glEnableVertexAttribArray(attribPosition);
	glEnableVertexAttribArray(attribTexPos);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBOPosTex);
	glVertexAttribPointer(attribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, 0);
	glVertexAttribPointer(attribTexPos, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (GLvoid*)(3 * sizeof(float)));

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(attribPosition);
	glDisableVertexAttribArray(attribTexPos);
}

void Renderer::ProxyGeo()
{
	GLuint shader = m_Shader_ProxyGeo;

	glUseProgram(shader);

	GLuint projView = glGetUniformLocation(shader, "u_ProjView");
	glUniformMatrix4fv(projView, 1, GL_FALSE, &m_m4ProjView[0][0]);

	GLuint samplerTex = glGetUniformLocation(shader, "u_Tex");
	glUniform1i(samplerTex, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_Tex_Twice);

	GLuint uniformTime = glGetUniformLocation(shader, "u_Time");
	glUniform1f(uniformTime, g_time);
	g_time += 0.1;

	GLuint attribPos = glGetAttribLocation(shader, "a_Position");
	glEnableVertexAttribArray(attribPos);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_ProxyGeo);

	glVertexAttribPointer(attribPos, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glDrawArrays(GL_LINE_STRIP, 0, m_Count_ProxyGeo);

	glDisableVertexAttribArray(attribPos);
}

void Renderer::DrawParticle()
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO_P1);
	GLenum drawBuffers[2] = { GL_COLOR_ATTACHMENT0 , GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, drawBuffers);

	glClearColor(0, 0, 0, 1);
	glClearDepth(1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0, 0, 0, 0);

	GLuint shader = m_Shader_Particle;

	glUseProgram(shader);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	GLuint samplerTex = glGetUniformLocation(shader, "u_Texture");
	glUniform1i(samplerTex, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_Tex_Particle);

	GLuint uniformTime = glGetUniformLocation(shader, "u_Time");
	glUniform1f(uniformTime, g_time);
	g_time += 0.01;

	GLuint attribPos = glGetAttribLocation(shader, "a_Position");
	GLuint attribTex = glGetAttribLocation(shader, "a_TexPos");
	GLuint attribVel = glGetAttribLocation(shader, "a_Velocity");
	glEnableVertexAttribArray(attribPos);
	glEnableVertexAttribArray(attribTex);
	glEnableVertexAttribArray(attribVel);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Particle);

	glVertexAttribPointer(attribPos, 3, GL_FLOAT, GL_FALSE, sizeof(float)*9, 0);
	glVertexAttribPointer(attribTex, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 9, (GLvoid*)(sizeof(float)*3));
	glVertexAttribPointer(attribVel, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 9, (GLvoid*)(sizeof(float) * 5));

	glDrawArrays(GL_TRIANGLES, 0, m_Count_ProxyGeo);
	
	glDisableVertexAttribArray(attribPos);
	glDisableVertexAttribArray(attribTex);
	glDisableVertexAttribArray(attribVel);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::SetRotation(float rX, float rY)
{
	m_v3ModelRotation = glm::vec3(rX, rY, 0.f);
	m_m4ModelTranslation
		=
		glm::translate(glm::mat4(1.f), m_v3ModelTranslation);
	m_m4ModelRotation
		=
		glm::eulerAngleXYZ(m_v3ModelRotation.x, m_v3ModelRotation.y, m_v3ModelRotation.z);
	m_m4ModelScaling
		=
		glm::scale(glm::mat4(1.f), m_v3ModelScaling);
	m_m4Model
		= m_m4ModelTranslation*m_m4ModelRotation*m_m4ModelScaling;
}

void Renderer::Cube()
{
	GLuint shader = m_Shader_Proj;

	glUseProgram(shader);

	glEnable(GL_CULL_FACE);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	GLuint projView = glGetUniformLocation(shader, "u_ProjView");
	GLuint model = glGetUniformLocation(shader, "u_Model");
	GLuint rotation = glGetUniformLocation(shader, "u_Rotation");
	GLuint light = glGetUniformLocation(shader, "u_LightPos");
	GLuint camera = glGetUniformLocation(shader, "u_CameraPos");

	glUniformMatrix4fv(projView, 1, GL_FALSE, &m_m4ProjView[0][0]);
	glUniformMatrix4fv(model, 1, GL_FALSE, &m_m4Model[0][0]);
	glUniformMatrix4fv(rotation, 1, GL_FALSE, &m_m4ModelRotation[0][0]);
	glUniform3f(light, 2, 0, 0);
	glUniform3f(camera, m_v3Camera_Position.x,
		m_v3Camera_Position.y,
		m_v3Camera_Position.z);

	int attribPosition = glGetAttribLocation(shader, "a_Position");
	int attribNormal = glGetAttribLocation(shader, "a_Normal");
	int attribColor = glGetAttribLocation(shader, "a_Color");

	glEnableVertexAttribArray(attribPosition);
	glEnableVertexAttribArray(attribNormal);
	glEnableVertexAttribArray(attribColor);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Cube);

	glVertexAttribPointer(attribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 10, 0);
	glVertexAttribPointer(attribNormal, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (GLvoid*)(sizeof(float) * 3));
	glVertexAttribPointer(attribColor, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (GLvoid*)(sizeof(float) * 6));

	glDrawArrays(GL_TRIANGLES, 0, 36);

	glDisableVertexAttribArray(attribPosition);
	glDisableVertexAttribArray(attribNormal);
	glDisableVertexAttribArray(attribColor);
}

void Renderer::Test1()
{
	GLuint shader = m_Shader_Test1;

	glUseProgram(shader);

	GLuint tex = glGetUniformLocation(shader, "u_Texture");
	glUniform1i(tex, 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_Tex_Twice);

	GLuint pos = glGetAttribLocation(shader, "a_Position");
	GLuint texPos = glGetAttribLocation(shader, "a_TexPos");
	
	glEnableVertexAttribArray(pos);
	glEnableVertexAttribArray(texPos);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Test1);

	glVertexAttribPointer(pos, 3, GL_FLOAT,
		GL_FALSE, sizeof(float) * 5, 0);
	glVertexAttribPointer(texPos, 2, GL_FLOAT,
		GL_FALSE, sizeof(float) * 5, 
		(GLvoid*)(sizeof(float)*3));

	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Renderer::GenFBOs()
{
	glGenTextures(1, &m_Tex_P1_Color);
	glBindTexture(GL_TEXTURE_2D, m_Tex_P1_Color);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_WindowSizeX, m_WindowSizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	glGenTextures(1, &m_Tex_P1_Emissive);
	glBindTexture(GL_TEXTURE_2D, m_Tex_P1_Emissive);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_WindowSizeX, m_WindowSizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	glGenTextures(1, &m_Tex_P1_Depth);
	glBindTexture(GL_TEXTURE_2D, m_Tex_P1_Depth);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_WindowSizeX, m_WindowSizeY, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);

	glGenFramebuffers(1, &m_FBO_P1);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO_P1);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_Tex_P1_Color, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, m_Tex_P1_Emissive, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_Tex_P1_Depth, 0);

	GLenum error = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (error != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "bind framebuffer object error \n";
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glGenTextures(1, &m_Tex_P2_Bloom);
	glBindTexture(GL_TEXTURE_2D, m_Tex_P2_Bloom);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_WindowSizeX, m_WindowSizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	glGenTextures(1, &m_Tex_P2_Depth);
	glBindTexture(GL_TEXTURE_2D, m_Tex_P2_Depth);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_WindowSizeX, m_WindowSizeY, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);

	glGenFramebuffers(1, &m_FBO_P2);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO_P2);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_Tex_P2_Bloom, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_Tex_P2_Depth, 0);

	error = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (error != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "bind framebuffer object error \n";
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::Bogang()
{
	DrawParticle();
	BloomPass2(m_Tex_P1_Emissive);
	BloomPass3(m_Tex_P1_Color, m_Tex_P2_Bloom);
	DrawTexture(m_Tex_P1_Color, 0, 0, 100, 100);
	DrawTexture(m_Tex_P1_Emissive, 100, 0, 100, 100);
	DrawTexture(m_Tex_P1_Depth, 200, 0, 100, 100);
	DrawTexture(m_Tex_P2_Bloom, 300, 0, 100, 100);
}

void Renderer::BloomPass1()
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO_P1);
	GLenum drawBuffers[2] = { GL_COLOR_ATTACHMENT0 , GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, drawBuffers);

	glClearColor(0, 0, 0, 1);
	glClearDepth(1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0, 0, 0, 0);

	GLuint shader = m_Shader_Proj;

	glUseProgram(shader);

	glEnable(GL_CULL_FACE);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	GLuint projView = glGetUniformLocation(shader, "u_ProjView");
	GLuint model = glGetUniformLocation(shader, "u_Model");
	GLuint rotation = glGetUniformLocation(shader, "u_Rotation");
	GLuint light = glGetUniformLocation(shader, "u_LightPos");
	GLuint camera = glGetUniformLocation(shader, "u_CameraPos");
	GLuint emissive = glGetUniformLocation(shader, "u_Emissive");

	glUniformMatrix4fv(projView, 1, GL_FALSE, &m_m4ProjView[0][0]);
	glUniformMatrix4fv(model, 1, GL_FALSE, &m_m4Model[0][0]);
	glUniformMatrix4fv(rotation, 1, GL_FALSE, &m_m4ModelRotation[0][0]);
	glUniform3f(light, 0.5, 0.5, 3);
	glUniform3f(camera, m_v3Camera_Position.x,
		m_v3Camera_Position.y,
		m_v3Camera_Position.z);
	glUniform3f(emissive, 0.5, 0.5, 0.5);

	int attribPosition = glGetAttribLocation(shader, "a_Position");
	int attribNormal = glGetAttribLocation(shader, "a_Normal");
	int attribColor = glGetAttribLocation(shader, "a_Color");

	glEnableVertexAttribArray(attribPosition);
	glEnableVertexAttribArray(attribNormal);
	glEnableVertexAttribArray(attribColor);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Cube);

	glVertexAttribPointer(attribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 10, 0);
	glVertexAttribPointer(attribNormal, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (GLvoid*)(sizeof(float) * 3));
	glVertexAttribPointer(attribColor, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (GLvoid*)(sizeof(float) * 6));

	glDrawArrays(GL_TRIANGLES, 0, 36);

	glDisableVertexAttribArray(attribPosition);
	glDisableVertexAttribArray(attribNormal);
	glDisableVertexAttribArray(attribColor);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::BloomPass2(GLuint texEmissive)
{
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO_P2);
	GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, drawBuffers);

	glClearColor(0, 0, 0, 1);
	glClearDepth(1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0, 0, 0, 0);

	GLuint shader = m_Shader_Bloom_P2;

	glUseProgram(shader);

	int uniformSampler = glGetUniformLocation(shader, "u_TextureSlot");
	glUniform1i(uniformSampler, 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texEmissive);

	int attribPosition = glGetAttribLocation(shader, "a_Position");
	int attribTexPos = glGetAttribLocation(shader, "a_TexPos");

	glEnableVertexAttribArray(attribPosition);
	glEnableVertexAttribArray(attribTexPos);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBOPosTex);
	glVertexAttribPointer(attribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, 0);
	glVertexAttribPointer(attribTexPos, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (GLvoid*)(3 * sizeof(float)));

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(attribPosition);
	glDisableVertexAttribArray(attribTexPos);

	glViewport(0, 0, m_WindowSizeX, m_WindowSizeY);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::BloomPass3(GLuint baseTex, GLuint bloomTex)
{
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	GLuint shader = m_Shader_Bloom_P3;

	glUseProgram(shader);

	int uniformSampler1 = glGetUniformLocation(shader, "u_Texture0");
	int uniformSampler2 = glGetUniformLocation(shader, "u_Texture1");
	glUniform1i(uniformSampler1, 0);
	glUniform1i(uniformSampler2, 1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, baseTex);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, bloomTex);

	int attribPosition = glGetAttribLocation(shader, "a_Position");
	int attribTexPos = glGetAttribLocation(shader, "a_TexPos");

	glEnableVertexAttribArray(attribPosition);
	glEnableVertexAttribArray(attribTexPos);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBOPosTex);
	glVertexAttribPointer(attribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, 0);
	glVertexAttribPointer(attribTexPos, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (GLvoid*)(3 * sizeof(float)));

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(attribPosition);
	glDisableVertexAttribArray(attribTexPos);
}

void Renderer::DrawTexture(GLuint texID, GLuint x, GLuint y, GLuint width, GLuint height)
{
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glViewport(x, y, width, height);

	GLuint shader = m_TextureShader;

	glUseProgram(shader);

	int uniformSampler = glGetUniformLocation(shader, "u_TextureSlot");
	glUniform1i(uniformSampler, 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texID);

	int attribPosition = glGetAttribLocation(shader, "a_Position");
	int attribTexPos = glGetAttribLocation(shader, "a_TexPos");

	glEnableVertexAttribArray(attribPosition);
	glEnableVertexAttribArray(attribTexPos);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBOPosTex);
	glVertexAttribPointer(attribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, 0);
	glVertexAttribPointer(attribTexPos, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (GLvoid*)(3 * sizeof(float)));

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(attribPosition);
	glDisableVertexAttribArray(attribTexPos);

	glViewport(0, 0, m_WindowSizeX, m_WindowSizeY);
}

void Renderer::DrawHeightMap()
{
	GLuint shader = m_Shader_HeightMap;

	glUseProgram(shader);

	glEnable(GL_CULL_FACE);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	GLuint projView = glGetUniformLocation(shader, "u_ProjView");
	GLuint model = glGetUniformLocation(shader, "u_Model");
	GLuint rotation = glGetUniformLocation(shader, "u_Rotation");
	GLuint light = glGetUniformLocation(shader, "u_LightPos");
	GLuint camera = glGetUniformLocation(shader, "u_CameraPos");
	GLuint baseTex = glGetUniformLocation(shader, "u_TextureBase");
	GLuint detailTex = glGetUniformLocation(shader, "u_TextureDetail");

	glUniformMatrix4fv(projView, 1, GL_FALSE, &m_m4ProjView[0][0]);
	glUniformMatrix4fv(model, 1, GL_FALSE, &m_m4Model[0][0]);
	glUniformMatrix4fv(rotation, 1, GL_FALSE, &m_m4ModelRotation[0][0]);
	glUniform3f(light, 2, 0, 0);
	glUniform3f(camera, m_v3Camera_Position.x, m_v3Camera_Position.y, m_v3Camera_Position.z);
	glUniform1i(baseTex, 0);
	glUniform1i(detailTex, 1);

	int attribPosition = glGetAttribLocation(shader, "a_Position");
	int attribNormal = glGetAttribLocation(shader, "a_Normal");
	int attribTex = glGetAttribLocation(shader, "a_Tex");

	glEnableVertexAttribArray(attribPosition);
	glEnableVertexAttribArray(attribNormal);
	glEnableVertexAttribArray(attribTex);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_HeightMapVertices);

	glVertexAttribPointer(attribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, 0);
	glVertexAttribPointer(attribNormal, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (GLvoid*)(sizeof(float) * 3));
	glVertexAttribPointer(attribTex, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (GLvoid*)(sizeof(float) * 6));

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VBO_HeightMapIndices);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_Tex_HeightMap);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_Tex_HeightMapDetail);

	glLineWidth(1.f);
	//glDrawElements(GL_LINE_STRIP, m_Count_HeightMapIndices, GL_UNSIGNED_INT, 0);
	glDrawElements(GL_TRIANGLE_STRIP, m_Count_HeightMapIndices, GL_UNSIGNED_INT, 0);

	glDisableVertexAttribArray(attribPosition);
	glDisableVertexAttribArray(attribNormal);
	glDisableVertexAttribArray(attribTex);
}

void Renderer::DrawSkyBox()
{
	GLuint shader = m_Shader_SkyBox;
	
	glDepthMask(GL_FALSE);
	glDisable(GL_CULL_FACE);

	glUseProgram(shader);
	
	GLuint pvm = glGetUniformLocation(shader, "u_PVM");

	glm::mat4 mat4PVM = m_m4ProjView*m_m4Model;

	glUniformMatrix4fv(pvm, 1, GL_FALSE, &mat4PVM[0][0]);

	int attribPosition = glGetAttribLocation(shader, "a_Position");

	glEnableVertexAttribArray(attribPosition);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_SkyBox);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_Tex_Cube_SkyBox);

	glVertexAttribPointer(attribPosition, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glDrawArrays(GL_TRIANGLES, 0, 36);

	glDisableVertexAttribArray(attribPosition);
	glDepthMask(GL_TRUE);
}

void Renderer::LoadHeightMapImage(char* pFileName, int nWidth, int nHeight)
{
	BYTE *pHeightMapPixels = new BYTE[nWidth * nHeight];
	m_HeightMapImageWidth = nWidth;
	m_HeightMapImageHeight = nHeight;

	FILE* file;
	size_t readsize;
	fopen_s(&file, pFileName, "rb");

	if (!file)
	{
		std::cout << "height map raw file not exist. \n";
		return;
	}

	readsize = fread(pHeightMapPixels, 1, nWidth * nHeight * sizeof(BYTE), file);
	fclose(file);

	m_pHeightMapImage = new BYTE[nWidth * nHeight];
	for (int y = 0; y < nHeight; y++)
	{
		for (int x = 0; x < nWidth; x++)
		{
			m_pHeightMapImage[x + ((nHeight - 1 - y)*nWidth)] = pHeightMapPixels[x + (y*nWidth)];
		}
	}

	if (pHeightMapPixels) delete[] pHeightMapPixels;
}

glm::vec3 Renderer::GetHeightMapNormal(int x, int y)
{
	if ((x < 0.0f) || (y < 0.0f) || (x >= m_HeightMapImageWidth) || (y >= m_HeightMapImageHeight))
	{
		return glm::vec3(0, 0, 0);
	}

	int nHeightMapIndex = x + (y * m_HeightMapImageWidth);
	int xHeightMapAdd = (x < (m_HeightMapImageWidth - 1)) ? 1 : -1;
	int zHeightMapAdd = (y < (m_HeightMapImageHeight - 1)) ? m_HeightMapImageWidth : -m_HeightMapImageWidth;
	float y1 = (float)m_pHeightMapImage[nHeightMapIndex];
	float y2 = (float)m_pHeightMapImage[nHeightMapIndex + xHeightMapAdd];
	float y3 = (float)m_pHeightMapImage[nHeightMapIndex + zHeightMapAdd];

	glm::vec3 edge1 = glm::vec3(0.0f, y3 - y1, 1.f);
	glm::vec3 edge2 = glm::vec3(1.f, y2 - y1, 0.0f);
	glm::vec3 normal = glm::cross(edge1, edge2);

	return(normal);
}

void Renderer::InitializeHeightMapGridMesh(int nWidth, int nHeight)
{
	int nVertices = (m_HeightMapImageWidth) * (m_HeightMapImageHeight);
	int nStride = 3 + 3 + 2;
	int nFloats = nVertices * nStride;

	float* pVertices = new float[nFloats];
	
	float fHeight = 0.0f, fMinHeight = +FLT_MAX, fMaxHeight = -FLT_MAX;

	for (int i = 0, y = 0; y < nHeight; y++)
	{
		for (int x = 0; x < nWidth; x++, i++)
		{
			fHeight = m_pHeightMapImage[x + (y*nWidth)];

			pVertices[i*nStride + 0] = (float)x - (float)nWidth/2.f;
			pVertices[i*nStride + 1] = fHeight;
			pVertices[i*nStride + 2] = (float)y - (float)nHeight / 2.f;

			glm::vec3 nor = GetHeightMapNormal(x, y);
			pVertices[i*nStride + 3] = nor.x;
			pVertices[i*nStride + 4] = nor.y;
			pVertices[i*nStride + 5] = nor.z;

			pVertices[i*nStride + 6] = float(x) / float(m_HeightMapImageWidth - 1);
			pVertices[i*nStride + 7] = float(m_HeightMapImageHeight - 1 - y) / float(m_HeightMapImageHeight - 1);
		}
	}
	
	glGenBuffers(1, &m_VBO_HeightMapVertices);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_HeightMapVertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*nFloats, pVertices, GL_STATIC_DRAW);

	delete[] pVertices;

	int nIndices = ((nWidth * 2)*(nHeight - 1)) + ((nHeight - 1) - 1);
	m_Count_HeightMapIndices = nIndices;
	UINT *pIndices = new UINT[nIndices];

	for (int j = 0, z = 0; z < nHeight - 1; z++)
	{
		if ((z % 2) == 0)
		{
			for (int x = 0; x < nWidth; x++)
			{
				if ((x == 0) && (z > 0)) pIndices[j++] = (UINT)(x + (z * nWidth));
				pIndices[j++] = (UINT)(x + (z * nWidth));
				pIndices[j++] = (UINT)((x + (z * nWidth)) + nWidth);
			}
		}
		else
		{
			for (int x = nWidth - 1; x >= 0; x--)
			{
				if (x == (nWidth - 1)) pIndices[j++] = (UINT)(x + (z * nWidth));
				pIndices[j++] = (UINT)(x + (z * nWidth));
				pIndices[j++] = (UINT)((x + (z * nWidth)) + nWidth);
			}
		}
	}

	glGenBuffers(1, &m_VBO_HeightMapIndices);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VBO_HeightMapIndices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(UINT)*nIndices, pIndices, GL_STATIC_DRAW);

	delete[] pIndices;
}

void Renderer::InitializeHeightMap()
{
	char* filename = "./Textures/HeightMap/HeightMap.raw";
	int nWidth = 513;
	int nHeight = 513;

	//Compile HeightMap Shader
	m_Shader_HeightMap = CompileShaders("./Shaders/HeightMapShaders/HeightMap.vs", "./Shaders/HeightMapShaders/HeightMap.fs");

	//Load HeightMap
	LoadHeightMapImage(filename, nWidth, nHeight);

	//Generate HeightMap Mesh
	InitializeHeightMapGridMesh(nWidth, nHeight);

	//Load Textures
	m_Tex_HeightMap = CreatePngTexture("./Textures/HeightMap/world.png");
	m_Tex_HeightMapDetail = CreatePngTexture("./Textures/HeightMap/base.png");
	m_Tex_HeightMapWater = CreatePngTexture("./Textures/HeightMap/water.png");
}

void Renderer::InitializeSkyBox()
{
	//Compile SkyBox Shader
	m_Shader_SkyBox = CompileShaders("./Shaders/SkyBoxShaders/SkyBox.vs", "./Shaders/SkyBoxShaders/SkyBox.fs");


	float temp = 50.f;

	float cube[] = {
		-temp, -temp, temp,
		temp, temp, temp, 
		-temp, temp, temp,
		-temp, -temp, temp,
		temp, -temp, temp,
		temp, temp, temp, 

		temp, -temp, temp, 
		temp, temp, -temp, 
		temp, temp, temp,
		temp, -temp, temp, 
		temp, -temp, -temp,
		temp, temp, -temp, 

		-temp, temp, temp, 
		temp, temp, -temp, 
		-temp, temp, -temp,
		-temp, temp, temp, 
		temp, temp, temp, 
		temp, temp, -temp, 

		-temp, -temp, -temp,
		-temp, temp, -temp,
		temp, temp, -temp, 
		-temp, -temp, -temp,
		temp, temp, -temp, 
		temp, -temp, -temp,

		-temp, -temp, temp,
		-temp, temp, temp, 
		-temp, temp, -temp,
		-temp, -temp, temp,
		-temp, temp, -temp,
		-temp, -temp, -temp,

		-temp, -temp, temp, 
		temp, -temp, -temp, 
		temp, -temp, temp, 
		-temp, -temp, temp, 
		-temp, -temp, -temp,
		temp, -temp, -temp, 
	};

	glGenBuffers(1, &m_VBO_SkyBox);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_SkyBox);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube), cube, GL_STATIC_DRAW);

	//Load Textures
	glGenTextures(1, &m_Tex_Cube_SkyBox);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_Tex_Cube_SkyBox);

	char* filePath = "./Textures/SkyBox/SkyBox1/front.png";
	std::vector<unsigned char> imageFront;
	unsigned width, height;
	unsigned error = lodepng::decode(imageFront, width, height, filePath);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &imageFront[0]);

	filePath = "./Textures/SkyBox/SkyBox1/back.png";
	std::vector<unsigned char> imageBack;
	error = lodepng::decode(imageBack, width, height, filePath);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &imageBack[0]);

	filePath = "./Textures/SkyBox/SkyBox1/left.png";
	std::vector<unsigned char> imageLeft;
	error = lodepng::decode(imageLeft, width, height, filePath);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &imageLeft[0]);

	filePath = "./Textures/SkyBox/SkyBox1/right.png";
	std::vector<unsigned char> imageRight;
	error = lodepng::decode(imageRight, width, height, filePath);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &imageRight[0]);

	filePath = "./Textures/SkyBox/SkyBox1/top.png";
	std::vector<unsigned char> imageTop;
	error = lodepng::decode(imageTop, width, height, filePath);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &imageTop[0]);

	filePath = "./Textures/SkyBox/SkyBox1/bottom.png";
	std::vector<unsigned char> imageBottom;
	error = lodepng::decode(imageBottom, width, height, filePath);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &imageBottom[0]);

	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

glm::vec3 Renderer::MakingNormalizedLookVector(glm::vec3& eye, glm::vec3& object) {
	float diff_x = object.x - eye.x;
	float diff_y = object.y - eye.y;
	float diff_z = object.z - eye.z;

	float dinominator = sqrt(diff_x * diff_x + diff_y * diff_y + diff_z * diff_z);

	glm::vec3 norm_vec;
	norm_vec.x = diff_x / dinominator;
	norm_vec.y = diff_y / dinominator;
	norm_vec.z = diff_z / dinominator;

	return norm_vec;
}

float Renderer::GetVectorSize(glm::vec3& input_vec) {
	float return_value = sqrt(input_vec.x * input_vec.x + input_vec.y*input_vec.y + input_vec.z*input_vec.z);
	return return_value;
}