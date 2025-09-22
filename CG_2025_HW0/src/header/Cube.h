#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <bits/stdc++.h>
using namespace std;
class Cube{
public:
    void draw();
    void change_color(int r, int g, int b);
    Cube();
private: 
    unsigned int VAO;
    glm::vec3 color;
};