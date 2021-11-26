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
glm::vec3 cameraPos   = glm::vec3(0.0f, 10.0f,  15.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, -0.3f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);

glm::vec3 craftFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 craftRight = glm::vec3(1.0f, 0.0f, 0.0f);
GLfloat delta_angle = 0.0f;
GLfloat delta_angle_before;

glm::vec3 cameraPosTrans = glm::vec3(0.0f, 0.0f, 200.0f);

GLfloat lastX;
GLfloat fov = 60.0f;

//shader
Shader shader;
Shader skyboxShader;

bool keys[1024];
//Delta time
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

//rock random
GLint randomY[300];
GLint randomZ[300];
GLint randomAngle[300];
GLint randomAngle2[300];
GLint randomSize[300];

bool firstMouse = true;

GLint craftTextureIndex[3] = {0, 0, 0};
GLint collect_count = 0;
GLint collected[3] = {0, 0, 0};


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
            //printf("%s\n", lineHeader);
            // Face elements
            int count = 0;
            
            std::string line;
            std::getline(file, line);
            //std::cout << line <<std::endl;
            
            if (line.length() != 0){
                file.seekg(-line.length(), std::ios::cur);
            }
            
            
            char* s = (char *)line.c_str();
            
            char *p = std::strtok(s, " ");
            while(p) {
                count += 1;
                //std::cout << count <<std::endl;
                p = strtok(NULL, " ");
            }
            

            
            V vertices[10];
            for (int i = 0; i < count ; i++) {
                char ch;
                file >> vertices[i].index_position >> ch >> vertices[i].index_uv >> ch >> vertices[i].index_normal;
                
                
            }
            

            // Check if there are more than three vertices in one face.
            /*
            std::string redundency;
            std::getline(file, redundency);
            if (redundency.length() >= 5) {
                std::cerr << "There may exist some errors while load the obj file. Error content: [" << redundency << " ]" << std::endl;
                std::cerr << "Please note that we only support the faces drawing with triangles. There are more than three vertices in one face." << std::endl;
                std::cerr << "Your obj file can't be read properly by our simple parser :-( Try exporting with other options." << std::endl;
                exit(1);
            }*/
            
             
            if(count == 3)
            {
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
            }
            else if(count == 4)
            {
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
                }
                for (int i = 0; i < 4; i++) {
                    if (temp_vertices.find(vertices[i]) == temp_vertices.end() && i !=1) {
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
                    else if(i !=1)
                    {
                        // reuse the existing vertex
                        unsigned int index = temp_vertices[vertices[i]];
                        model.indices.push_back(index);
                    }
                }
            }
            else if(count == 5)
            {
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
                for (int i = 0; i < 4; i++) {
                    if (temp_vertices.find(vertices[i]) == temp_vertices.end() && i !=1) {
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
                    else if(i !=1)
                    {
                        // reuse the existing vertex
                        unsigned int index = temp_vertices[vertices[i]];
                        model.indices.push_back(index);
                    }
                } // for
                for (int i = 0; i < 5; i++) {
                    if (temp_vertices.find(vertices[i]) == temp_vertices.end() && i !=1 && i !=2) {
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
                    else if(i !=1 && i !=2)
                    {
                        // reuse the existing vertex
                        unsigned int index = temp_vertices[vertices[i]];
                        model.indices.push_back(index);
                    }
                } // for
            }
            
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
Texture spacecraftTexture[2];

GLuint craftVAO, craftVBO, craftEBO;
Model craftObj;
Texture craftTexture[2];

GLuint planetVAO, planetVBO, planetEBO;
Model planetObj;
Texture planetTexture[2];

GLuint rockVAO, rockVBO, rockEBO;
Model rockObj;
Texture rockTexture;

Texture goldTexture;

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
    
    faces.push_back("./CourseProjectMaterials/texture/skybox/right.bmp");
    faces.push_back("./CourseProjectMaterials/texture/skybox/left.bmp");
    faces.push_back("./CourseProjectMaterials/texture/skybox/top.bmp");
    faces.push_back("./CourseProjectMaterials/texture/skybox/bottom.bmp");
    faces.push_back("./CourseProjectMaterials/texture/skybox/back.bmp");
    faces.push_back("./CourseProjectMaterials/texture/skybox/front.bmp");
    cubemapTexture = loadCubemap(faces);
    
    
    //spacecraft
    spacecraftObj = loadOBJ("./CourseProjectMaterials/object/spacecraft.obj");
    spacecraftTexture[0].setupTexture("./CourseProjectMaterials/texture/spacecraftTexture.bmp");
    spacecraftTexture[1].setupTexture("./CourseProjectMaterials/texture/gold_big.bmp");
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
    
    //craft
    /*
    meshModel craftObj("./CourseProjectMaterials/object/craft.obj");
     */

    craftObj = loadOBJ("./CourseProjectMaterials/object/craft.obj");
     
    craftTexture[0].setupTexture("./CourseProjectMaterials/texture/ringTexture.bmp");
    craftTexture[1].setupTexture("./CourseProjectMaterials/texture/red_big.bmp");
    //vertex array object
    glGenVertexArrays(1, &craftVAO);
    glBindVertexArray(craftVAO);
    
    //vertex buffer object
    glGenBuffers(1, &craftVBO);
    glBindBuffer(GL_ARRAY_BUFFER, craftVBO);
    glBufferData(GL_ARRAY_BUFFER, craftObj.vertices.size() * sizeof(Vertex), &craftObj.vertices[0], GL_STATIC_DRAW);
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
    glGenBuffers(1, &craftEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, craftEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, craftObj.indices.size() * sizeof(unsigned int), &craftObj.indices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    
    
    //planet
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
    
    //rock obj
    rockObj = loadOBJ("./CourseProjectMaterials/object/rock.obj");
    
    rockTexture.setupTexture("./CourseProjectMaterials/texture/rockTexture.bmp");
    //vertex array object
    glGenVertexArrays(1, &rockVAO);
    glBindVertexArray(rockVAO);
    
    //vertex buffer object
    glGenBuffers(1, &rockVBO);
    glBindBuffer(GL_ARRAY_BUFFER, rockVBO);
    glBufferData(GL_ARRAY_BUFFER, rockObj.vertices.size() * sizeof(Vertex), &rockObj.vertices[0], GL_STATIC_DRAW);
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
    glGenBuffers(1, &rockEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rockEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, rockObj.indices.size() * sizeof(unsigned int), &rockObj.indices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    
    srand((unsigned)(time(NULL)));
    for(int p = 0; p < 300; p++){
        GLint a = rand() % 300 - 150;
        randomY[p] = a;
    }
    for(int p = 0; p < 300; p++){
        GLint a = rand() % 400 - 200;
        randomZ[p] = a;
    }
    for(int p = 0; p < 300; p++){
        GLint a = rand() % 25 + 10;
        randomSize[p] = a;
    }
    for(int p = 0; p < 300; p++){
        GLint a = rand() % 3600;
        randomAngle[p] = a;
    }
    for(int p = 0; p < 300; p++){
        GLint a = rand() % 3600;
        randomAngle2[p] = a;
    }
    
    goldTexture.setupTexture("./CourseProjectMaterials/texture/gold_big.bmp");
    
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
    glm::vec3 point_lightPos = glm::vec3(0.0f, 100.0f, 100.0f);
    lshader.setVec3("point_lightPos", point_lightPos);
    
    point_ambientLight = glm::vec3(0.5f, 0.5f, 0.5f);
    lshader.setVec3("point_ambientLight_planet", point_ambientLight);
    point_lightPos = glm::vec3(0.0f, 0.0f, 0.0f);
    lshader.setVec3("point_lightPos_planet", point_lightPos);
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
    
    cameraPos   = glm::vec3(15.0f * glm::sin(glm::radians(delta_angle)), 10.0f,  15.0f * glm::cos(glm::radians(delta_angle)));
    cameraFront = glm::vec3(-1.0f * glm::sin(glm::radians(delta_angle)), -0.3f, -1.0f * glm::cos(glm::radians(delta_angle)));
    
    craftFront = glm::vec3(-1.0f * glm::sin(glm::radians(delta_angle)), 0.0f, -1.0f * glm::cos(glm::radians(delta_angle)));
    craftRight = glm::vec3(1.0f * glm::cos(glm::radians(delta_angle)), 0.0f, -1.0f * glm::sin(glm::radians(delta_angle)));
        
    view = glm::lookAt(cameraPos + cameraPosTrans, cameraPos + cameraPosTrans + cameraFront, cameraUp);
    projection = glm::perspective(glm::radians(fov), (GLfloat)SCR_WIDTH / (GLfloat)SCR_HEIGHT,0.1f, 2000.0f);
    
    glDepthMask(GL_FALSE);// Remember to turn depth writing off
    skyboxShader.use();

    model = glm::scale(model, glm::vec3(500.0f, 500.0f, 500.0f));
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
    
    model = glm::translate(model, cameraPosTrans);
    model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));
    model = glm::rotate(model, glm::radians(delta_angle), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::translate(model, glm::vec3(0.0f, -200.0f, 0.0f));
    shader.setMat4("model", model);
    if(collected[0] == 0 || collected[1] == 0 || collected[2] == 0 )
        spacecraftTexture[0].bind(0);
    else
        spacecraftTexture[1].bind(0);
    glBindVertexArray(spacecraftVAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, spacecraftEBO);
    glDrawElements(GL_TRIANGLES, spacecraftObj.indices.size(), GL_UNSIGNED_INT, 0);
    
    
    GLfloat x_pos = (((GLint)(glfwGetTime() * 0.5f) / 1 % 2) == 0) ? ((GLfloat)(glfwGetTime() * 0.5f) - (GLint)(glfwGetTime() * 0.5f) / 1) : (1- ((GLfloat)(glfwGetTime() * 0.5f) - (GLint)(glfwGetTime() * 0.5f) / 1));
    
    //craft 1
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-100.0f * x_pos + 50.0f, -2.0f, 50.0f));
    
    glm::vec3 distance_vec = cameraPosTrans - glm::vec3(-100.0f * x_pos + 50.0f, -2.0f, 50.0f);
    GLfloat distance1 = sqrt(distance_vec.x * distance_vec.x + distance_vec.y * distance_vec.y + distance_vec.z * distance_vec.z);
    
    if (distance1 < 10)
        craftTextureIndex[0] = 1;
    
    model = glm::rotate(model, (GLfloat)glfwGetTime() * glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    shader.setMat4("model", model);
    if(craftTextureIndex[0] == 0)
        craftTexture[0].bind(0);
    else
        craftTexture[((GLint)(glfwGetTime() * 4.0f) / 1 % 2)].bind(0);
    glBindVertexArray(craftVAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, craftEBO);
    glDrawElements(GL_TRIANGLES, craftObj.indices.size(), GL_UNSIGNED_INT, 0);
    //craft 2
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(100.0f * x_pos - 50.0f, -2.0f, 100.0f));
    
    distance_vec = cameraPosTrans - glm::vec3(100.0f * x_pos - 50.0f, -2.0f, 100.0f);
    GLfloat distance2 = sqrt(distance_vec.x * distance_vec.x + distance_vec.y * distance_vec.y + distance_vec.z * distance_vec.z);
    
    if (distance2 < 10)
        craftTextureIndex[1] = 1;
    
    model = glm::rotate(model, (GLfloat)glfwGetTime() * glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    shader.setMat4("model", model);
    if(craftTextureIndex[1] == 0)
        craftTexture[0].bind(0);
    else
        craftTexture[((GLint)(glfwGetTime() * 4.0f) / 1 % 2)].bind(0);
    glBindVertexArray(craftVAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, craftEBO);
    glDrawElements(GL_TRIANGLES, craftObj.indices.size(), GL_UNSIGNED_INT, 0);
    //craft 3
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-100.0f * x_pos + 50.0f, -2.0f, 150.0f));
    
    distance_vec = cameraPosTrans - glm::vec3(-100.0f * x_pos + 50.0f, -2.0f, 150.0f);
    GLfloat distance3 = sqrt(distance_vec.x * distance_vec.x + distance_vec.y * distance_vec.y + distance_vec.z * distance_vec.z);
    
    if (distance3 < 10)
        craftTextureIndex[2] = 1;
    
    model = glm::rotate(model, (GLfloat)glfwGetTime() * glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    shader.setMat4("model", model);
    if(craftTextureIndex[2] == 0)
        craftTexture[0].bind(0);
    else
        craftTexture[((GLint)(glfwGetTime() * 4.0f) / 1 % 2)].bind(0);
    glBindVertexArray(craftVAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, craftEBO);
    glDrawElements(GL_TRIANGLES, craftObj.indices.size(), GL_UNSIGNED_INT, 0);
    
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, -2.0f, 0.0f));
    model = glm::scale(model, glm::vec3(4.0f, 4.0f, 4.0f));
    model = glm::rotate(model, (GLfloat)glfwGetTime() * (glm::radians(15.0f)), glm::vec3(0.0f, 1.0f, 0.0f));
    shader.setMat4("model", model);
    shader.setInt("normalMapping_flag", 1);
    planetTexture[0].bind(0);
    planetTexture[1].bind(1);
    glBindVertexArray(planetVAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planetEBO);
    glDrawElements(GL_TRIANGLES, planetObj.indices.size(), GL_UNSIGNED_INT, 0);
    shader.setInt("normalMapping_flag", 0);
    
    for (int i = 0; i < 300; i++){
        model = glm::mat4(1.0f);
        model = glm::rotate(model, (GLfloat)glfwGetTime() * (glm::radians(3.0f)) + glm::radians(randomAngle[i] * 0.1f), glm::vec3(0.0f, 1.0f, 0.0f));
        //std::cout << randomAngle[i] * 0.1f << std::endl;
        model = glm::translate(model, glm::vec3(0.0f, 0.0f + randomY[i] * 0.01f, 20.0f + randomZ[i]*0.01f));
        model = glm::rotate(model, (GLfloat)glfwGetTime() * (glm::radians(30.0f)) + glm::radians(randomAngle2[i] * 0.1f), glm::vec3(1.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.01f * randomSize[i], 0.01f * randomSize[i], 0.01f * randomSize[i]));
        shader.setMat4("model", model);
        rockTexture.bind(0);
        glBindVertexArray(rockVAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rockEBO);
        glDrawElements(GL_TRIANGLES, rockObj.indices.size(), GL_UNSIGNED_INT, 0);
    }
    for (int i = 0; i < 3; i++){
        model = glm::mat4(1.0f);
        
        GLfloat rotate_angle = (GLfloat)glfwGetTime() * (glm::radians(3.0f)) + glm::radians(i * 120.0f);

        glm::vec3 trans = glm::vec3(20.0f * glm::sin(rotate_angle), 0.0f, 20.0f * glm::cos(rotate_angle));
        
        model = glm::translate(model, trans);
        model = glm::rotate(model, (GLfloat)glfwGetTime() * (glm::radians(30.0f)) + glm::radians(randomAngle2[i] * 0.1f), glm::vec3(1.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.4f , 0.4f , 0.4f ));
        shader.setMat4("model", model);
        
        distance_vec = cameraPosTrans - trans;
        GLfloat distance = sqrt(distance_vec.x * distance_vec.x + distance_vec.y * distance_vec.y + distance_vec.z * distance_vec.z);
        
        if (distance < 5)
            collected[i] = 1;
        
        if(collected[i] == 0)
        {
            goldTexture.bind(0);
            glBindVertexArray(rockVAO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rockEBO);
            glDrawElements(GL_TRIANGLES, rockObj.indices.size(), GL_UNSIGNED_INT, 0);
        }

    }
    
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    // Sets the mouse-button callback for the current window.

}

void cursor_position_callback(GLFWwindow* window, double x, double y)
{
    
    if(firstMouse)
    {
        lastX = x;
        delta_angle_before = delta_angle;
        firstMouse = false;
    }
    GLfloat xoffset = x - lastX;
    
    delta_angle = delta_angle_before - 0.2f * xoffset;
    
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
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
    
    
    if (key >= 0 && key < 1024)
    {
        if (action == GLFW_PRESS){
            keys[key] = true;
        }
        else if (action == GLFW_RELEASE) {
            keys[key] = false;
        }
    }
}

void do_movement()
{
    //penguin controls
    GLfloat Speed = deltaTime;
    if (keys[GLFW_KEY_UP])
        cameraPosTrans += craftFront * (40 * Speed);
    if (keys[GLFW_KEY_DOWN])
        cameraPosTrans -= craftFront * (40 * Speed);
    if (keys[GLFW_KEY_LEFT])
        cameraPosTrans -= craftRight * (40 * Speed);
    if (keys[GLFW_KEY_RIGHT])
        cameraPosTrans += craftRight * (40 * Speed);


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
        
        GLfloat currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        do_movement();
    }

    glfwTerminate();

    return 0;
}







