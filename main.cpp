//
//  main.cpp
//  OpenGL Advances Lighting
//
//  Created by CGIS on 28/11/16.
//  Copyright � 2016 CGIS. All rights reserved.
//

#if defined (__APPLE__)
    #define GLFW_INCLUDE_GLCOREARB
    #define GL_SILENCE_DEPRECATION
#else
    #define GLEW_STATIC
    #include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>
#include <Windows.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Window.h"
#include "Shader.hpp"
#include "Model3D.hpp"
#include "Camera.hpp"
#include "Skybox.hpp"

#include <iostream>

int glWindowWidth = 1920;// 800;
int glWindowHeight = 1080;// 600;
int retina_width, retina_height;
float lastX = 400, lastY = 300;
float yaw = -90.0f, pitch;
bool firstMouse;
GLFWwindow* glWindow = NULL;
gps::Window myWindow;

const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;

glm::mat4 model;
GLuint modelLoc;
glm::mat4 view;
GLuint viewLoc;
glm::mat4 projection;
GLuint projectionLoc;
glm::mat3 normalMatrix;
GLuint normalMatrixLoc;
glm::mat4 lightRotation;

glm::vec3 lightDir;
GLuint lightDirLoc;
glm::vec3 lightColor;
GLuint lightColorLoc;

GLuint textureID;

gps::Camera myCamera(
	glm::vec3(-125.0f, 20.0f, 85.5f),
	glm::vec3(100.0f, 0.0f, -15.0f),
	glm::vec3(0.0f, 1.0f, 0.0f));
float cameraSpeed = 0.5f;

bool pressedKeys[1024];
float angleY = 0.0f;
GLfloat lightAngle;


float delta = 0;
float movementSpeed = 2; // units per second

double startTime = 0;
const double animationDuration = 15.0; 

double lastTimeStamp = glfwGetTime();

void updateDelta(double elapsedSeconds) {
	delta = delta + movementSpeed * elapsedSeconds;
}


gps::Model3D scene;
gps::Model3D boat;
gps::Model3D rain;

gps::Model3D nanosuit;
gps::Model3D ground;
gps::Model3D lightCube;
gps::Model3D screenQuad;

GLfloat showFog = 0.0f;

gps::Shader myCustomShader;
gps::Shader lightShader;
gps::Shader screenQuadShader;
gps::Shader depthMapShader;

GLuint shadowMapFBO; 
GLuint depthMapTexture;

gps::SkyBox mySkyBox;
gps::Shader skyboxShader;

glm::vec3 boatPos = glm::vec3(247.0f, -4.0f, -310.0f);
float boatSpeed = 0.1f;
glm::vec3 targetBoatPos = glm::vec3(220.0f, -4.0f, -270.0f);
bool boatMoving = false;

struct Raindrop {
	glm::vec3 position;
	glm::vec3 velocity;
};

std::vector<Raindrop> raindrops;

bool showDepthMap;
bool animation=false, applyFog=true, raining=false;

GLenum glCheckError_(const char *file, int line) {
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR)
	{
		std::string error;
		switch (errorCode)
		{
		case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
		case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
		case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
		case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	fprintf(stdout, "window resized to width: %d , and height: %d\n", width, height);
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

	myCustomShader.useShaderProgram();

	//set projection matrix
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	//send matrix data to shader
	GLint projLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	lightShader.useShaderProgram();

	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	//set Viewport transform
	glViewport(0, 0, retina_width, retina_height);
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_M && action == GLFW_PRESS)
		showDepthMap = !showDepthMap;
	if (key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action == GLFW_RELEASE)) {
		pressedKeys[GLFW_KEY_LEFT] = (action == GLFW_PRESS);
	}
	if (key == GLFW_KEY_RIGHT && (action == GLFW_PRESS || action == GLFW_RELEASE)) {
		pressedKeys[GLFW_KEY_RIGHT] = (action == GLFW_PRESS);
	}
	if (key == GLFW_KEY_UP && (action == GLFW_PRESS || action == GLFW_RELEASE)) {
		pressedKeys[GLFW_KEY_UP] = (action == GLFW_PRESS);
	}
	if (key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_RELEASE)) {
		pressedKeys[GLFW_KEY_DOWN] = (action == GLFW_PRESS);
	}
	if (key == GLFW_KEY_H && (action == GLFW_PRESS )) {
		raining = !raining;
	}
	if (key == GLFW_KEY_N && (action == GLFW_PRESS )) {

		boatMoving = !boatMoving;
	}
	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			pressedKeys[key] = true;
		else if (action == GLFW_RELEASE)
			pressedKeys[key] = false;
	}
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	animation = false;
	myCamera.rotate(pitch, yaw);
	view = myCamera.getViewMatrix();
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
}

void processMovement()
{
	if (pressedKeys[GLFW_KEY_Q]) {
		angleY -= 1.0f;		
	}

	if (pressedKeys[GLFW_KEY_E]) {
		angleY += 1.0f;		
	}

	if (pressedKeys[GLFW_KEY_U]) {
		lightAngle -= 1.0f;		
	}

	if (pressedKeys[GLFW_KEY_I]) {
		lightAngle += 1.0f;
	}

	if (pressedKeys[GLFW_KEY_W]) {
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);		
	}

	if (pressedKeys[GLFW_KEY_S]) {
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);		
	}

	if (pressedKeys[GLFW_KEY_A]) {
		myCamera.move(gps::MOVE_LEFT, cameraSpeed);		
	}

	if (pressedKeys[GLFW_KEY_D]) {
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);		
	}
	if (pressedKeys[GLFW_KEY_L]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	if (pressedKeys[GLFW_KEY_K]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	if (pressedKeys[GLFW_KEY_J]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	}
	if (pressedKeys[GLFW_KEY_LEFT]) {
		boatMoving = false;
		boatPos.x -= boatSpeed;
		//printf("boat pos.x : %f\n", boatPos.x);
	}
	if (pressedKeys[GLFW_KEY_RIGHT]) {
		boatMoving = false;
		boatPos.x += boatSpeed;
		//printf("boat pos.x : %f\n", boatPos.x);
	}
	if (pressedKeys[GLFW_KEY_UP]) {
		boatMoving = false;
		boatPos.z -= boatSpeed;
		//printf("boat pos.z :%f\n", boatPos.z);
	}

	if (pressedKeys[GLFW_KEY_DOWN]) {
		boatMoving = false;
		boatPos.z += boatSpeed;
		//printf("boat pos.z :%f\n", boatPos.z);
	}

	if (pressedKeys[GLFW_KEY_R]) {
		myCamera.reset();
		startTime = glfwGetTime();
		animation = true;
	}

	if (pressedKeys[GLFW_KEY_F])
	{
		myCustomShader.useShaderProgram();
		glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "applyFog"), true);

	}
	if (pressedKeys[GLFW_KEY_G])
	{
		myCustomShader.useShaderProgram();
		glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "applyFog"), false);

	};
	
}

bool initOpenGLWindow()
{
	if (!glfwInit()) {
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return false;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    
    //window scaling for HiDPI displays
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

    //for sRBG framebuffer
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

    //for antialising
    glfwWindowHint(GLFW_SAMPLES, 4);

	glWindow = glfwCreateWindow(glWindowWidth, glWindowHeight, "OpenGL Shader Example", NULL, NULL);
	if (!glWindow) {
		fprintf(stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return false;
	}

	glfwSetWindowSizeCallback(glWindow, windowResizeCallback);
	glfwSetKeyCallback(glWindow, keyboardCallback);
	glfwSetCursorPosCallback(glWindow, mouseCallback);
	glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwMakeContextCurrent(glWindow);

	glfwSwapInterval(1);

#if not defined (__APPLE__)
    // start GLEW extension handler
    glewExperimental = GL_TRUE;
    glewInit();
#endif

	// get version info
	const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString(GL_VERSION); // version as a string
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	//for RETINA display
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

	return true;
}

void initOpenGLState()
{
	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glViewport(0, 0, retina_width, retina_height);
	
	
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise

	glEnable(GL_FRAMEBUFFER_SRGB);
}

glm::mat4 computeLightSpaceTrMatrix()
{
	glm::mat4 lightView = glm::lookAt(glm::inverseTranspose(glm::mat3(lightRotation)) * lightDir, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	const GLfloat near_plane = 0.1f, far_plane = 500.0f;
	glm::mat4 lightProjection = glm::ortho(-100.0f, 100.0f, -100.0f, 100.0f, near_plane, far_plane);
	glm::mat4 lightSpaceTrMatrix = lightProjection * lightView;

	return lightSpaceTrMatrix;
}

void initObjects() {
	scene.LoadModel("objects/blender_scene/project.obj");
	boat.LoadModel("objects/blender_scene/boat.obj");
	rain.LoadModel("objects/blender_scene/rain.obj");
	
	//nanosuit.LoadModel("objects/nanosuit/nanosuit.obj");
	//ground.LoadModel("objects/ground/ground.obj");
	lightCube.LoadModel("objects/cube/cube.obj");
	//screenQuad.LoadModel("objects/quad/quad.obj");
}

void initSkybox() {
	std::vector<const GLchar*> faces;
	faces.push_back("skybox/right.tga");
	faces.push_back("skybox/left.tga");
	faces.push_back("skybox/top.tga");
	faces.push_back("skybox/bottom.tga");
	faces.push_back("skybox/back.tga");
	faces.push_back("skybox/front.tga");
	mySkyBox.Load(faces);
}

void initShaders() {
	myCustomShader.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
	myCustomShader.useShaderProgram();
	lightShader.loadShader("shaders/lightCube.vert", "shaders/lightCube.frag");
	lightShader.useShaderProgram();
	screenQuadShader.loadShader("shaders/screenQuad.vert", "shaders/screenQuad.frag");
	screenQuadShader.useShaderProgram();
	skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
	skyboxShader.useShaderProgram();
	depthMapShader.loadShader("shaders/depthMap.vert", "shaders/depthMap.frag");
	depthMapShader.useShaderProgram();
}

void initUniforms() {
	myCustomShader.useShaderProgram();



	model = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	modelLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myCustomShader.shaderProgram, "view");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	normalMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "normalMatrix");
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	
	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	projectionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(0.0f, 1.0f, 1.0f);
	lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
	lightDirLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDir");	
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

	//set light color
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

	lightShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	
}

void initFBO() {

	glGenFramebuffers(1, &shadowMapFBO);

	//create depth texture for FBO
	glGenTextures(1, &depthMapTexture);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	//attach texture to FBO
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture,
		0);

	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void drawObjects(gps::Shader shader, bool depthPass) {
		
	shader.useShaderProgram();

	model = glm::rotate(glm::mat4(1.0f), glm::radians(angleY), glm::vec3(0.0f, 1.0f, 0.0f));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	// do not send the normal matrix if we are rendering in the depth map
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}

	scene.Draw(shader);

	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(0.5f));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	// do not send the normal matrix if we are rendering in the depth map
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}

	ground.Draw(shader);
}

void renderBoat(gps::Shader shader) {
	shader.useShaderProgram();
	glm::mat4 boatModel = glm::translate(glm::mat4(1.0f), boatPos);
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(boatModel));
	boat.Draw(shader);

}

void initializeRaindrops() {
	
	for (int i = 0; i < 10000; ++i) {
		Raindrop raindrop;
		raindrop.position = glm::vec3(
			static_cast<float>(rand() % 400 - 200),  
			static_cast<float>(rand() % 50 + 50),
			static_cast<float>(rand() % 400 - 200)

		);
		//raindrop.velocity = glm::vec3(0.0f, -1.0f, 0.0f);  // Falling straight down
		raindrop.velocity = glm::vec3(0.0f, -1.7f, 0.0f);  
		float angle = glm::radians(45.0f);
		float speed = 0.1f;
		//raindrop.velocity = glm::vec3(glm::cos(angle) * speed, -1.0f, glm::sin(angle) * speed);



		raindrops.push_back(raindrop);
	}
}

void renderRaindrops(gps::Shader shader) {
	shader.useShaderProgram();

	for (const auto& raindrop : raindrops) {
		glm::mat4 raindropModelMatrix = glm::translate(glm::mat4(1.0f), raindrop.position);
		glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(raindropModelMatrix));

		// Render the raindrop model or particle (replace with your rendering logic)
		rain.Draw(shader);
	}
}

void updateRaindrops(float deltaTime) {
	for (auto& raindrop : raindrops) {
		// Update raindrop position based on velocity and time
		raindrop.position += raindrop.velocity;// *deltaTime;


		// Check if raindrop hit the ground (replace -4.0f with the actual ground level)
		if (raindrop.position.y < -4.0f) {
			// Reset raindrop position to the top
			raindrop.position = glm::vec3(
				static_cast<float>(rand() % 400 - 200),
				static_cast<float>(rand() % 50 + 50),
				static_cast<float>(rand() % 400 - 200)
			);
		}
	}
}

void updateBoatPos()
{
	if (pressedKeys[GLFW_KEY_UP]) {
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_DOWN]) {
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
	}
}

void renderObjects(gps::Shader shader, bool depthPass) {
	// select active shader program
	shader.useShaderProgram();

	//send teapot model matrix data to shader
	modelLoc = glGetUniformLocation(shader.shaderProgram, "model");
	

	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	// do not send the normal matrix if we are rendering in the depth map
	if (!depthPass) {

		normalMatrixLoc = glGetUniformLocation(shader.shaderProgram, "normalMatrix");
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
		lightDirLoc = glGetUniformLocation(shader.shaderProgram, "lightDir");
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view)) * lightDir));

	}

	// draw teapot
	scene.Draw(shader);


}

void renderScene() {

	// depth maps creation pass
	//TODO - Send the light-space transformation matrix to the depth map creation shader and
	//		 render the scene in the depth map


	// render depth map on screen - toggled with the M key
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	depthMapShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"),1,GL_FALSE,glm::value_ptr(computeLightSpaceTrMatrix()));
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	drawObjects(depthMapShader, false);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (showDepthMap) {
		glViewport(0, 0, retina_width, retina_height);

		glClear(GL_COLOR_BUFFER_BIT);

		screenQuadShader.useShaderProgram();

		//bind the depth map
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depthMapTexture);
		glUniform1i(glGetUniformLocation(screenQuadShader.shaderProgram, "depthMap"), 0);

		glDisable(GL_DEPTH_TEST);
		screenQuad.Draw(screenQuadShader);
		glEnable(GL_DEPTH_TEST);
	}
	else {
		
		// final scene rendering pass (with shadows)
		glViewport(0, 0, retina_width, retina_height);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		myCustomShader.useShaderProgram();

		view = myCamera.getViewMatrix();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

		lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

		//bind the shadow map
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, depthMapTexture);
		glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "shadowMap"), 3);

		glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightSpaceTrMatrix"),
			1,
			GL_FALSE,
			glm::value_ptr(computeLightSpaceTrMatrix()));

		drawObjects(myCustomShader, false);

		//draw a white cube around the light

		lightShader.useShaderProgram();

		glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

		model = lightRotation;
		model = glm::translate(model, 400.0f * lightDir);
		model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
		glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

		lightCube.Draw(lightShader);
		mySkyBox.Draw(skyboxShader, view, projection);
	
		//bind the shadow map

		//draw a white cube around the light

		/*
		lightShader.useShaderProgram();

		glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

		model = lightRotation;
		model = glm::translate(model, 1.0f * lightDir);
		model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
		glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

		lightCube.Draw(lightShader);
		*/

		
		mySkyBox.Draw(skyboxShader, view, projection);
	}
}

void cleanup() {
	glDeleteTextures(1,& depthMapTexture);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &shadowMapFBO);
	glfwDestroyWindow(glWindow);
	//close GL context and any other GLFW resources
	glfwTerminate();
}

int main(int argc, const char * argv[]) {

	if (!initOpenGLWindow()) {
		glfwTerminate();
		return 1;
	}

	initOpenGLState();
	initObjects();
	initShaders();
	initUniforms();
	initFBO();
	initSkybox();
	initializeRaindrops();

	glCheckError();

	while (!glfwWindowShouldClose(glWindow)) {
		processMovement();
		if (animation) {
			double currentTimeStamp = glfwGetTime();
			double elapsedTime = currentTimeStamp - startTime;

			// movements and rotations with interpolation
			double t = std::min(1.0, elapsedTime / animationDuration);

			float movementOffset = 2.0f;
			float rotationOffset = 600.0f;

			if (t < 0.4) {
				myCamera.move(gps::MOVE_LEFT, cameraSpeed * t * movementOffset);
				float rotationAngle = glm::radians(-rotationOffset) * t * 14.0f; 
				myCamera.rotate(0.0f, rotationAngle);
			}

			if (t < 0.8) {
				myCamera.move(gps::MOVE_RIGHT, cameraSpeed * t * movementOffset);
				float rotationAngle = glm::radians(-rotationOffset) * t * 14.0f; 
				myCamera.rotate(0.0f, rotationAngle);
			}
			else {
				
				myCamera.move(gps::MOVE_BACKWARD, cameraSpeed * (1.0 - t) * movementOffset);
				float rotationAngle = glm::radians(-rotationOffset) * (t) * 14.0f; 
				myCamera.rotate(0.0f, rotationAngle);
				
			}


			glfwPollEvents();

			// Check if the animation should stop
			if (t >= 1.0) {
				animation = false;
			}
		}


		if (boatMoving) {
			float animationSpeed = 0.05f;
			if (boatPos.x + 0.1 > 220.0f && boatPos.x - 0.1 < 220.0f)
			{
				boatPos.z = glm::mix(boatPos.z, targetBoatPos.z, animationSpeed);
				if (boatPos.z + 0.1 > -270.0f && boatPos.z - 0.1 < -270.0f) {
					targetBoatPos = glm::vec3(253.0f, -4.0f, -270.0f);
				}

			}
			boatPos.x = glm::mix(boatPos.x, targetBoatPos.x, animationSpeed);

		}

		renderScene();
		renderBoat(myCustomShader);

		if (raining) {
			updateDelta(glfwGetTime() - lastTimeStamp);
			lastTimeStamp = glfwGetTime();
			updateRaindrops(delta);
			renderRaindrops(myCustomShader);
		}


		glfwPollEvents();
		glfwSwapBuffers(glWindow);
	}

	cleanup();

	return 0;
}
