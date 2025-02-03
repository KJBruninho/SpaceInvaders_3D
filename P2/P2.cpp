#define SCR_WIDTH 1500
#define SCR_HEIGHT 1500
#define PI 3.14159265359
#define STB_IMAGE_IMPLEMENTATION

// Includes
#include <iostream>
#include <vector>
#include "stb_image.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <common/shader.hpp>
#include <common/texture.hpp>
#include <common/controls.hpp>
#include <tiny_obj_loader.h>
#include <list>
#include <random>

struct Mesh {
    std::vector<float> vertices;
    unsigned int VAO, VBO;
};

struct Ball {
    glm::vec3 position;
    glm::vec3 direction;
    float speed;
    float lifetime;
    bool isEnemyBall;
    bool hasCollided;
};

struct Enemy {
    glm::vec3 position;
    glm::vec3 velocity;
    float shootTimer;  
    bool active;
    float radius;
    float angle;
    float targetTimer;       
    glm::vec3 targetPoint;
};

struct HUDElement {
    std::vector<float> vertices;
    unsigned int VAO, VBO;
};

// Global Variables
GLFWwindow* window;
GLuint ProgramID;
GLuint MatrixID;
GLuint TexturaNave;
GLuint TexturaTiro; 
GLuint TexturaInimigo;
extern glm::vec3 position;
std::list<Ball> activeBalls;
std::vector<Enemy> enemies;
int playerHealth = 100; 
int Health = playerHealth;          
int maxEnemies = 10;              
int kills = 0;
float ballSpeed = 300.0f;
float ballLifetime = 9.0f;
float enemyShootInterval = 2.0f;  
float spawnRadius = 200.0f;      
float enemyBallSpeed = 50.0f;   
float playerRadius = 2.0f;      
float ballRadius = 0.50f;         
float enemyRadius = 3.0f;       
float enemySpeed = 20.0f;             
float minDistanceToPlayer = 100.0f;   
float maxDistanceToPlayer = 300.0f;   
float targetChangeTime = 2.0f;        
bool leftMousePressed = false;
bool gameOver = false;          

void createHUDRectangle(HUDElement& element, float x, float y, float width, float height) {
   
    element.vertices = {
        x, y, 0.0f,          
        x + width, y, 0.0f,   
        x, y + height, 0.0f, 
        x + width, y, 0.0f,  
        x + width, y + height, 0.0f, 
        x, y + height, 0.0f   
    };

    glGenVertexArrays(1, &element.VAO);
    glGenBuffers(1, &element.VBO);

    glBindVertexArray(element.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, element.VBO);
    glBufferData(GL_ARRAY_BUFFER, element.vertices.size() * sizeof(float), &element.vertices[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void renderHealthBar(HUDElement& healthBar, float health, GLuint shaderProgram) {
    glUseProgram(shaderProgram);

    // Matriz de projeção ortográfica para o HUD
    glm::mat4 projection = glm::ortho(0.0f, (float)SCR_WIDTH, 0.0f, (float)SCR_HEIGHT);
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &projection[0][0]);

    // Cor vermelha para a barra de fundo
    glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.5f, 0.0f, 0.0f);
    glBindVertexArray(healthBar.VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Ajustar a largura da barra de vida baseado na saúde atual
    float healthPercent = health / Health;  // Considerando vida máxima de 1000
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(healthPercent, 1.0f, 1.0f));
    glm::mat4 MVP = projection * scale;
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

    // Cor verde para a vida atual
    glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.0f, 1.0f, 0.0f);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
}

float randomFloat(float min, float max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(min, max);
    return dis(gen);
}

glm::vec3 generateTargetPoint(const glm::vec3& playerPos, const glm::vec3& enemyPos) {
    float angle = randomFloat(0, 2 * PI);
    float distance = randomFloat(minDistanceToPlayer, maxDistanceToPlayer);

    glm::vec3 targetPoint = playerPos + glm::vec3(
        distance * cos(angle),
        randomFloat(-20.0f, 20.0f),  
        distance * sin(angle)
    );

    return targetPoint;
}

bool checkSphereCollision(const glm::vec3& pos1, float radius1, const glm::vec3& pos2, float radius2) {
    float distance = glm::length(pos1 - pos2);
    return distance < (radius1 + radius2);
}

bool LoadObjModel(const std::string& path, Mesh& mesh) {
    
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    auto lastSlash = path.find_last_of("/\\");
    std::string baseDir = (lastSlash == std::string::npos)
       ? std::string("")
       : path.substr(0, lastSlash + 1);

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), baseDir.c_str())) {
        std::cerr << "Failed to load .obj file: " << err << std::endl;
        return false;
    }

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            float vx = attrib.vertices[3 * index.vertex_index + 0];
            float vy = attrib.vertices[3 * index.vertex_index + 1];
            float vz = attrib.vertices[3 * index.vertex_index + 2];
            mesh.vertices.push_back(vx);
            mesh.vertices.push_back(vy);
            mesh.vertices.push_back(vz);

            if (!attrib.normals.empty()) {
                float nx = attrib.normals[3 * index.normal_index + 0];
                float ny = attrib.normals[3 * index.normal_index + 1];
                float nz = attrib.normals[3 * index.normal_index + 2];
                mesh.vertices.push_back(nx);
                mesh.vertices.push_back(ny);
                mesh.vertices.push_back(nz);
            }
            else {
                mesh.vertices.push_back(0.0f);
                mesh.vertices.push_back(0.0f);
                mesh.vertices.push_back(1.0f);
            }

            if (!attrib.texcoords.empty() && index.texcoord_index >= 0) {
                float u = attrib.texcoords[2 * index.texcoord_index + 0];
                float v = attrib.texcoords[2 * index.texcoord_index + 1];
                mesh.vertices.push_back(u);
                mesh.vertices.push_back(1.0f - v);
            }
            else {
                mesh.vertices.push_back(0.0f);
                mesh.vertices.push_back(0.0f);
            }
        }
    }

    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);

    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(float), &mesh.vertices[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    return true;
}


GLuint transferDataToGPUMemory(Mesh& Obj, const std::string& obj, const char * objmtl) {
    ProgramID = LoadShaders(
        //Alterar o path para obter corresponder
        "C:\\TransformVertexShader.vertexshader",
        "C:\\TextureFragmentShader.fragmentshader");

    if (ProgramID == 0) {
        std::cerr << "Error loading shaders!" << std::endl;
        exit(EXIT_FAILURE);
    }

    MatrixID = glGetUniformLocation(ProgramID, "MVP");

    if (!LoadObjModel(obj, Obj)) {
        std::cerr << "Error loading OBJ model!" << std::endl;
        exit(EXIT_FAILURE);
    }

    return loadDDS(objmtl);
}

void cleanupDataFromGPU(Mesh& Obj) {
    glDeleteVertexArrays(1, &Obj.VAO);
    glDeleteBuffers(1, &Obj.VBO);
    glDeleteProgram(ProgramID);
}

int main(void) {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "3D SpaceInvaders", NULL, NULL);

    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glewExperimental = true;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        glfwTerminate();
        return -1;
    }

    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPos(window, SCR_WIDTH / 2, SCR_HEIGHT / 2);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);

    
    Mesh Obj1, Obj2, Obj3;
    HUDElement healthBar;

    TexturaNave = transferDataToGPUMemory(Obj1, "Millennium_Falcon.obj", "nave.DDS");
    TexturaInimigo = transferDataToGPUMemory(Obj2, "Ball_fighter.obj", "inimigo.DDS");
    TexturaTiro = transferDataToGPUMemory(Obj3, "fireball.obj", "metal2.DDS");

    float scaleFactor = 5.0f;
    float lastFrameTime = glfwGetTime();

    createHUDRectangle(healthBar, 20.0f, SCR_HEIGHT - 40.0f, 200.0f, 20.0f);

    // Inicializar inimigos com novas propriedades
    for (int i = 0; i < maxEnemies; i++) {
        Enemy enemy;
        float angle = (2 * PI * i) / maxEnemies;
        enemy.position = glm::vec3(
            spawnRadius * cos(angle),
            randomFloat(-spawnRadius,spawnRadius),
            spawnRadius * sin(angle)
        );
        enemy.velocity = glm::vec3(0.0f);
        enemy.shootTimer = randomFloat(0, enemyShootInterval);
        enemy.targetTimer = randomFloat(0, targetChangeTime);
        enemy.targetPoint = generateTargetPoint(position, enemy.position);
        enemy.active = true;
        enemy.radius = enemyRadius;
        enemies.push_back(enemy);
    }

    while (!gameOver && glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && !glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(ProgramID);

        float currentTime = glfwGetTime();
        float deltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;

        computeMatricesFromInputs();
        glm::mat4 ProjectionMatrix = getProjectionMatrix();
        glm::mat4 ViewMatrix = getViewMatrix();

        // Variáveis de iluminação
        glm::vec3 lightPos(5.0f, 5.0f, 5.0f);
        glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
        glm::vec3 objectColor(0.70f, 1.0f, 1.0f);

        glUniform3fv(glGetUniformLocation(ProgramID, "lightPos"), 1, &lightPos[0]);
        glUniform3fv(glGetUniformLocation(ProgramID, "lightColor"), 1, &lightColor[0]);
        glUniform3fv(glGetUniformLocation(ProgramID, "objectColor"), 1, &objectColor[0]);

        // Verificar colisões entre tiros inimigos e jogador
        for (auto& ball : activeBalls) {
            if (ball.isEnemyBall && !ball.hasCollided) {
                if (checkSphereCollision(ball.position, ballRadius, position, playerRadius)) {
                    playerHealth--;
                    ball.hasCollided = true;

                    std::cout << "Player hit! Health: " << playerHealth << std::endl;

                    if (playerHealth <= 0) {
                        std::cout << "Game Over!" << std::endl;
                        gameOver = true;
                        break;
                    }
                }
            }
            // Verificar colisões entre tiros do jogador e inimigos
            else if (!ball.isEnemyBall && !ball.hasCollided) {
                for (auto& enemy : enemies) {
                    if (enemy.active && checkSphereCollision(ball.position, ballRadius, enemy.position, enemyRadius)) {
                        enemy.active = false;
                        ball.hasCollided = true;
                        kills++;

                        // Reativar inimigo em nova posição após um tempo
                        enemy.angle = randomFloat(0, 2 * PI);
                        enemy.position = glm::vec3(
                            position.x + spawnRadius * cos(enemy.angle),
                            position.y,
                            position.z + spawnRadius * sin(enemy.angle)

                        );
                        enemy.active = true;
                        break;
                    }
                }
            }
        }

        // Remover bolas que colidiram
        activeBalls.remove_if([](const Ball& ball) { return ball.hasCollided; });

        // Check for mouse click and create new ball
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !leftMousePressed) {
            leftMousePressed = true;

            glm::vec3 cameraDirection = -glm::normalize(glm::vec3(ViewMatrix[0][2], ViewMatrix[1][2], ViewMatrix[2][2]));
            glm::vec3 spawnPosition = position + (cameraDirection * 20.0f); 
            spawnPosition.y -= 2.0f;
            spawnPosition.x -= 0.5f;

            Ball newBall;
            newBall.position = spawnPosition;
            newBall.direction = cameraDirection;
            newBall.speed = ballSpeed;
            newBall.lifetime = ballLifetime;
            newBall.isEnemyBall = false;
            newBall.hasCollided = false;

            activeBalls.push_back(newBall);
        }
        else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) {
            leftMousePressed = false;
        }

        // Atualizar e renderizar inimigos
        for (auto& enemy : enemies) {
            if (!enemy.active) continue;

            // Atualizar timer de tiro
            enemy.shootTimer -= deltaTime;
            if (enemy.shootTimer <= 0) {
                Ball enemyBall;
                enemyBall.position = enemy.position;
                enemyBall.direction = glm::normalize(position - enemy.position);
                enemyBall.speed = enemyBallSpeed;
                enemyBall.lifetime = ballLifetime;
                enemyBall.isEnemyBall = true;
                enemyBall.hasCollided = false;

                activeBalls.push_back(enemyBall);
                enemy.shootTimer = enemyShootInterval;
            }

            // Atualizar movimento do inimigo
            enemy.targetTimer -= deltaTime;
            if (enemy.targetTimer <= 0) {
                enemy.targetPoint = generateTargetPoint(position, enemy.position);
                enemy.targetTimer = targetChangeTime;
            }

            // Calcular direção para o ponto alvo
            glm::vec3 directionToTarget = enemy.targetPoint - enemy.position;
            float distanceToTarget = glm::length(directionToTarget);

            // Ajustar velocidade com base na direção ao alvo
            if (distanceToTarget > 0.1f) {
                glm::vec3 desiredVelocity = glm::normalize(directionToTarget) * enemySpeed;
                enemy.velocity = glm::mix(enemy.velocity, desiredVelocity, deltaTime * 2.0f);
            }

            // Verificar distância do jogador
            float distanceToPlayer = glm::length(position - enemy.position);
            if (distanceToPlayer < minDistanceToPlayer) {
                // Se muito perto, adicionar força de repulsão
                glm::vec3 awayFromPlayer = glm::normalize(enemy.position - position);
                enemy.velocity += awayFromPlayer * enemySpeed * deltaTime * 2.0f;
            }
            else if (distanceToPlayer > maxDistanceToPlayer) {
                // Se muito longe, gerar novo ponto alvo mais próximo do jogador
                enemy.targetPoint = generateTargetPoint(position, enemy.position);
                enemy.targetTimer = targetChangeTime;
            }

            // Atualizar posição
            enemy.position += enemy.velocity * deltaTime;

            // Renderizar inimigo
            glm::mat4 EnemyModel = glm::mat4(1.0f);
            EnemyModel = glm::translate(EnemyModel, enemy.position);

            // Fazer o inimigo olhar na direção do movimento
            glm::vec3 lookDirection = glm::normalize(position - enemy.position);
            float rotationAngle = atan2(lookDirection.x, lookDirection.z);
            EnemyModel = glm::rotate(EnemyModel, rotationAngle+90, glm::vec3(0.0f, 1.0f, 0.0f));
            EnemyModel = glm::scale(EnemyModel, glm::vec3(2.0f));

            glm::mat4 EnemyMVP = ProjectionMatrix * ViewMatrix * EnemyModel;
            glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &EnemyMVP[0][0]);

            glUniform1i(glGetUniformLocation(ProgramID, "objectID"), 2);
            glBindTexture(GL_TEXTURE_2D, TexturaInimigo);
            glBindVertexArray(Obj2.VAO);
            glDrawArrays(GL_TRIANGLES, 0, Obj2.vertices.size() / 8);
            glBindVertexArray(0);
        }

        // Posicionar e rotacionar a Falcon
        glm::vec3 cameraDirection = -glm::normalize(glm::vec3(ViewMatrix[0][2], ViewMatrix[1][2], ViewMatrix[2][2]));
        glm::vec3 cameraUp = glm::vec3(ViewMatrix[0][1], ViewMatrix[1][1], ViewMatrix[2][1]);
        glm::vec3 cameraRight = glm::normalize(glm::cross(cameraDirection, cameraUp));
        glm::vec3 falconPosition = position;

        glm::mat4 falconRotation = glm::mat4(1.0f);
        falconRotation[0] = glm::vec4(cameraRight, 0.0f);
        falconRotation[1] = glm::vec4(cameraUp, 0.0f);
        falconRotation[2] = glm::vec4(-cameraDirection, 0.0f);

        glm::mat4 FalconModel = glm::mat4(1.0f);
        FalconModel = glm::translate(FalconModel, falconPosition);
        FalconModel = FalconModel * falconRotation;
        FalconModel = glm::scale(FalconModel, glm::vec3(scaleFactor) + glm::vec3(0.0f, -7.0f, 0.0f));
        glm::mat4 MVP3 = ProjectionMatrix * ViewMatrix * FalconModel;

        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP3[0][0]);
        glUniform1i(glGetUniformLocation(ProgramID, "objectID"), 1); 
        glBindTexture(GL_TEXTURE_2D, TexturaNave);
        glBindVertexArray(Obj1.VAO);
        glDrawArrays(GL_TRIANGLES, 0, Obj1.vertices.size() / 8);
        glBindVertexArray(0);

        // Atualizar e renderizar bolas
        for (auto it = activeBalls.begin(); it != activeBalls.end();) {
            it->lifetime -= deltaTime;
            it->position += it->direction * it->speed * deltaTime;

            if (it->lifetime <= 0 || it->hasCollided) {
                it = activeBalls.erase(it);
            }
            else {
                glm::mat4 BallModel = glm::mat4(1.0f);
                BallModel = glm::translate(BallModel, it->position);
                BallModel = glm::scale(BallModel, glm::vec3(0.5f));

                glm::mat4 BallMVP = ProjectionMatrix * ViewMatrix * BallModel;
                glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &BallMVP[0][0]);

                if (it->isEnemyBall) {
                    glUniform3fv(glGetUniformLocation(ProgramID, "objectColor"), 1,
                        &glm::vec3(1.0f, 0.0f, 0.0f)[0]);  // Vermelho para tiros inimigos
                }
                else {
                    glUniform3fv(glGetUniformLocation(ProgramID, "objectColor"), 1,
                        &glm::vec3(0.0f, 0.0f, 1.0f)[0]);  // Azul para tiros do jogador
                }

                glUniform1i(glGetUniformLocation(ProgramID, "objectID"), 3);
                glBindTexture(GL_TEXTURE_2D, TexturaTiro);
                glBindVertexArray(Obj3.VAO);
                glDrawArrays(GL_TRIANGLES, 0, Obj3.vertices.size() / 8);
                glBindVertexArray(0);

                ++it;
            }
        }

        glDisable(GL_DEPTH_TEST);   
        renderHealthBar(healthBar, (float)playerHealth, ProgramID);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    if (gameOver) {
        std::cout << "Kills: " << kills << std::endl;
        std::cout << "Pressione ESC para sair..." << std::endl;
        while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS) {
            glfwPollEvents();
        }
    }

    cleanupDataFromGPU(Obj1);
    cleanupDataFromGPU(Obj2);
    cleanupDataFromGPU(Obj3);

    glfwTerminate();
    return 0;
}
