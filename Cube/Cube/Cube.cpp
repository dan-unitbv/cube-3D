#include <iostream>
#include <fstream>
#include <sstream>

#include <stdlib.h>
#include <stdio.h>
#include <math.h> 

#include <GL/glew.h>
#include <GLM.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#pragma comment (lib, "glfw3dll.lib")
#pragma comment (lib, "glew32.lib")
#pragma comment (lib, "OpenGL32.lib")

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

enum ECameraMovementType
{
	UNKNOWN,
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	UP,
	DOWN
};

class Camera
{
private:
	const float zNEAR = 0.1f;
	const float zFAR = 500.f;
	const float YAW = -90.0f;
	const float PITCH = 0.0f;
	const float FOV = 45.0f;
	glm::vec3 startPosition;

public:
	Camera(const int width, const int height, const glm::vec3& position)
	{
		startPosition = position;
		Set(width, height, position);
	}

	void Set(const int width, const int height, const glm::vec3& position)
	{
		this->isPerspective = true;
		this->yaw = YAW;
		this->pitch = PITCH;

		this->FoVy = FOV;
		this->width = width;
		this->height = height;
		this->zNear = zNEAR;
		this->zFar = zFAR;

		this->worldUp = glm::vec3(0, 1, 0);
		this->position = position;

		lastX = width / 2.0f;
		lastY = height / 2.0f;
		bFirstMouseMove = true;

		UpdateCameraVectors();
	}

	void Reset(const int width, const int height)
	{
		Set(width, height, startPosition);
	}

	void Reshape(int windowWidth, int windowHeight)
	{
		width = windowWidth;
		height = windowHeight;

		glViewport(0, 0, windowWidth, windowHeight);
	}

	const glm::mat4 GetViewMatrix() const
	{
		return glm::lookAt(position, position + forward, up);
	}

	const glm::vec3 GetPosition() const
	{
		return position;
	}

	const glm::mat4 GetProjectionMatrix() const
	{
		glm::mat4 Proj = glm::mat4(1);
		if (isPerspective) {
			float aspectRatio = ((float)(width)) / height;
			Proj = glm::perspective(glm::radians(FoVy), aspectRatio, zNear, zFar);
		}
		else {
			float scaleFactor = 2000.f;
			Proj = glm::ortho<float>(
				-width / scaleFactor, width / scaleFactor,
				-height / scaleFactor, height / scaleFactor, -zFar, zFar);
		}
		return Proj;
	}

	void ProcessKeyboard(ECameraMovementType direction, float deltaTime)
	{
		float velocity = (float)(cameraSpeedFactor * deltaTime);

		switch (direction)
		{
		case ECameraMovementType::FORWARD:
			position += forward * velocity;
			break;
		case ECameraMovementType::BACKWARD:
			position -= forward * velocity;
			break;
		case ECameraMovementType::LEFT:
			position -= right * velocity;
			break;
		case ECameraMovementType::RIGHT:
			position += right * velocity;
			break;
		case ECameraMovementType::UP:
			position += up * velocity;
			break;
		case ECameraMovementType::DOWN:
			position -= up * velocity;
			break;
		}
	}

	void MouseControl(float xPos, float yPos)
	{
		if (bFirstMouseMove)
		{
			lastX = xPos;
			lastY = yPos;
			bFirstMouseMove = false;
		}

		float xChange = xPos - lastX;
		float yChange = lastY - yPos;

		lastX = xPos;
		lastY = yPos;

		if (fabs(xChange) <= 1e-6 && fabs(yChange) <= 1e-6)
		{
			return;
		}

		xChange *= mouseSensitivity;
		yChange *= mouseSensitivity;

		ProcessMouseMovement(xChange, yChange);
	}

	void ProcessMouseScroll(float yOffset)
	{
		if (FoVy >= 1.0f && FoVy <= 90.0f)
		{
			FoVy -= yOffset;
		}
		if (FoVy <= 1.0f)
			FoVy = 1.0f;
		if (FoVy >= 90.0f)
			FoVy = 90.0f;
	}

private:
	void ProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch = true)
	{
		yaw += xOffset;
		pitch += yOffset;

		if (constrainPitch)
		{
			if (pitch > 89.0f)
				pitch = 89.0f;
			if (pitch < -89.0f)
				pitch = -89.0f;
		}

		UpdateCameraVectors();
	}

	void UpdateCameraVectors()
	{
		this->forward.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		this->forward.y = sin(glm::radians(pitch));
		this->forward.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		this->forward = glm::normalize(this->forward);
		right = glm::normalize(glm::cross(forward, worldUp));
		up = glm::normalize(glm::cross(right, forward));
	}

protected:
	const float cameraSpeedFactor = 2.5f;
	const float mouseSensitivity = 0.1f;

	float zNear;
	float zFar;
	float FoVy;
	int width;
	int height;
	bool isPerspective;

	glm::vec3 position;
	glm::vec3 forward;
	glm::vec3 right;
	glm::vec3 up;
	glm::vec3 worldUp;

	float yaw;
	float pitch;

	bool bFirstMouseMove = true;
	float lastX = 0.f, lastY = 0.f;
};

class Shader
{
public:
	Shader(const char* vertexPath, const char* fragmentPath)
	{
		Init(vertexPath, fragmentPath);
	}

	~Shader()
	{
		glDeleteProgram(ID);
	}

	void Use() const
	{
		glUseProgram(ID);
	}

	unsigned int GetID() const
	{
		return ID;
	}

	unsigned int loc_model_matrix;
	unsigned int loc_view_matrix;
	unsigned int loc_projection_matrix;

	void SetVec3(const std::string& name, const glm::vec3& value) const
	{
		glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
	}
	void SetVec3(const std::string& name, float x, float y, float z) const
	{
		glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
	}
	void SetFloat(const std::string& name, float fValue) const
	{
		glUniform1f(glGetUniformLocation(ID, name.c_str()), fValue);
	}
	void SetMat4(const std::string& name, const glm::mat4& mat) const
	{
		glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
	}

private:
	void Init(const char* vertexPath, const char* fragmentPath)
	{
		std::string vertexCode;
		std::string fragmentCode;
		std::ifstream vShaderFile;
		std::ifstream fShaderFile;

		vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		try
		{
			vShaderFile.open(vertexPath);
			fShaderFile.open(fragmentPath);
			std::stringstream vShaderStream, fShaderStream;
			vShaderStream << vShaderFile.rdbuf();
			fShaderStream << fShaderFile.rdbuf();
			vShaderFile.close();
			fShaderFile.close();
			vertexCode = vShaderStream.str();
			fragmentCode = fShaderStream.str();
		}
		catch (std::ifstream::failure e)
		{
			std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
		}
		const char* vShaderCode = vertexCode.c_str();
		const char* fShaderCode = fragmentCode.c_str();

		unsigned int vertex, fragment;

		vertex = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex, 1, &vShaderCode, NULL);
		glCompileShader(vertex);
		CheckCompileErrors(vertex, "VERTEX");

		fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &fShaderCode, NULL);
		glCompileShader(fragment);
		CheckCompileErrors(fragment, "FRAGMENT");

		ID = glCreateProgram();
		glAttachShader(ID, vertex);
		glAttachShader(ID, fragment);
		glLinkProgram(ID);
		CheckCompileErrors(ID, "PROGRAM");

		glDeleteShader(vertex);
		glDeleteShader(fragment);
	}

	void CheckCompileErrors(unsigned int shader, std::string type)
	{
		GLint success;
		GLchar infoLog[1024];
		if (type != "PROGRAM")
		{
			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if (!success)
			{
				glGetShaderInfoLog(shader, 1024, NULL, infoLog);
				std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
			}
		}
		else
		{
			glGetProgramiv(shader, GL_LINK_STATUS, &success);
			if (!success)
			{
				glGetProgramInfoLog(shader, 1024, NULL, infoLog);
				std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
			}
		}
	}
private:
	unsigned int ID;
};

GLuint ProjMatrixLocation, ViewMatrixLocation, WorldMatrixLocation;
Camera* pCamera = nullptr;

unsigned int CreateTexture(const std::string& strTexturePath)
{
	unsigned int textureId = -1;

	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* data = stbi_load(strTexturePath.c_str(), &width, &height, &nrChannels, 0);
	if (data) {
		GLenum format;
		if (nrChannels == 1)
			format = GL_RED;
		else if (nrChannels == 3)
			format = GL_RGB;
		else if (nrChannels == 4)
			format = GL_RGBA;

		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else {
		std::cout << "Failed to load texture: " << strTexturePath << std::endl;
	}
	stbi_image_free(data);

	return textureId;
}

void Cleanup()
{
	delete pCamera;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

double deltaTime = 0.0f;
double lastFrame = 0.0f;

float ambientalValue = 0.5;
float diffuseValue = 0.5;
float specularValue = 0.5;
float specularExp = 2;

float constantAttenuation = 0.5f;
float linearAttenuation = 0.5f;
float squareAttenuation = 0.5f;

float objHeight = 1.0f;
glm::vec3 objMovement(0.f);
glm::vec3 objRotation(0.0f);
glm::vec3 objScale(1.f);
float radius = 0.1f;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// modelarea componentei ambientale

	if (key == GLFW_KEY_A && action == GLFW_PRESS)
	{
		if (ambientalValue < 1.0)
			ambientalValue += 0.1;
	}
	if (key == GLFW_KEY_Z && action == GLFW_PRESS)
	{
		if (ambientalValue > 0.0)
			ambientalValue -= 0.1;
	}

	// modelarea componentei difuze

	if (key == GLFW_KEY_D && action == GLFW_PRESS)
	{
		if (diffuseValue < 1.0)
			diffuseValue += 0.1;
	}
	if (key == GLFW_KEY_C && action == GLFW_PRESS)
	{
		if (diffuseValue > 0.0)
			diffuseValue -= 0.1;
	}

	// modelarea componentei speculare

	if (key == GLFW_KEY_S && action == GLFW_PRESS)
	{
		if (specularValue < 1.0)
			specularValue += 0.1;
	}
	if (key == GLFW_KEY_X && action == GLFW_PRESS)
	{
		if (specularValue > 0.0)
			specularValue -= 0.1;
	}

	// exponentul de reflexie speculara

	if (key == GLFW_KEY_E && action == GLFW_PRESS)
	{
		specularExp *= 2;
	}

	if (key == GLFW_KEY_F && action == GLFW_PRESS)
	{
		specularExp /= 2;
	}

	// dublarea inaltimii obiectului

	if (key == GLFW_KEY_9 && action == GLFW_PRESS)
	{
		objHeight *= 2;
	}

	// injumatatirea inaltimii obiectului

	if (key == GLFW_KEY_0 && action == GLFW_PRESS)
	{
		objHeight /= 2;
	}

	// marirea razei cercului

	if (key == GLFW_KEY_INSERT && action == GLFW_PRESS)
	{
		radius += 0.05f;
	}

	// micsorarea razei cercului

	if (key == GLFW_KEY_DELETE && action == GLFW_PRESS)
	{
		radius -= 0.05f;
	}

	if (key == GLFW_KEY_I && action == GLFW_PRESS)
	{
		squareAttenuation /= 0.05;
	}
	
	if (key == GLFW_KEY_P && action == GLFW_PRESS)
	{
		squareAttenuation *= 0.05;
	}
}

int main(int argc, char** argv)
{
	std::string strFullExeFileName = argv[0];
	std::string strExePath;
	const size_t last_slash_idx = strFullExeFileName.rfind('\\');
	if (std::string::npos != last_slash_idx) {
		strExePath = strFullExeFileName.substr(0, last_slash_idx);
	}

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "biletfeb23", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetKeyCallback(window, key_callback);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glewInit();

	glEnable(GL_DEPTH_TEST);

	float vertices[] = {
	  -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
	   0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
	   0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
	   0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
	   -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
	   -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,

	   -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
	   0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
	   0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
	   0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
	   -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
	   -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,

	   -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f,
	   -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f,
	   -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f,
	   -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f,
	   -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f,
	   -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f,

	   0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
	   0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
	   0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
	   0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
	   0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
	   0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,

	   -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,
	   0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,
	   0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,
	   0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,
	   -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,
	   -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,

	   -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
	   0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
	   0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
	   0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
	   -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
	   -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f
	};

	unsigned int VBO, cubeVAO;
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindVertexArray(cubeVAO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(2);

	unsigned int lightVAO;
	glGenVertexArrays(1, &lightVAO);
	glBindVertexArray(lightVAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	pCamera = new Camera(SCR_WIDTH, SCR_HEIGHT, glm::vec3(0.0, 0.0, 3.0));

	glm::vec3 lightPos(0.0f, 0.0f, 2.0f);

	Shader lightingShader("PhongLight.vs", "PhongLight.fs");
	Shader lampShader("Lamp.vs", "Lamp.fs");

	unsigned int floorTexture = CreateTexture(strExePath + "\\ColoredFloor.jpg");

	while (!glfwWindowShouldClose(window))
	{

		double currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		lightPos.x = radius * glm::sin(currentFrame);
		lightPos.y = radius * glm::cos(currentFrame);
		lightingShader.Use();
		lightingShader.SetVec3("objectColor", 0.5f, 1.0f, 0.31f);
		lightingShader.SetVec3("lightColor", 1.0f, 1.0f, 1.0f);
		lightingShader.SetVec3("lightPos", lightPos);
		lightingShader.SetFloat("aV", ambientalValue);
		lightingShader.SetFloat("dV", diffuseValue);
		lightingShader.SetFloat("sV", specularValue);
		lightingShader.SetFloat("sE", specularExp);
		lightingShader.SetFloat("constantAt", constantAttenuation);
		lightingShader.SetFloat("linearAt", linearAttenuation);
		lightingShader.SetFloat("squareAt", squareAttenuation);
		lightingShader.SetVec3("viewPos", pCamera->GetPosition());
		lightingShader.SetMat4("projection", pCamera->GetProjectionMatrix());
		lightingShader.SetMat4("view", pCamera->GetViewMatrix());

		glm::mat4 model = glm::scale(glm::mat4(1.0), glm::vec3(3.0f));
		model = glm::translate(model, objMovement);
		model = glm::rotate(model, glm::radians(objRotation.x), glm::vec3(1.f, 0.f, 0.f));
		model = glm::rotate(model, glm::radians(objRotation.y), glm::vec3(0.f, 1.f, 0.f));
		model = glm::rotate(model, glm::radians(objRotation.z), glm::vec3(0.f, 0.f, 1.f));
		model = glm::scale(model, glm::vec3(1.0f, objHeight, 1.0f));
		model = glm::scale(model, objScale);
		lightingShader.SetMat4("model", model);

		glBindVertexArray(cubeVAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		lampShader.Use();
		lampShader.SetMat4("projection", pCamera->GetProjectionMatrix());
		lampShader.SetMat4("view", pCamera->GetViewMatrix());
		model = glm::translate(glm::mat4(1.0), lightPos);
		model = glm::scale(model, glm::vec3(0.05f));
		lampShader.SetMat4("model", model);

		glBindVertexArray(lightVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		//glm::mat4 lightProjection, lightView;
		//glm::mat4 lightSpaceMatrix;
		//float near_plane = 1.0f, far_plane = 7.5f;
		//lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
		//lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
		//lightSpaceMatrix = lightProjection * lightView;

		//shadowMappingDepthShader.Use();
		//shadowMappingDepthShader.SetMat4("lightSpaceMatrix", lightSpaceMatrix);

		//glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		//glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		//glClear(GL_DEPTH_BUFFER_BIT);
		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, floorTexture);
		//glEnable(GL_CULL_FACE);
		//glCullFace(GL_FRONT);
		//renderScene(shadowMappingDepthShader);
		//glCullFace(GL_BACK);
		//glBindFramebuffer(GL_FRAMEBUFFER, 0);

		//glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//shadowMappingShader.Use();
		//glm::mat4 projection = pCamera->GetProjectionMatrix();
		//glm::mat4 view = pCamera->GetViewMatrix();
		//shadowMappingShader.SetMat4("projection", projection);
		//shadowMappingShader.SetMat4("view", view);
		//shadowMappingShader.SetVec3("viewPos", pCamera->GetPosition());
		//shadowMappingShader.SetVec3("lightPos", lightPos);
		//shadowMappingShader.SetMat4("lightSpaceMatrix", lightSpaceMatrix);
		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, floorTexture);
		//glActiveTexture(GL_TEXTURE1);
		//glBindTexture(GL_TEXTURE_2D, depthMap);
		//glDisable(GL_CULL_FACE);
		//renderScene(shadowMappingShader);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	Cleanup();

	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteVertexArrays(1, &lightVAO);
	glDeleteBuffers(1, &VBO);

	// delete pCamera;

	glfwTerminate();
	return 0;
}

void renderScene(const Shader& shader)
{
	glm::mat4 model;
	shader.SetMat4("model", model);
	// renderFloor();
}

unsigned int planeVAO = 0;
void renderFloor()
{
	unsigned int planeVBO;

	if (planeVAO == 0) {
		float planeVertices[] = {
			// positions            // normals         // texcoords
			25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
			-25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
			-25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,

			25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
			-25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,
			25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,  25.0f, 25.0f
		};
		glGenVertexArrays(1, &planeVAO);
		glGenBuffers(1, &planeVBO);
		glBindVertexArray(planeVAO);
		glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindVertexArray(0);
	}

	glBindVertexArray(planeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void processInput(GLFWwindow* window)
{
	// inchiderea aplicatiei

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);
	}

	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
	{
		pCamera->ProcessKeyboard(FORWARD, (float)deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
	{
		pCamera->ProcessKeyboard(BACKWARD, (float)deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
	{
		pCamera->ProcessKeyboard(LEFT, (float)deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
	{
		pCamera->ProcessKeyboard(RIGHT, (float)deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS)
	{
		pCamera->ProcessKeyboard(UP, (float)deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS)
	{
		pCamera->ProcessKeyboard(DOWN, (float)deltaTime);
	}

	// reset inapoi la fereastra initiala a camerei

	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
	{
		int width, height;
		glfwGetWindowSize(window, &width, &height);
		pCamera->Reset(width, height);
		objMovement = glm::vec3(0.0f, 0.01, 0.003f);
	}

	// rotatie in jurul axei OX a obiectului

	if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
	{
		objRotation += glm::vec3(.0f, -0.03f, 0.f);
	}
	if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
	{
		objRotation += glm::vec3(.0f, 0.03f, 0.f);
	}

	// rotatie in jurul axei OY a obiectului

	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
	{
		objRotation += glm::vec3(-.03f, 0.f, 0.f);
	}
	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
	{
		objRotation += glm::vec3(.03f, 0.f, 0.f);
	}

	// rotatie in jurul axei OZ a obiectului

	if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS)
	{
		objRotation += glm::vec3(.0f, 0.f, -0.03f);
	}
	if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS)
	{
		objRotation += glm::vec3(.0f, 0.f, 0.03f);
	}

	// scale fata de centrul de greutate al obietului

	if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS)
	{
		objScale *= glm::vec3(0.999f);
	}
	if (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS)
	{
		objScale *= glm::vec3(1.001f);
	}
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	pCamera->Reshape(width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	pCamera->MouseControl((float)xpos, (float)ypos);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yOffset)
{
	pCamera->ProcessMouseScroll((float)yOffset);
}
