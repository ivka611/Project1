#include <GL\glew.h>

#include <SDL.h>
#include <SDL_opengl.h>
#include <stdio.h>
#include <gl\GLU.h>

#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\matrix_inverse.hpp>
#include <glm\gtc\type_ptr.hpp>

#include <iostream>

#include "Shader.h"
#include "Camera.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <cmath>

bool init();
bool initGL();
void render();
GLuint CreateCube(float, GLuint&);
GLuint LoadTexture(const char* path);
void DrawCube(GLuint id);
void close();

//The window we'll be rendering to
SDL_Window* gWindow = NULL;

//OpenGL context
SDL_GLContext gContext;

//shaders
Shader gCubeShader;
Shader gLightShader;

//OpenGL ids
GLuint gVAO, gVBO;
GLuint gTexture;
GLuint gEmissionTexture;
GLuint gSpecularTexture;

// Текстурен масив за четирите сгради / фасади
unsigned int buildingTextures[4];

// camera
Camera camera(glm::vec3(0.0f, 1.1f, 2.0f));
float lastX = -1;
float lastY = -1;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// lighting

glm::vec3 lightPos(0.0f, 5.0f, -1.0f);
glm::vec3 lightPos2(0.0f, 8.0f, -5.0f);
glm::vec3 lightColor2(1.0f, 1.0f, 0.2f);

// структура и масив със сградите (добавен е textureIndex към всяка сграда)
struct Building {
	glm::vec3 position;
	glm::vec3 scale; // Размери по X (ширина), Y (височина), Z (дълбочина)
	int textureIndex;
};

// списък със сградите с коректно зададен индекс за текстурата (от 0 до 3)
Building district[] = {
	// --- ЛЕВА СТРАНА НА УЛИЦАТА ---
	{ glm::vec3(-1.5f, 1.5f,  0.0f),  glm::vec3(1.2f, 3.0f, 1.2f), 0 },  // Блок 1 -> atlas1.jpg
	{ glm::vec3(-1.5f, 1.0f, -2.0f),  glm::vec3(1.4f, 2.0f, 1.2f), 1 },  // Блок 2 -> atlas2.jpg
	{ glm::vec3(-1.5f, 1.25f, -4.0f), glm::vec3(1.2f, 2.5f, 1.2f), 2 },  // Блок 3 -> atlas3.jpg
	{ glm::vec3(-1.5f, 1.0f, -6.0f),  glm::vec3(1.5f, 2.0f, 1.2f), 3 },  // Блок 4 -> atlas4.jpg

	// --- ДЯСНА СТРАНА НА УЛИЦАТА ---
	{ glm::vec3(1.5f, 1.75f, 0.0f),  glm::vec3(1.4f, 3.5f, 1.2f), 1 },  // Блок 5 -> atlas2.jpg
	{ glm::vec3(1.5f, 1.0f, -2.0f),  glm::vec3(1.3f, 2.0f, 1.2f), 2 },  // Блок 6 -> atlas3.jpg
	{ glm::vec3(1.5f, 1.0f, -4.0f),  glm::vec3(1.5f, 2.0f, 1.2f), 3 },  // Блок 7 -> atlas4.jpg
	{ glm::vec3(1.5f, 1.25f, -6.0f), glm::vec3(1.2f, 2.5f, 1.2f), 0 }   // Блок 8 -> atlas1.jpg
};

// event handlers
void HandleKeyDown(const SDL_KeyboardEvent& key);
void HandleMouseMotion(const SDL_MouseMotionEvent& motion);
void HandleMouseWheel(const SDL_MouseWheelEvent& wheel);


//
float cameraAnimTime = 0.0f;
// Контролни точки за движение по улицата между сградите.
glm::vec3 p0(0.0f, 1.1f, 2.0f);
glm::vec3 p1(-0.35f, 1.1f, 0.0f);
glm::vec3 p2(0.35f, 1.1f, -2.0f);
glm::vec3 p3(0.0f, 1.1f, -4.0f);

// Функция за Де Кастелю (рекурсивна или поетапна линейна интерполация за 4 контролни точки)
glm::vec3 DeCasteljau(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float t)
{
	// Първо ниво на интерполация (между съседните точки)
	glm::vec3 q0 = glm::mix(p0, p1, t);
	glm::vec3 q1 = glm::mix(p1, p2, t);
	glm::vec3 q2 = glm::mix(p2, p3, t);

	// Второ ниво на интерполация
	glm::vec3 r0 = glm::mix(q0, q1, t);
	glm::vec3 r1 = glm::mix(q1, q2, t);

	// Финално ниво - крайната точка върху кривата за параметър t
	return glm::mix(r0, r1, t);
}

glm::vec3 BezierTangent(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float t)
{
	float oneMinusT = 1.0f - t;
	return 3.0f * oneMinusT * oneMinusT * (p1 - p0)
		+ 6.0f * oneMinusT * t * (p2 - p1)
		+ 3.0f * t * t * (p3 - p2);
}

int main(int argc, char* args[])
{
	if (!init())
	{
		return -1;
	}

	SDL_Event e;
	//While application is running
	bool quit = false;
	while (!quit)
	{
		// per-frame time logic
		// --------------------
		float currentFrame = SDL_GetTicks() / 1000.0f;
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		//Handle events on queue
		while (SDL_PollEvent(&e) != 0)
		{
			//User requests quit
			if (e.type == SDL_QUIT)
			{
				quit = true;
			}
			switch (e.type)
			{
			case SDL_QUIT:
				quit = true;
				break;
			case SDL_KEYDOWN:
				if (e.key.keysym.sym == SDLK_ESCAPE)
				{
					quit = true;
				}
				else {
					HandleKeyDown(e.key);
				}
				break;
			case SDL_MOUSEMOTION:
				HandleMouseMotion(e.motion);
				break;
			case SDL_MOUSEWHEEL:
				HandleMouseWheel(e.wheel);
				break;
			}
		}
		// Движение напред и назад по цялата крива с плавно ускоряване и спиране.
		cameraAnimTime += deltaTime * 0.35f;
		float curveT = (1.0f - cos(cameraAnimTime)) * 0.5f;
		camera.Position = DeCasteljau(p0, p1, p2, p3, curveT);
		camera.Front = glm::normalize(BezierTangent(p0, p1, p2, p3, curveT));
		//Render quad
		render();

		//Update screen
		SDL_GL_SwapWindow(gWindow);
	}

	close();

	return 0;
}

void HandleKeyDown(const SDL_KeyboardEvent& key)
{
	switch (key.keysym.sym)
	{
	case SDLK_w:
		camera.ProcessKeyboard(FORWARD, deltaTime);
		break;
	case SDLK_s:
		camera.ProcessKeyboard(BACKWARD, deltaTime);
		break;
	case SDLK_a:
		camera.ProcessKeyboard(LEFT, deltaTime);
		break;
	case SDLK_d:
		camera.ProcessKeyboard(RIGHT, deltaTime);
		break;
	}
}

void HandleMouseMotion(const SDL_MouseMotionEvent& motion)
{
	if (firstMouse)
	{
		lastX = motion.x;
		lastY = motion.y;
		firstMouse = false;
	}
	else
	{
		camera.ProcessMouseMovement(motion.x - lastX, lastY - motion.y);
		lastX = motion.x;
		lastY = motion.y;
	}
}

void HandleMouseWheel(const SDL_MouseWheelEvent& wheel)
{
	camera.ProcessMouseScroll(wheel.y);
}

bool init()
{
	//Initialization flag
	bool success = true;

	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
		success = false;
	}
	else
	{
		//Use OpenGL 3.3
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);


		//Create window
		gWindow = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480,
			SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN /*| SDL_WINDOW_FULLSCREEN*/);
		if (gWindow == NULL)
		{
			printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
			success = false;
		}
		else
		{
			//Create context
			gContext = SDL_GL_CreateContext(gWindow);
			if (gContext == NULL)
			{
				printf("OpenGL context could not be created! SDL Error: %s\n", SDL_GetError());
				success = false;
			}
			else
			{
				//Use Vsync
				if (SDL_GL_SetSwapInterval(1) < 0)
				{
					printf("Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError());
				}

				//Initialize OpenGL
				if (!initGL())
				{
					printf("Unable to initialize OpenGL!\n");
					success = false;
				}
			}
		}
	}

	return success;
}

bool initGL()
{
	bool success = true;
	GLenum error = GL_NO_ERROR;

	glewInit();

	error = glGetError();
	if (error != GL_NO_ERROR)
	{
		success = false;
		printf("Error initializing OpenGL! %s\n", gluErrorString(error));
	}

	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glEnable(GL_DEPTH_TEST);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	gCubeShader.Load("./shaders/cube.vert", "./shaders/cube.frag");
	gLightShader.Load("./shaders/light.vert", "./shaders/light.frag");

	gVAO = CreateCube(1.0f, gVBO);
	gTexture = LoadTexture("./container2.png");
	gEmissionTexture = LoadTexture("./emission.png");
	gSpecularTexture = LoadTexture("./container2_specular.png");
	gTexture = LoadTexture("./road.jpg");

	// ТУК СЕ ЗАРЕЖДАТ ТЕКСТУРИТЕ (преместено тук, защото е изпълним код)
	buildingTextures[0] = LoadTexture("atlas1.jpg");
	buildingTextures[1] = LoadTexture("atlas2.jpg");
	buildingTextures[2] = LoadTexture("atlas3.jpg");
	buildingTextures[3] = LoadTexture("atlas4.jpg");

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); //other modes GL_FILL, GL_POINT

	return success;
}

void close()
{
	//delete GL programs, buffers and objects
	glDeleteProgram(gCubeShader.ID);
	glDeleteVertexArrays(1, &gVAO);
	glDeleteBuffers(1, &gVBO);

	//Delete OGL context
	SDL_GL_DeleteContext(gContext);
	//Destroy window	
	SDL_DestroyWindow(gWindow);
	gWindow = NULL;

	//Quit SDL subsystems
	SDL_Quit();
}

void render()
{
	// Clear color buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 proj = glm::perspective(glm::radians(camera.Zoom), 4.0f / 3.0f, 0.1f, 100.0f);

	glUseProgram(gCubeShader.ID);

	// Общи настройки за камерата и осветлението
	gCubeShader.setMat4("view", view);
	gCubeShader.setMat4("proj", proj);
	gCubeShader.setVec3("light.diffuse", 1.0f, 1.0f, 1.0f);
	gCubeShader.setVec3("light.position", lightPos);
	gCubeShader.setVec3("light2.diffuse", lightColor2);
	gCubeShader.setVec3("light2.position", lightPos2);
	gCubeShader.setVec3("viewPos", camera.Position);

	// Material properties (направени неутрални и ярки, за да се виждат текстурите)
	gCubeShader.setVec3("material.ambient", 0.3f, 0.3f, 0.3f);
	gCubeShader.setVec3("material.diffuse", 1.0f, 1.0f, 1.0f);
	gCubeShader.setVec3("material.specular", 0.5f, 0.5f, 0.5f);
	gCubeShader.setFloat("material.shininess", 32.0f);

	// Material coefficients
	gCubeShader.setFloat("material.ka", 0.6f); // По-силна фонова осветеност
	gCubeShader.setFloat("material.kd", 1.0f);
	gCubeShader.setFloat("material.ks", 0.5f);

	// Допълнителни текстури (ако се ползват от шейдъра)
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gEmissionTexture);
	gCubeShader.setInt("emissionMap", 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gSpecularTexture);
	gCubeShader.setInt("specularMap", 2);

	// Подът използва фасадната текстура, повтаряща се по цялата повърхност.
	glm::mat4 groundModel = glm::mat4(1.0f);
	groundModel = glm::translate(groundModel, glm::vec3(0.0f, -0.08f, -2.0f));
	groundModel = glm::scale(groundModel, glm::vec3(7.0f, 0.16f, 14.0f));
	gCubeShader.setMat4("model", groundModel);
	gCubeShader.setMat3("normalMat", glm::inverseTranspose(glm::mat3(groundModel)));
	gCubeShader.setVec2("textureScale", 7.0f, 14.0f);
	gCubeShader.setFloat("material.ks", 0.0f);
	gCubeShader.setBool("unlitTexture", true);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTexture);
	gCubeShader.setInt("diffuseMap", 0);
	DrawCube(gVAO);

	gCubeShader.setBool("unlitTexture", false);
	gCubeShader.setVec2("textureScale", 1.0f, 1.0f);
	gCubeShader.setFloat("material.ks", 0.5f);

	// ЦИКЪЛ ЗА РИСУВАНЕ НА СГРАДИТЕ ОТ КВАРТАЛА
	for (int i = 0; i < 8; i++)
	{
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, district[i].position);
		model = glm::scale(model, district[i].scale);

		gCubeShader.use();
		gCubeShader.setMat4("model", model);
		gCubeShader.setMat3("normalMat", glm::inverseTranspose(glm::mat3(model)));
		gCubeShader.setMat4("view", view);
		gCubeShader.setMat4("proj", proj); // Поправено от "projection" на "proj", за да отговаря на дефинираната горе матрица

		// --- ЛОГИКА ЗА ТОЧКА 1 (Различни текстури за различните сгради) ---
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, buildingTextures[district[i].textureIndex]);
		gCubeShader.setInt("diffuseMap", 0);
		// ----------------------------------------------------------------

		DrawCube(gVAO); // Рисуване на куба
	}

	// Рисуване на лампите
	glUseProgram(gLightShader.ID);
	gLightShader.setMat4("projection", proj);
	gLightShader.setMat4("view", view);

	// Първа лампа
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, lightPos);
	model = glm::scale(model, glm::vec3(0.2f));
	gLightShader.setMat4("model", model);
	gLightShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
	DrawCube(gVAO);

	// Втора лампа
	model = glm::mat4(1.0f);
	model = glm::translate(model, lightPos2);
	model = glm::scale(model, glm::vec3(0.2f));
	gLightShader.setMat4("model", model);
	gLightShader.setVec3("lightColor", lightColor2);
	DrawCube(gVAO);
}

GLuint LoadTexture(const char* path)
{
	stbi_set_flip_vertically_on_load(true);
	int w, h, ch;
	unsigned char* data = stbi_load(path, &w, &h, &ch, 3);
	if (!data)
	{
		printf("Failed to load texture '%s': %s\n", path, stbi_failure_reason());
		return 0;
	}

	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	stbi_image_free(data);
	return tex;
}

GLuint CreateCube(float width, GLuint& VBO)
{
	//each side of the cube with its own vertices to use different normals
	float vertices[] = {
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

		0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
	};

	// позиция(3) + нормала(3) + uv за дифузната(2) + uv за емисията(2)
	float verts10[36 * 10];
	for (int i = 0; i < 36; i++)
	{
		float px = vertices[i * 6 + 0], py = vertices[i * 6 + 1], pz = vertices[i * 6 + 2];
		float nx = vertices[i * 6 + 3], ny = vertices[i * 6 + 4], nz = vertices[i * 6 + 5];
		float u, v;

		// пълната картинка на всяка стена
		if (fabs(nx) > 0.5f) {
			u = pz + 0.5f;
			v = py + 0.5f;
		}
		else if (fabs(ny) > 0.5f) {
			u = px + 0.5f;
			v = pz + 0.5f;
		}
		else {
			u = px + 0.5f;
			v = py + 0.5f;
		}

		//UV за емисията - пълната картинка на всяка стена
		float eu, ev;
		if (fabs(nx) > 0.5f) { eu = pz + 0.5f; ev = py + 0.5f; }
		else if (fabs(ny) > 0.5f) { eu = px + 0.5f; ev = pz + 0.5f; }
		else { eu = px + 0.5f; ev = py + 0.5f; }

		float* o = &verts10[i * 10];
		o[0] = px; o[1] = py; o[2] = pz;
		o[3] = nx; o[4] = ny; o[5] = nz;
		o[6] = u;  o[7] = v;
		o[8] = eu; o[9] = ev;
	}

	GLuint VAO;
	glGenBuffers(1, &VBO);
	glGenVertexArrays(1, &VAO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts10), verts10, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(8 * sizeof(float)));
	glEnableVertexAttribArray(3);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return VAO;
}

void DrawCube(GLuint vaoID)
{
	glBindVertexArray(vaoID);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}