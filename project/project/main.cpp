/*
Student Information
Student ID: 1155124490
Student Name: YU Yue
*/
#include <string>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Dependencies/stb_image/stb_image.h"

#include "Shader.h"
#include "Texture.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <map>


// screen setting
const int SCR_WIDTH = 1920;
const int SCR_HEIGHT = 1080;

//camera views
glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f,  100.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);

GLfloat lastX = SCR_WIDTH / 2.0;
GLfloat lastY = SCR_HEIGHT / 2.0;
GLfloat fov = 45.0f;
glm::vec3 cameraPosBefore;

//shader
Shader shader;
Shader skyboxShader;

bool firstMouse = true;
bool firstClick = true;
bool mouseLeftDown = false;

// struct for storing the obj file
struct Vertex {
    glm::vec3 position;
    glm::vec2 uv;
    glm::vec3 normal;
};

struct Model {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

Model loadOBJ(const char* objPath)
{
    // function to load the obj file
    // Note: this simple function cannot load all obj files.

    struct V {
        // struct for identify if a vertex has showed up
        unsigned int index_position, index_uv, index_normal;
        bool operator == (const V& v) const {
            return index_position == v.index_position && index_uv == v.index_uv && index_normal == v.index_normal;
        }
        bool operator < (const V& v) const {
            return (index_position < v.index_position) ||
                (index_position == v.index_position && index_uv < v.index_uv) ||
                (index_position == v.index_position && index_uv == v.index_uv && index_normal < v.index_normal);
        }
    };

    std::vector<glm::vec3> temp_positions;
    std::vector<glm::vec2> temp_uvs;
    std::vector<glm::vec3> temp_normals;

    std::map<V, unsigned int> temp_vertices;

    Model model;
    unsigned int num_vertices = 0;

    std::cout << "\nLoading OBJ file " << objPath << "..." << std::endl;

    std::ifstream file;
    file.open(objPath);

    // Check for Error
    if (file.fail()) {
        std::cerr << "Impossible to open the file! Do you use the right path? See Tutorial 6 for details" << std::endl;
        exit(1);
    }

    while (!file.eof()) {
        // process the object file
        char lineHeader[128];
        file >> lineHeader;

        if (strcmp(lineHeader, "v") == 0) {
            // geometric vertices
            glm::vec3 position;
            file >> position.x >> position.y >> position.z;
            temp_positions.push_back(position);
        }
        else if (strcmp(lineHeader, "vt") == 0) {
            // texture coordinates
            glm::vec2 uv;
            file >> uv.x >> uv.y;
            temp_uvs.push_back(uv);
        }
        else if (strcmp(lineHeader, "vn") == 0) {
            // vertex normals
            glm::vec3 normal;
            file >> normal.x >> normal.y >> normal.z;
            temp_normals.push_back(normal);
        }
        else if (strcmp(lineHeader, "f") == 0) {
            // Face elements
            V vertices[3];
            for (int i = 0; i < 3; i++) {
                char ch;
                file >> vertices[i].index_position >> ch >> vertices[i].index_uv >> ch >> vertices[i].index_normal;
            }

            // Check if there are more than three vertices in one face.
            std::string redundency;
            std::getline(file, redundency);
            if (redundency.length() >= 5) {
                std::cerr << "There may exist some errors while load the obj file. Error content: [" << redundency << " ]" << std::endl;
                std::cerr << "Please note that we only support the faces drawing with triangles. There are more than three vertices in one face." << std::endl;
                std::cerr << "Your obj file can't be read properly by our simple parser :-( Try exporting with other options." << std::endl;
                exit(1);
            }

            for (int i = 0; i < 3; i++) {
                if (temp_vertices.find(vertices[i]) == temp_vertices.end()) {
                    // the vertex never shows before
                    Vertex vertex;
                    vertex.position = temp_positions[vertices[i].index_position - 1];
                    vertex.uv = temp_uvs[vertices[i].index_uv - 1];
                    vertex.normal = temp_normals[vertices[i].index_normal - 1];

                    model.vertices.push_back(vertex);
                    model.indices.push_back(num_vertices);
                    temp_vertices[vertices[i]] = num_vertices;
                    num_vertices += 1;
                }
                else {
                    // reuse the existing vertex
                    unsigned int index = temp_vertices[vertices[i]];
                    model.indices.push_back(index);
                }
            } // for
        } // else if
        else {
            // it's not a vertex, texture coordinate, normal or face
            char stupidBuffer[1024];
            file.getline(stupidBuffer, 1024);
        }
    }
    file.close();

    std::cout << "There are " << num_vertices << " vertices in the obj file.\n" << std::endl;
    return model;
}

void get_OpenGL_info()
{
    // OpenGL information
    const GLubyte* name = glGetString(GL_VENDOR);
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* glversion = glGetString(GL_VERSION);
    std::cout << "OpenGL company: " << name << std::endl;
    std::cout << "Renderer name: " << renderer << std::endl;
    std::cout << "OpenGL version: " << glversion << std::endl;
}

//objects
GLuint skyboxVAO, skyboxVBO;
std::vector<const GLchar *> faces;
GLuint cubemapTexture;


GLuint spacecraftVAO, spacecraftVBO, spacecraftEBO;
Model spacecraftObj;
Texture spacecraftTexture;

GLuint planetVAO, planetVBO, planetEBO;
Model planetObj;
Texture planetTexture[2];

GLuint loadCubemap(std::vector<const GLchar *> faces)
{
    
    GLuint textureID;
    glGenTextures(1, &textureID);

    int width, height, BPP;
    unsigned char* image;
    
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    for(GLuint i = 0; i < faces.size(); i++)
    {
        image = stbi_load(faces[i], &width, &height, &BPP, 0);
        if(image)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
            std::cout << "Load " << faces[i] << " successfully!" << std::endl;
        }
        else {
            std::cout << "Failed to load texture: " << faces[i] << std::endl;
            stbi_image_free(image);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    return textureID;
}

void sendDataToOpenGL()
{
    //TODO
    //Load objects and bind to VAO and VBO
    //Load textures
    
    GLfloat skyboxVertices[] =
    {
        // Positions
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
      
        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
      
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
       
        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
      
        -1.0f,  1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,
      
        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f,  1.0f
    };
    
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glBindVertexArray(0);
    
    faces.push_back("./CourseProjectMaterials/texture/skybox/orbital-element_rt.bmp");
    faces.push_back("./CourseProjectMaterials/texture/skybox/orbital-element_lf.bmp");
    faces.push_back("./CourseProjectMaterials/texture/skybox/orbital-element_up.bmp");
    faces.push_back("./CourseProjectMaterials/texture/skybox/orbital-element_dn.bmp");
    faces.push_back("./CourseProjectMaterials/texture/skybox/orbital-element_bk.bmp");
    faces.push_back("./CourseProjectMaterials/texture/skybox/orbital-element_ft.bmp");
    cubemapTexture = loadCubemap(faces);
    
    spacecraftObj = loadOBJ("./CourseProjectMaterials/object/spacecraft.obj");
    
    spacecraftTexture.setupTexture("./CourseProjectMaterials/texture/spacecraftTexture.bmp");
    //vertex array object
    glGenVertexArrays(1, &spacecraftVAO);
    glBindVertexArray(spacecraftVAO);
    
    //vertex buffer object
    glGenBuffers(1, &spacecraftVBO);
    glBindBuffer(GL_ARRAY_BUFFER, spacecraftVBO);
    glBufferData(GL_ARRAY_BUFFER, spacecraftObj.vertices.size() * sizeof(Vertex), &spacecraftObj.vertices[0], GL_STATIC_DRAW);
    //vertex position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    //vertex texture coordinate
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    //vertex normal
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    //index buffer
    glGenBuffers(1, &spacecraftEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, spacecraftEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, spacecraftObj.indices.size() * sizeof(unsigned int), &spacecraftObj.indices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    
    planetObj = loadOBJ("./CourseProjectMaterials/object/planet.obj");
    
    planetTexture[0].setupTexture("./CourseProjectMaterials/texture/earthTexture.bmp");
    planetTexture[1].setupTexture("./CourseProjectMaterials/texture/earthNormal.bmp");
    //vertex array object
    glGenVertexArrays(1, &planetVAO);
    glBindVertexArray(planetVAO);
    
    //vertex buffer object
    glGenBuffers(1, &planetVBO);
    glBindBuffer(GL_ARRAY_BUFFER, planetVBO);
    glBufferData(GL_ARRAY_BUFFER, planetObj.vertices.size() * sizeof(Vertex), &planetObj.vertices[0], GL_STATIC_DRAW);
    //vertex position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    //vertex texture coordinate
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    //vertex normal
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    //index buffer
    glGenBuffers(1, &planetEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planetEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, planetObj.indices.size() * sizeof(unsigned int), &planetObj.indices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    
    
    
}

void initializedGL(void) //run only once
{
    if (glewInit() != GLEW_OK) {
        std::cout << "GLEW not OK." << std::endl;
    }

    get_OpenGL_info();
    sendDataToOpenGL();

    //TODO: set up the camera parameters
    //TODO: set up the vertex shader and fragment shader
    
    shader.setupShader("VertexShaderCode.glsl", "FragmentShaderCode.glsl");
    skyboxShader.setupShader("SkyboxVertexShader.glsl", "SkyboxFragmentShader.glsl");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

void setupLight(Shader lshader)
{
    lshader.setVec3("eyePositionWorld", cameraPos);
    
    glm::vec3 point_ambientLight = glm::vec3(0.5f, 0.5f, 0.5f);
    lshader.setVec3("point_ambientLight", point_ambientLight);
    glm::vec3 point_lightPos = glm::vec3(0.0f, 100.0f, 0.0f);
    lshader.setVec3("point_lightPos", point_lightPos);
}

void paintGL(void)  //always run
{
    glClearColor(1.0f, 1.0f, 1.0f, 0.5f); //specify the background color, this is just an example
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //TODO:
    //Set lighting information, such as position and color of lighting source
    //Set transformation matrix
    //Bind different textures
    
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);
    
    view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    projection = glm::perspective(glm::radians(fov), (GLfloat)SCR_WIDTH / (GLfloat)SCR_HEIGHT,0.1f, 500.0f);
    
    glDepthMask(GL_FALSE);// Remember to turn depth writing off
    skyboxShader.use();

    model = glm::scale(model, glm::vec3(200.0f, 200.0f, 200.0f));
    skyboxShader.setMat4("model", model);
    skyboxShader.setMat4("view", view);
    skyboxShader.setMat4("projection", projection);
    // skybox cube
    glBindVertexArray(skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    
    model = glm::mat4(1.0f);
    shader.use();
    setupLight(shader);

    shader.setMat4("model", model);
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);
    shader.setInt("normalMapping_flag", 0);
    
    shader.setInt("Texture", 0);
    shader.setInt("NormalMapping", 1);
    shader.setInt("Gold", 2);
    
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 80.0f));
    model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::translate(model, glm::vec3(0.0f, -200.0f, 0.0f));
    shader.setMat4("model", model);
    spacecraftTexture.bind(0);
    glBindVertexArray(spacecraftVAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, spacecraftEBO);
    glDrawElements(GL_TRIANGLES, spacecraftObj.indices.size(), GL_UNSIGNED_INT, 0);
    
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, -80.0f));
    model = glm::scale(model, glm::vec3(4.0f, 4.0f, 4.0f));
    model = glm::rotate(model, (GLfloat)glfwGetTime() * (glm::radians(30.0f)), glm::vec3(0.0f, 1.0f, 0.0f));
    shader.setMat4("model", model);
    shader.setInt("normalMapping_flag", 1);
    planetTexture[0].bind(0);
    planetTexture[1].bind(1);
    glBindVertexArray(planetVAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planetEBO);
    glDrawElements(GL_TRIANGLES, planetObj.indices.size(), GL_UNSIGNED_INT, 0);
    shader.setInt("normalMapping_flag", 0);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    // Sets the mouse-button callback for the current window.
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            firstClick = true;
            mouseLeftDown = true;
        }
        else if (action == GLFW_RELEASE)
        {
            firstMouse = true;
            mouseLeftDown = false;
        }
    }
}

void cursor_position_callback(GLFWwindow* window, double x, double y)
{
    // Sets the cursor position callback for the current window
    if (!mouseLeftDown) {
        if (firstMouse)
        {
            lastX = x;
            lastY = y;
            cameraPosBefore = cameraPos;
            firstMouse = false;
        }

        GLfloat xoffset = x - lastX;
        GLfloat yoffset = lastY - y;

        GLfloat sensitivity = 0.01;    // Change this value to your liking
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        GLfloat cameraPosLen = glm::sqrt(cameraPosBefore.x * cameraPosBefore.x + cameraPosBefore.y * cameraPosBefore.y + cameraPosBefore.z * cameraPosBefore.z);

        cameraPos.x = (cos(glm::radians(-xoffset)) * cameraPosBefore.x + sin(glm::radians(-xoffset)) * cameraPosBefore.z);
        if (glm::asin(cameraPosBefore.y / cameraPosLen) + glm::radians(-yoffset) > glm::radians(89.0f)) {
            cameraPos.y = cameraPosLen * glm::sin(glm::radians(89.0f));
        }
        else if (glm::asin(cameraPosBefore.y / cameraPosLen) + glm::radians(-yoffset) < glm::radians((-89.0f))) {
            cameraPos.y = cameraPosLen * glm::sin(glm::radians(-89.0f));
        }
        else {
            cameraPos.y = cameraPosLen * glm::sin(glm::asin(cameraPosBefore.y / cameraPosLen) + glm::radians(-yoffset));
        }
        cameraPos.z = -sin(glm::radians(-xoffset)) * cameraPosBefore.x + cos(glm::radians(-xoffset)) * cameraPosBefore.z;

        cameraFront.x = -0.5 * cameraPos.x;
        cameraFront.y = -0.5 * cameraPos.y;
        cameraFront.z = -0.5 * cameraPos.z;
    }
    else {
        if (firstClick)
        {
            lastX = x;
            lastY = y;
            cameraPosBefore = cameraPos;
            firstClick = false;
        }

        GLfloat xoffset = x - lastX;
        GLfloat yoffset = lastY - y;

        GLfloat sensitivity = 0.5;    // Change this value to your liking
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        GLfloat cameraPosLen = glm::sqrt(cameraPosBefore.x * cameraPosBefore.x + cameraPosBefore.y * cameraPosBefore.y + cameraPosBefore.z * cameraPosBefore.z);

        cameraPos.x = (cos(glm::radians(-xoffset)) * cameraPosBefore.x + sin(glm::radians(-xoffset)) * cameraPosBefore.z);
        if (glm::asin(cameraPosBefore.y / cameraPosLen) + glm::radians(-yoffset) > glm::radians(89.0f)) {
            cameraPos.y = cameraPosLen * glm::sin(glm::radians(89.0f));
        }
        else if (glm::asin(cameraPosBefore.y / cameraPosLen) + glm::radians(-yoffset) < glm::radians((-89.0f))) {
            cameraPos.y = cameraPosLen * glm::sin(glm::radians(-89.0f));
        }
        else {
            cameraPos.y = cameraPosLen * glm::sin(glm::asin(cameraPosBefore.y / cameraPosLen) + glm::radians(-yoffset));
        }
        cameraPos.z = -sin(glm::radians(-xoffset)) * cameraPosBefore.x + cos(glm::radians(-xoffset)) * cameraPosBefore.z;

        cameraFront.x = -0.5 * cameraPos.x;
        cameraFront.y = -0.5 * cameraPos.y;
        cameraFront.z = -0.5 * cameraPos.z;
    }
}


void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Sets the scoll callback for the current window.
    if (fov >= 1.0f && fov <= 90.0f)
        fov -= yoffset;
    if (fov <= 1.0f)
        fov = 1.0f;
    if (fov >= 90.0f)
        fov = 90.0f;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Sets the Keyboard callback for the current window.
}


int main(int argc, char* argv[])
{
    GLFWwindow* window;

    /* Initialize the glfw */
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    /* glfw: configure; necessary for MAC */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "CSCI3260 project", NULL, NULL);
    if (!window) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    /*register callback functions*/
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);                                                                  //
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    initializedGL();

    while (!glfwWindowShouldClose(window)) {
        /* Render here */
        paintGL();

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}







