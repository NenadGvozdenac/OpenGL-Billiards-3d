#ifndef TEXT_RENDER_H
#define TEXT_RENDER_H

#include <map>
#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>

struct Character {
    GLuint TextureID;
    glm::ivec2 Size;
    glm::ivec2 Bearing;
    GLuint Advance;
};

class TextRender {
public:
    TextRender(const std::string& fontPath, const std::string& vertexShaderPath, const std::string& fragmentShaderPath, int fontSize);

    TextRender(const TextRender& textRender) = delete;
    void RenderText(const std::string& text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color);
    ~TextRender();

private:
    GLuint VAO, VBO;
    GLuint shaderProgram;
    std::map<char, Character> Characters;

    unsigned int compileShader(GLenum type, const std::string& source);
    unsigned int createShader(const std::string& vertexShaderPath, const std::string& fragmentShaderPath);
    void loadCharacters(const std::string& fontPath, int fontSize);
};

#endif