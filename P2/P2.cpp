/*******************************************************************************
 * PROJETO: 3D Space Invaders
 * FICHEIRO: P2.cpp
 * 
 *      ORGANIZAÇÃO DO CÓDIGO:
 * 
 * * 1. DEFINIÇÕES E INCLUDES: Configurações de ecrã, constantes matemáticas e
 *      importação de bibliotecas (OpenGL, GLFW, GLM, TinyOBJ).
 * 
 * * 2. ESTRUTURAS DE DADOS:
 *       - Mesh: Gestão de buffers de modelos 3D.
 *       - Ball: Lógica de projéteis (jogador e inimigos).
 *       - HUD/Crosshair: Elementos de interface 2D.
 * 
 * * 3. VARIÁVEIS GLOBAIS: Estado do jogo, pontuação, saúde do jogador e
 *      gestão de texturas/shaders.
 * 
 * * 4. INTERFACE E HUD:
 *       - Funções de criação e renderização da mira (crosshair) e barra de vida.
 *       - Cálculo de indicadores de direção de inimigos fora do ecrã.
 * 
 * * 5. LÓGICA DE JOGO E IA:
 *       - Sistema de colisão esférica.
 *       - Gestão de ondas (waves) e spawn de diferentes tipos de inimigos.
 *       - IA de movimento e comportamento de ataque dos inimigos.
 * 
 * * 6. AMBIENTE E GRÁFICOS:
 *       - Geração procedural e renderização de Skybox espacial.
 *       - Carregamento de modelos OBJ e transferência para memória da GPU.
 * 
 * * 7. CICLO PRINCIPAL (Main Loop):
 *       - Inicialização do contexto gráfico.
 *       - Loop de processamento de inputs, física e renderização de frames.
 *       - Limpeza de recursos e terminação.
 * 
 * Bruno Marinho - 51619
 *******************************************************************************/



#define SCR_WIDTH 1500
#define SCR_HEIGHT 800
#define PI 3.14159265359f
#define STB_IMAGE_IMPLEMENTATION

// Includes
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include "stb_image.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "shader.hpp"
#include "texture.hpp"
#include "controls.hpp"
#include <tiny_obj_loader.h>
#include <list>
#include <random>
#include <algorithm>

struct Mesh {
    std::vector<float> vertices;
    unsigned int VAO = 0, VBO = 0;
    size_t count = 0;
};

enum EnemyType {
    BASIC_ENEMY = 0,
    FAST_ENEMY = 1,
    TANK_ENEMY = 2,
    SHOOTER_ENEMY = 3,
    BOSS_ENEMY = 4
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
    EnemyType type;
    int health;
    float speedMultiplier;
    float damageMultiplier;
    float scale;
    glm::vec3 color;
};


struct Ball {
    glm::vec3 position;
    glm::vec3 direction;
    float speed;
    float lifetime;
    bool isEnemyBall;
    bool hasCollided;
};

struct HUDElement {
    std::vector<float> vertices;
    unsigned int VAO = 0, VBO = 0;
};

// Globals
GLFWwindow* window;
GLuint ProgramID = 0;
GLuint SkyboxProgramID = 0;
GLuint MatrixID = 0;
GLuint ViewMatrixID = 0;
GLuint ModelMatrixID = 0;
GLuint TextureSamplerID = 0;

GLuint TexturaNave = 0;
GLuint TexturaTiro = 0;
GLuint TexturaInimigo = 0;
GLuint TexturaSkybox = 0;

extern glm::vec3 position;
std::list<Ball> activeBalls;
std::vector<Enemy> enemies;

int playerMaxHealth = 1000;
int playerHealth = playerMaxHealth;
int maxEnemies = 10;
int kills = 0;
int score = 1000000;
int currentWave = 100;
int enemiesPerWave = 10;
int enemiesInCurrentWave = 0;
float waveCooldown = 5.0f;
float waveTimer = 0.0f;
bool waveInProgress = false;
bool gameOver = false;

float ballSpeed = 900.0f;
float ballLifetime = 9.0f;
float enemyShootInterval = 2.0f;
float spawnRadius = 300.0f;
float enemyBallSpeed = 50.0f;
float playerRadius = 0.10f;
float ballRadius = 0.050f;
float enemyRadius = 20.0f;
float enemySpeed = 20.0f;
float minDistanceToPlayer = 100.0f;
float maxDistanceToPlayer = 400.0f;
float targetChangeTime = 2.0f;

bool leftMousePressed = false;
Mesh skyboxMesh;

struct Crosshair {
    HUDElement horizontal;
    HUDElement vertical;
};

// Função para criar mira
void createCrosshair(Crosshair& crosshair) {
    // Linha horizontal
    float hWidth = 20.0f;
    float hHeight = 2.0f;
    float centerX = SCR_WIDTH / 2.0f - hWidth / 2.0f;
    float centerY = SCR_HEIGHT / 2.0f - hHeight / 2.0f;

    crosshair.horizontal.vertices = {
        centerX, centerY, 0.0f,
        centerX + hWidth, centerY, 0.0f,
        centerX, centerY + hHeight, 0.0f,
        centerX + hWidth, centerY, 0.0f,
        centerX + hWidth, centerY + hHeight, 0.0f,
        centerX, centerY + hHeight, 0.0f
    };

    // Linha vertical
    float vWidth = 2.0f;
    float vHeight = 20.0f;
    centerX = SCR_WIDTH / 2.0f - vWidth / 2.0f;
    centerY = SCR_HEIGHT / 2.0f - vHeight / 2.0f;

    crosshair.vertical.vertices = {
        centerX, centerY, 0.0f,
        centerX + vWidth, centerY, 0.0f,
        centerX, centerY + vHeight, 0.0f,
        centerX + vWidth, centerY, 0.0f,
        centerX + vWidth, centerY + vHeight, 0.0f,
        centerX, centerY + vHeight, 0.0f
    };

    // Criar VAOs/VBOs
    for (auto& element : { &crosshair.horizontal, &crosshair.vertical }) {
        glGenVertexArrays(1, &element->VAO);
        glGenBuffers(1, &element->VBO);

        glBindVertexArray(element->VAO);
        glBindBuffer(GL_ARRAY_BUFFER, element->VBO);
        glBufferData(GL_ARRAY_BUFFER, element->vertices.size() * sizeof(float), element->vertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }
}

// Função para renderizar mira
void renderCrosshair(Crosshair& crosshair, GLuint shaderProgram) {
    glUseProgram(shaderProgram);

    glm::mat4 projection = glm::ortho(0.0f, (float)SCR_WIDTH, 0.0f, (float)SCR_HEIGHT);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "MVP"), 1, GL_FALSE, glm::value_ptr(projection));

    // Cor vermelha para a mira
    glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 1.0f, 0.0f, 0.0f);

    // Renderizar linha horizontal
    glBindVertexArray(crosshair.horizontal.VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Renderizar linha vertical
    glBindVertexArray(crosshair.vertical.VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
}

// Funções utilitárias
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
    glBufferData(GL_ARRAY_BUFFER, element.vertices.size() * sizeof(float), element.vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void renderHealthBar(HUDElement& healthBar, float health, GLuint shaderProgram) {
    glUseProgram(shaderProgram);

    glm::mat4 projection = glm::ortho(0.0f, (float)SCR_WIDTH, 0.0f, (float)SCR_HEIGHT);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "MVP"), 1, GL_FALSE, glm::value_ptr(projection));

    // Desenhar fundo (vermelho)
    glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.5f, 0.0f, 0.0f);
    glBindVertexArray(healthBar.VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Desenhar barra verde proporcional
    float healthPercent = glm::clamp(health / (float)playerMaxHealth, 0.0f, 1.0f);

    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(healthPercent, 1.0f, 1.0f));
    glm::mat4 MVP = projection * scale;
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "MVP"), 1, GL_FALSE, glm::value_ptr(MVP));

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
    float angle = randomFloat(0, 2.0f * PI);
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

Enemy createEnemy(EnemyType type, const glm::vec3& playerPos) {
    
    Enemy enemy;

    float angle = randomFloat(0, 2.0f * PI);
    float distance = randomFloat(100.0f, 300.0f);

    enemy.position = playerPos + glm::vec3(
        distance * cos(angle),
        randomFloat(-10.0f * currentWave/2,10.0f * currentWave/2),
        distance * sin(angle)
    );

    enemy.type = type;
    enemy.active = true;
    enemy.targetTimer = randomFloat(0, targetChangeTime);
    enemy.targetPoint = generateTargetPoint(playerPos, enemy.position);
    enemy.shootTimer = randomFloat(0, enemyShootInterval);
    enemy.velocity = glm::vec3(0.0f);

    switch (type) {
        case BASIC_ENEMY:
            enemy.radius = 4.0f;
            enemy.health = 1;
            enemy.speedMultiplier = 1.0f;
            enemy.damageMultiplier = 1.0f;
            enemy.scale = 2.0f;
            enemy.color = glm::vec3(1.0f, 1.0f, 1.0f);
            break;

        case FAST_ENEMY:
            enemy.radius = 0.8f * 4.0f;
            enemy.health = 1;
            enemy.speedMultiplier = 2.0f;
            enemy.damageMultiplier = 0.5f;
            enemy.scale = 1.5f;
            enemy.color = glm::vec3(1.0f, 1.0f, 0.0f);
            break;

        case TANK_ENEMY:
            enemy.radius = 2.0f * 4.0f;
            enemy.health = 3;
            enemy.speedMultiplier = 0.5f;
            enemy.damageMultiplier = 2.0f;
            enemy.scale = 6.5f; 
            enemy.color = glm::vec3(0.6f, 0.0f, 0.0f);
            break;

        case SHOOTER_ENEMY:
            enemy.radius = 1.2f * 4.0f;
            enemy.health = 1;
            enemy.speedMultiplier = 0.8f;
            enemy.damageMultiplier = 1.5f;
            enemy.scale = 2.2f;
            enemy.color = glm::vec3(0.0f, 7.0f, 0.4f);
            enemy.shootTimer = enemyShootInterval * 0.5f;
            break;

        case BOSS_ENEMY:
            enemy.radius = 4.0f * 10.0f;
            enemy.health = 10;
            enemy.speedMultiplier = 0.3f;
            enemy.damageMultiplier = 3.0f;
            enemy.scale = 25.0f; 
            enemy.color = glm::vec3(0.010f, 0.01f, 0.01f);
            break;
        }

    return enemy;
}

void handleEnemyHit(Enemy& enemy) {
    enemy.health--;
    if (enemy.health <= 0) {
        enemy.active = false;
        kills++;
        score += 50;

        switch (enemy.type) {
        case BASIC_ENEMY: score += 10; break;
        case FAST_ENEMY: score += 25; break;
        case TANK_ENEMY: score += 75; break;
        case SHOOTER_ENEMY: score += 40; break;
        case BOSS_ENEMY: score += 500; break;
        }

        enemiesInCurrentWave--;

        if (enemiesInCurrentWave <= 0 && waveInProgress) {
            waveInProgress = false;
            waveTimer = waveCooldown;
            currentWave++;
            enemiesPerWave = 10 + (currentWave - 1) * 3;

            std::cout << "=== WAVE " << currentWave - 1 << " COMPLETED! ===" << std::endl;
            std::cout << "Score: " << score << std::endl;
            std::cout << "Next wave in " << waveCooldown << " seconds..." << std::endl;
        }
    }
}

void startNewWave() {
    enemies.clear();
    enemiesInCurrentWave = enemiesPerWave;
    waveInProgress = true;
    waveTimer = 0.0f;

    for (int i = 0; i < enemiesPerWave; i++) {
        EnemyType type = BASIC_ENEMY;
        float r = randomFloat(0.0f, 1.0f);

        if (currentWave % 5 == 0) {
            if (r < 0.3f) type = BOSS_ENEMY;
            else if (r < 0.7f) type = TANK_ENEMY;
            else type = SHOOTER_ENEMY;
        }
        // Lógica de progressão normal
        else {
            if (currentWave >= 10 && r < 0.10f) {
                type = BOSS_ENEMY;
            }
            else if (currentWave >= 7 && r < 0.20f) {
                type = SHOOTER_ENEMY;
            }
            else if (currentWave >= 5 && r < 0.25f) {
                type = TANK_ENEMY;
            }
            else if (currentWave >= 3 && r < 0.30f) {
                type = FAST_ENEMY;
            }

        }

        enemies.push_back(createEnemy(type, position));
    }

    std::cout << "=== WAVE " << currentWave << " STARTED! ===" << std::endl;
    std::cout << "Enemies: " << enemiesPerWave << std::endl;
}

// Corrigir função worldToScreen e arrow drawing (linhas 367-391)

bool worldToScreen(
    const glm::vec3& worldPos,
    const glm::mat4& view,
    const glm::mat4& proj,
    glm::vec2& screenPos
) {
    glm::vec4 clip = proj * view * glm::vec4(worldPos, 1.0f);

    if (clip.w <= 0.0f) return false; // atrás da câmara

    glm::vec3 ndc = glm::vec3(clip) / clip.w;

    screenPos.x = (ndc.x * 0.5f + 0.5f) * SCR_WIDTH;
    screenPos.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * SCR_HEIGHT; // Corrigido: Y invertido

    return true;
}

// Função corrigida para desenhar setas indicadoras
void drawArrow2D(glm::vec2 pos, glm::vec2 dir, float radius, GLuint shader) {
    float size = 12.0f;

    // Calcular posição final da seta (na borda do círculo)
    glm::vec2 finalPos = pos + dir * radius;

    glm::vec2 right(-dir.y, dir.x);

    glm::vec2 p1 = finalPos + dir * size;
    glm::vec2 p2 = finalPos - dir * size + right * size * 0.5f;
    glm::vec2 p3 = finalPos - dir * size - right * size * 0.5f;

    float vertices[] = {
        p1.x, p1.y, 0.0f,
        p2.x, p2.y, 0.0f,
        p3.x, p3.y, 0.0f
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    glEnableVertexAttribArray(0);

    glm::mat4 proj = glm::ortho(0.0f, (float)SCR_WIDTH, 0.0f, (float)SCR_HEIGHT);
    glUniformMatrix4fv(glGetUniformLocation(shader, "MVP"), 1, GL_FALSE, glm::value_ptr(proj));
    glUniform3f(glGetUniformLocation(shader, "objectColor"), 1.0f, 1.0f, 0.0f);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
}

// Adicionar no main() após a renderização da HUD (antes do glfwSwapBuffers):
void renderEnemyIndicators(glm::mat4 ViewMatrix, glm::mat4 ProjectionMatrix, GLuint shader) {
    glm::vec2 screenCenter(SCR_WIDTH / 2.0f, SCR_HEIGHT / 2.0f);

    glUseProgram(shader);
    glDisable(GL_DEPTH_TEST);

    for (const auto& enemy : enemies) {
        if (!enemy.active) continue;

        glm::vec2 screenPos;
        if (worldToScreen(enemy.position, ViewMatrix, ProjectionMatrix, screenPos)) {
            // Inimigo está na tela, não precisa de seta
        }
        else {
            // Inimigo fora da tela, calcular direção
            glm::vec3 enemyDir = glm::normalize(enemy.position - position);
            glm::vec4 enemyViewSpace = ViewMatrix * glm::vec4(enemyDir, 0.0f);

            glm::vec2 dir2D = glm::normalize(glm::vec2(enemyViewSpace.x, enemyViewSpace.y));

            // Desenhar seta na borda da tela
            drawArrow2D(screenCenter, dir2D, 150.0f, shader);
        }
    }

    glEnable(GL_DEPTH_TEST);
}



// Função simplificada para criar skybox
void createSimpleSkybox() {
    // Criar cubo para skybox
    float skyboxVertices[] = {
        // Back face
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,

        // Front face
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        // Left face
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,

        // Right face
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,

         // Bottom face
         -1.0f, -1.0f, -1.0f,
          1.0f, -1.0f, -1.0f,
          1.0f, -1.0f,  1.0f,
          1.0f, -1.0f,  1.0f,
         -1.0f, -1.0f,  1.0f,
         -1.0f, -1.0f, -1.0f,

         // Top face
         -1.0f,  1.0f, -1.0f,
          1.0f,  1.0f, -1.0f,
          1.0f,  1.0f,  1.0f,
          1.0f,  1.0f,  1.0f,
         -1.0f,  1.0f,  1.0f,
         -1.0f,  1.0f, -1.0f
    };

    glGenVertexArrays(1, &skyboxMesh.VAO);
    glGenBuffers(1, &skyboxMesh.VBO);

    glBindVertexArray(skyboxMesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxMesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    // Criar textura de skybox simples (cor sólida com algumas estrelas)
    const int texSize = 512;
    std::vector<unsigned char> skyboxData(texSize * texSize * 3);

    for (int y = 0; y < texSize; y++) {
        for (int x = 0; x < texSize; x++) {
            int idx = (y * texSize + x) * 3;

            // Cor de fundo - espaço escuro
            skyboxData[idx] = 10;     // R
            skyboxData[idx + 1] = 10; // G
            skyboxData[idx + 2] = 20; // B

            // Adicionar algumas "estrelas" aleatórias
            if (randomFloat(0, 1) > 0.995f) {
                skyboxData[idx] = 255;
                skyboxData[idx + 1] = 255;
                skyboxData[idx + 2] = 255;
            }
        }
    }

    glGenTextures(1, &TexturaSkybox);
    glBindTexture(GL_TEXTURE_CUBE_MAP, TexturaSkybox);

    for (unsigned int i = 0; i < 6; i++) {
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            0, GL_RGB,
            texSize, texSize,
            0, GL_RGB, GL_UNSIGNED_BYTE,
            skyboxData.data()
        );
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texSize, texSize, 0, GL_RGB, GL_UNSIGNED_BYTE, skyboxData.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void renderSkybox(glm::mat4 ViewMatrix, glm::mat4 ProjectionMatrix)
{
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    glUseProgram(SkyboxProgramID);

    glm::mat4 view = glm::mat4(glm::mat3(ViewMatrix));
    glm::mat4 MVP = ProjectionMatrix * view;

    glUniformMatrix4fv(
        glGetUniformLocation(SkyboxProgramID, "MVP"),
        1, GL_FALSE, glm::value_ptr(MVP)
    );

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, TexturaSkybox);
    glUniform1i(glGetUniformLocation(SkyboxProgramID, "skybox"), 0);

    glBindVertexArray(skyboxMesh.VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
}

// Load OBJ
bool LoadObjModel(const std::string& path, Mesh& mesh) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    auto lastSlash = path.find_last_of("/\\");
    std::string baseDir = (lastSlash == std::string::npos) ? std::string("") : path.substr(0, lastSlash + 1);

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), baseDir.c_str())) {
        std::cerr << "Failed to load .obj file: " << err << std::endl;
        return false;
    }

    mesh.vertices.clear();

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            mesh.vertices.push_back(attrib.vertices[3 * index.vertex_index + 0]);
            mesh.vertices.push_back(attrib.vertices[3 * index.vertex_index + 1]);
            mesh.vertices.push_back(attrib.vertices[3 * index.vertex_index + 2]);

            if (!attrib.normals.empty() && index.normal_index >= 0) {
                mesh.vertices.push_back(attrib.normals[3 * index.normal_index + 0]);
                mesh.vertices.push_back(attrib.normals[3 * index.normal_index + 1]);
                mesh.vertices.push_back(attrib.normals[3 * index.normal_index + 2]);
            }
            else {
                mesh.vertices.push_back(0.0f);
                mesh.vertices.push_back(0.0f);
                mesh.vertices.push_back(1.0f);
            }

            if (!attrib.texcoords.empty() && index.texcoord_index >= 0) {
                mesh.vertices.push_back(attrib.texcoords[2 * index.texcoord_index + 0]);
                mesh.vertices.push_back(1.0f - attrib.texcoords[2 * index.texcoord_index + 1]);
            }
            else {
                mesh.vertices.push_back(0.0f);
                mesh.vertices.push_back(0.0f);
            }
        }
    }

    mesh.count = mesh.vertices.size() / 8;

    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);

    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(float), mesh.vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    return true;
}

GLuint transferDataToGPUMemory(Mesh& Obj, const std::string& objPath, const std::string& textureDDSPath) {
    if (!LoadObjModel(objPath, Obj)) {
        std::cerr << "Error loading OBJ model: " << objPath << std::endl;
        exit(EXIT_FAILURE);
    }

    GLuint texID = 0;
    if (!textureDDSPath.empty()) {
        texID = loadDDS(textureDDSPath.c_str());
        if (texID == 0) {
            std::cerr << "Warning: failed to load DDS texture: " << textureDDSPath << std::endl;
        }
    }
    return texID;
}

void cleanupDataFromGPU(Mesh& Obj) {
    if (Obj.VAO) glDeleteVertexArrays(1, &Obj.VAO);
    if (Obj.VBO) glDeleteBuffers(1, &Obj.VBO);
    Obj.VAO = Obj.VBO = 0;
}

void initGLFWandGLEW() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        exit(EXIT_FAILURE);
    }
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "3D SpaceInvaders", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to open GLFW window" << std::endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);

    glewExperimental = true;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPos(window, SCR_WIDTH / 2, SCR_HEIGHT / 2);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
}

// Adicione estas funções ANTES do main():
GLuint createShaders() {
    std::string shaderPath = "P2/shaders/";

    // Tenta carregar shaders de arquivo
    GLuint program = LoadShaders(
        "C:\\Users\\bruno\\source\\repos\\KJBruninho\\SpaceInvaders_3D\\P2\\shaders\\TransformVertexShader.vertexshader",
        "C:\\Users\\bruno\\source\\repos\\KJBruninho\\SpaceInvaders_3D\\P2\\shaders\\TextureFragmentShader.fragmentshader"
    );

    return program;
}

GLuint createSkyboxShaders() {
    std::string shaderPath = "P2/shaders/";

    // Tenta carregar shaders de arquivo
    GLuint program = LoadShaders(
        "C:\\Users\\bruno\\source\\repos\\KJBruninho\\SpaceInvaders_3D\\P2\\shaders\\skybox.vertexshader",
        "C:\\Users\\bruno\\source\\repos\\KJBruninho\\SpaceInvaders_3D\\P2\\shaders\\skybox.fragmentshader"
    );

    return program;
}

int main(void) {
    initGLFWandGLEW();

    // Usar shaders embutidos
    ProgramID = createShaders();

    if (ProgramID == 0) {
        std::cerr << "Failed to create main shader program!" << std::endl;
        glfwTerminate();
        return -1;
    }

    glDepthFunc(GL_LEQUAL);
    SkyboxProgramID = createSkyboxShaders();
    glDepthFunc(GL_LESS);


    if (SkyboxProgramID == 0) {
        std::cerr << "Warning: Failed to create skybox shader" << std::endl;
    }

    MatrixID = glGetUniformLocation(ProgramID, "MVP");
    ViewMatrixID = glGetUniformLocation(ProgramID, "V");
    ModelMatrixID = glGetUniformLocation(ProgramID, "M");
    TextureSamplerID = glGetUniformLocation(ProgramID, "textureSampler");

    Mesh Obj1, Obj2, Obj3;
    TexturaNave = transferDataToGPUMemory(Obj1,
        "C:\\Users\\bruno\\source\\repos\\KJBruninho\\SpaceInvaders_3D\\P2\\DDS\\Millennium_Falcon.obj",
        "C:\\Users\\bruno\\source\\repos\\KJBruninho\\SpaceInvaders_3D\\P2\\DDS\\nave.DDS");

    TexturaInimigo = transferDataToGPUMemory(Obj2,
        "C:\\Users\\bruno\\source\\repos\\KJBruninho\\SpaceInvaders_3D\\P2\\DDS\\ball.obj",
        "C:\\Users\\bruno\\source\\repos\\KJBruninho\\SpaceInvaders_3D\\P2\\DDS\\inimigo.DDS");

    TexturaTiro = transferDataToGPUMemory(Obj3,
        "C:\\Users\\bruno\\source\\repos\\KJBruninho\\SpaceInvaders_3D\\P2\\DDS\\fireball.obj",
        "C:\\Users\\bruno\\source\\repos\\KJBruninho\\SpaceInvaders_3D\\P2\\DDS\\metal2.DDS");

    glUseProgram(ProgramID);
    glUniform1i(TextureSamplerID, 0);

    float scaleFactor = 0.1f; // Aumentado para a nave ser visível
    float lastFrameTime = glfwGetTime();

    HUDElement healthBar;
    createHUDRectangle(healthBar, 20.0f, SCR_HEIGHT - 40.0f, 200.0f, 20.0f);

    Crosshair crosshair;
    createCrosshair(crosshair);

    createSimpleSkybox();
    startNewWave();

    while (!gameOver && glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && !glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float currentTime = glfwGetTime();
        float deltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;

        computeMatricesFromInputs();
        glm::mat4 ProjectionMatrix = getProjectionMatrix();
        glm::mat4 ViewMatrix = getViewMatrix();

        // Render skybox primeiro
        renderSkybox(ViewMatrix, ProjectionMatrix);

        glUseProgram(ProgramID);

        // Configurar uniforms comuns - POSIÇÃO DA LUZ IMPORTANTE!
        glm::vec3 lightPos = position + glm::vec3(0.0f, 50.0f, 0.0f); // Luz acima do jogador
        glm::vec3 lightColor(1.0f, 1.0f, 1.0f);

        glUniform3fv(glGetUniformLocation(ProgramID, "LightPosition_worldspace"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(ProgramID, "lightColor"), 1, glm::value_ptr(lightColor));
        glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, glm::value_ptr(ViewMatrix));

        // Atualizar lógica da wave
        if (!waveInProgress) {
            waveTimer -= deltaTime;
            if (waveTimer <= 0.0f) {
                startNewWave();
            }
        }

        // Colisões
        for (auto& ball : activeBalls) {
            if (ball.isEnemyBall && !ball.hasCollided) {
                if (checkSphereCollision(ball.position, ballRadius, position, playerRadius)) {
                    playerHealth -= 10;
                    ball.hasCollided = true;
                    std::cout << "Player hit! Health: " << playerHealth << std::endl;
                    if (playerHealth <= 0) {
                        std::cout << "Game Over!" << std::endl;
                        gameOver = true;
                        break;
                    }
                }
            }
            else if (!ball.isEnemyBall && !ball.hasCollided) {
                for (auto& enemy : enemies) {
                    if (enemy.active && checkSphereCollision(ball.position, ballRadius, enemy.position, enemy.radius)) {
                        handleEnemyHit(enemy);
                        ball.hasCollided = true;
                        break;
                    }
                }
            }
        }

        activeBalls.remove_if([](const Ball& ball) { return ball.hasCollided || ball.lifetime <= 0.0f; });

        // Mouse input
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !leftMousePressed) {
            leftMousePressed = true;

            glm::vec3 cameraDirection = -glm::normalize(glm::vec3(ViewMatrix[0][2], ViewMatrix[1][2], ViewMatrix[2][2]));
            glm::vec3 spawnPosition = position + (cameraDirection * 5.0f); // Mais perto da câmera

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

        // ========== RENDERIZAR INIMIGOS ==========
        glUseProgram(ProgramID);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, TexturaInimigo);
        glBindVertexArray(Obj2.VAO);

        for (auto& enemy : enemies) {
            if (!enemy.active) continue;

            // Atualizar lógica do inimigo
            enemy.shootTimer -= deltaTime;
            if (enemy.shootTimer <= 0.0f) {
                Ball enemyBall;
                enemyBall.position = enemy.position;
                enemyBall.direction = glm::normalize(position - enemy.position);
                enemyBall.speed = enemyBallSpeed * enemy.damageMultiplier;
                enemyBall.lifetime = ballLifetime;
                enemyBall.isEnemyBall = true;
                enemyBall.hasCollided = false;

                activeBalls.push_back(enemyBall);
                enemy.shootTimer = enemyShootInterval;
            }

            enemy.targetTimer -= deltaTime;
            if (enemy.targetTimer <= 0.0f) {
                enemy.targetPoint = generateTargetPoint(position, enemy.position);
                enemy.targetTimer = targetChangeTime;
            }

            glm::vec3 directionToTarget = enemy.targetPoint - enemy.position;
            float distanceToTarget = glm::length(directionToTarget);
            if (distanceToTarget > 0.1f) {
                glm::vec3 desiredVelocity = glm::normalize(directionToTarget) * enemySpeed * enemy.speedMultiplier;
                enemy.velocity = glm::mix(enemy.velocity, desiredVelocity, deltaTime * 2.0f);
            }

            float distanceToPlayer = glm::length(position - enemy.position);
            if (distanceToPlayer < minDistanceToPlayer) {
                glm::vec3 awayFromPlayer = glm::normalize(enemy.position - position);
                enemy.velocity += awayFromPlayer * enemySpeed * deltaTime * 2.0f;
            }
            else if (distanceToPlayer > maxDistanceToPlayer) {
                enemy.targetPoint = generateTargetPoint(position, enemy.position);
                enemy.targetTimer = targetChangeTime;
            }

            enemy.position += enemy.velocity * deltaTime;

            // Configurar transformação do inimigo
            glm::mat4 EnemyModel = glm::mat4(1.0f);
            EnemyModel = glm::translate(EnemyModel, enemy.position);

            glm::vec3 lookDirection = glm::normalize(position - enemy.position);
            float rotationAngle = atan2(lookDirection.x, lookDirection.z);
            EnemyModel = glm::rotate(EnemyModel, rotationAngle + glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            EnemyModel = glm::scale(EnemyModel, glm::vec3(enemy.scale));

            // Calcular MVP
            glm::mat4 EnemyMVP = ProjectionMatrix * ViewMatrix * EnemyModel;

            // Configurar uniforms
            glUniformMatrix4fv(MatrixID, 1, GL_FALSE, glm::value_ptr(EnemyMVP));
            glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, glm::value_ptr(EnemyModel));
            glUniform3fv(glGetUniformLocation(ProgramID, "objectColor"), 1, glm::value_ptr(enemy.color));
            glUniform1i(glGetUniformLocation(ProgramID, "objectID"), 2); // ID para inimigos

            // Desenhar
            glDrawArrays(GL_TRIANGLES, 0, Obj2.count);
        }
        glBindVertexArray(0);

        // ========== RENDERIZAR NAVE ==========
        glUseProgram(ProgramID);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, TexturaNave);
        glBindVertexArray(Obj1.VAO);

        glm::vec3 cameraDirection = -glm::normalize(glm::vec3(ViewMatrix[0][2], ViewMatrix[1][2], ViewMatrix[2][2]));
        glm::vec3 cameraUp = glm::vec3(ViewMatrix[0][1], ViewMatrix[1][1], ViewMatrix[2][1]);
        glm::vec3 cameraRight = glm::normalize(glm::cross(cameraDirection, cameraUp));

        // Posição da nave: um pouco à frente e abaixo da câmera
        glm::vec3 falconPosition = position + (cameraDirection * 1.0f) - (cameraUp * 0.5f);

        glm::mat4 falconRotation(1.0f);
        falconRotation[0] = glm::vec4(cameraRight, 0.0f);
        falconRotation[1] = glm::vec4(cameraUp, 0.0f);
        falconRotation[2] = glm::vec4(-cameraDirection, 0.0f);

        glm::mat4 FalconModel = glm::mat4(1.0f);
        FalconModel = glm::translate(FalconModel, falconPosition);
        FalconModel = FalconModel * falconRotation;
        FalconModel = glm::rotate(FalconModel, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotacionar 180°
        FalconModel = glm::scale(FalconModel, glm::vec3(scaleFactor, scaleFactor, scaleFactor));

        glm::mat4 MVP3 = ProjectionMatrix * ViewMatrix * FalconModel;

        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, glm::value_ptr(MVP3));
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, glm::value_ptr(FalconModel));
        glUniform3fv(glGetUniformLocation(ProgramID, "objectColor"), 1, glm::value_ptr(glm::vec3(0.8f, 0.9f, 1.0f)));
        glUniform1i(glGetUniformLocation(ProgramID, "objectID"), 1); // ID para nave

        glDrawArrays(GL_TRIANGLES, 0, Obj1.count);
        glBindVertexArray(0);

        // ========== RENDERIZAR PROJÉTEIS ==========
        glUseProgram(ProgramID);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, TexturaTiro);
        glBindVertexArray(Obj3.VAO);

        for (auto it = activeBalls.begin(); it != activeBalls.end();) {
            it->lifetime -= deltaTime;
            it->position += it->direction * it->speed * deltaTime;

            if (it->lifetime <= 0.0f || it->hasCollided) {
                it = activeBalls.erase(it);
            }
            else {
                glm::mat4 BallModel = glm::mat4(1.0f);
                BallModel = glm::translate(BallModel, it->position);
                BallModel = glm::scale(BallModel, glm::vec3(0.10f)); // Aumentado

                glm::mat4 BallMVP = ProjectionMatrix * ViewMatrix * BallModel;

                glUniformMatrix4fv(MatrixID, 1, GL_FALSE, glm::value_ptr(BallMVP));
                glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, glm::value_ptr(BallModel));

                if (it->isEnemyBall) {
                    glUniform3fv(glGetUniformLocation(ProgramID, "objectColor"), 1, glm::value_ptr(glm::vec3(1.0f, 0.0f, 0.0f)));
                }
                else {
                    glUniform3fv(glGetUniformLocation(ProgramID, "objectColor"), 1, glm::value_ptr(glm::vec3(0.0f, 0.5f, 1.0f)));
                }
                glUniform1i(glGetUniformLocation(ProgramID, "objectID"), 3); // ID para projéteis

                glDrawArrays(GL_TRIANGLES, 0, Obj3.count);
                ++it;
            }
        }
        glBindVertexArray(0);

        // ========== HUD ==========
        glDisable(GL_DEPTH_TEST);
        renderHealthBar(healthBar, (float)playerHealth, ProgramID);
        renderCrosshair(crosshair, ProgramID);
        renderEnemyIndicators(ViewMatrix, ProjectionMatrix, ProgramID);
        glEnable(GL_DEPTH_TEST);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    // Limpeza
    std::cout << "\nCleaning up..." << std::endl;
    cleanupDataFromGPU(Obj1);
    cleanupDataFromGPU(Obj2);
    cleanupDataFromGPU(Obj3);

    glDeleteProgram(ProgramID);
    glfwTerminate();

    std::cout << "Exiting..." << std::endl;
    return 0;
}