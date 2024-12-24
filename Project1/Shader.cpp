#include "Shader.h"

Shader::Shader(const std::string& vertexShaderPath, const std::string& fragmentShaderPath) {
	shaderProgram = createShader(vertexShaderPath, fragmentShaderPath);
}

Shader::~Shader() {
	glDeleteProgram(shaderProgram);
}

unsigned int Shader::compileShader(GLenum type, const std::string& source) {
	unsigned int shader = glCreateShader(type);
	const char* src = source.c_str();
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);

	int success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char infoLog[512];
		glGetShaderInfoLog(shader, 512, nullptr, infoLog);
		std::cerr << "Shader compilation failed:\n" << infoLog << std::endl;
	}

	return shader;
}

unsigned int Shader::createShader(const std::string& vertexShaderPath, const std::string& fragmentShaderPath) {
	std::ifstream vsFile(vertexShaderPath);
	std::ifstream fsFile(fragmentShaderPath);

	if (!vsFile.is_open() || !fsFile.is_open()) {
		std::cerr << "Failed to load shader files: " << vertexShaderPath << ", " << fragmentShaderPath << std::endl;
		return 0;
	}

	std::stringstream vsStream, fsStream;
	vsStream << vsFile.rdbuf();
	fsStream << fsFile.rdbuf();

	unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vsStream.str());
	unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fsStream.str());

	unsigned int program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	int success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		char infoLog[512];
		glGetProgramInfoLog(program, 512, nullptr, infoLog);
		std::cerr << "Shader program linking failed:\n" << infoLog << std::endl;
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return program;
}