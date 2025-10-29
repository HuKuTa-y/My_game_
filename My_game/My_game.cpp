#include "raylib.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>

#define MAX_BULLETS 100
#define NUM_BRICK_BLOCKS 10
#define NUM_STONE_BLOCKS 1
#define NUM_BUSHES 3
#define NUM_ENEMIES 3
#define TRANSITION_DURATION 2.0f
#define ENEMY_SPEED 100.0f
#define ENEMY_VIEW_RADIUS 300.0f
#define ENEMY_FIRE_RADIUS 250.0f
#define ENEMY_PATROL_RADIUS 150.0f

int playerHits = 0;
int score = 0;

typedef struct {
    Vector2 position;
    Vector2 size;
    bool active;
} BrickBlock;

typedef struct {
    Vector2 position;
    Vector2 size;
    bool active;
} StoneBlock;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    bool active;
    bool fromPlayer; // добавлено
} Bullet;

typedef enum {
    BLOCK_TYPE_NORMAL,
    BLOCK_TYPE_BRICK,
    BLOCK_TYPE_BUSH
} BlockType;

typedef struct {
    Vector2 position;
    Vector2 size;
    bool active;
    float currentTransparency; // текущая прозрачность 0..1
    float targetTransparency;  // целевая прозрачность 0..1
    float transitionProgress;  // прогресс перехода 0..1
    bool inTransition;         // идет ли процесс плавного изменения
    BlockType type;
} BushBlock;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    bool active;
    float shootTimer;
    Vector2 patrolTarget;
    bool hasTarget;
} Enemy;

Vector2 impulse = { 0, 0 };

// Расширенная структура врага для вращения
typedef struct {
    Vector2 position;
    Vector2 velocity;
    bool active;
    float shootTimer;
    Vector2 patrolTarget;
    bool hasTarget;
    float rotationAngle;      // угол вращения
    float rotationSpeed;      // скорость вращения
    int rotationDirection;    // направление вращения (1 или -1)
} EnemyExt;

void SpawnBullet(Bullet bullets[], Vector2 startPos, float angleDeg, float bulletSpeed, bool fromPlayer) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            bullets[i].active = true;
            bullets[i].position = startPos;
            float angleRad = DEG2RAD * angleDeg;
            bullets[i].velocity.x = cosf(angleRad) * bulletSpeed;
            bullets[i].velocity.y = sinf(angleRad) * bulletSpeed;
            bullets[i].fromPlayer = fromPlayer;
            break;
        }
    }
}

Vector2 GetRandomPatrolTarget(Vector2 currentPos, float radius) {
    float angle = (float)(rand() % 360) * DEG2RAD;
    return Vector2{ currentPos.x + cosf(angle) * radius, currentPos.y + sinf(angle) * radius };
}

int main(void) {
    srand((unsigned int)time(NULL));
    const int screenWidth = 800;
    const int screenHeight = 600;
    const int mapWidth = 2000;
    const int mapHeight = 2000;

    InitWindow(screenWidth, screenHeight, "AI Enemies");
    DisableCursor();
    SetTargetFPS(60);

    Rectangle player = {
        (float)screenWidth / 2 - 35,
        (float)screenHeight / 2 - 25,
        70, 40
    };

    Vector2 velocity = { 0.0f, 0.0f };
    const float maxSpeed = 400.0f;
    const float acceleration = 1500.0f;
    const float friction = 0.89f;

    Bullet bullets[MAX_BULLETS] = { 0 };
    for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = false;

    float rotation = 0.0f; // угол фигуры в градусах
    const float rotationSpeed = 90.0f;

    BrickBlock brickBlocks[NUM_BRICK_BLOCKS] = {
        { Vector2 { 300, 100 }, Vector2 { 50, 50 }, true },
        { Vector2 { 1500, 1700 }, Vector2 { 80, 80 }, true },
        { Vector2 { 900, 600 }, Vector2 { 60, 60 }, true },
        { Vector2 { 1200, 700 }, Vector2 { 70, 40 }, true },
        { Vector2 { 600, 1550 }, Vector2 { 50, 100 }, true },
        { Vector2 { 100, 1150 }, Vector2 { 50, 100 }, true },
        { Vector2 { 200, 1900 }, Vector2 { 50, 100 }, true },
        { Vector2 { 1900, 450 }, Vector2 { 50, 100 }, true },
        { Vector2 { 1350, 700 }, Vector2 { 50, 100 }, true },
        { Vector2 { 1700, 1150 }, Vector2 { 50, 100 }, true },
    };

    StoneBlock stoneBlocks[NUM_STONE_BLOCKS] = {
        { Vector2 { 1300, 900 }, Vector2 { 60, 60 }, true }
    };

    BushBlock bushes[NUM_BUSHES] = {
        { Vector2 { 150, 150 }, Vector2 { 80, 80 }, true, 1.0f, 1.0f, 0.0f, false, BLOCK_TYPE_BUSH },
        { Vector2 { 400, 300 }, Vector2 { 100, 50 }, true, 1.0f, 1.0f, 0.0f, false, BLOCK_TYPE_BUSH },
        { Vector2 { 600, 900 }, Vector2 { 100, 100 }, true, 1.0f, 1.0f, 0.0f, false, BLOCK_TYPE_BUSH }
    };

    Camera2D camera = { 0 };

    EnemyExt enemies[NUM_ENEMIES];
    for (int i = 0; i < NUM_ENEMIES; i++) {
        enemies[i].active = true;
        enemies[i].position.x = rand() % (mapWidth - 50);
        enemies[i].position.y = rand() % (mapHeight - 50);
        enemies[i].hasTarget = false;
        enemies[i].shootTimer = 2.0f;
        enemies[i].patrolTarget = GetRandomPatrolTarget(enemies[i].position, ENEMY_PATROL_RADIUS);
        float randAngleDeg = (float)(rand() % 360);
        float randRad = DEG2RAD * randAngleDeg;
        enemies[i].velocity.x = cosf(randRad) * ENEMY_SPEED;
        enemies[i].velocity.y = sinf(randRad) * ENEMY_SPEED;
        enemies[i].rotationAngle = (float)(rand() % 360);
        enemies[i].rotationSpeed = 60 + rand() % 60; // 60-120 deg/sec
        enemies[i].rotationDirection = (rand() % 2) * 2 - 1; // 1 или -1
    }

    bool shootKeyPressed = false; // флаг для контроля однократной стрельбы

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();
        camera.target = Vector2{ player.x + player.width / 2, player.y + player.height / 2 };
        camera.offset = Vector2{ (float)screenWidth / 2, (float)screenHeight / 2 };
        camera.zoom = 1.0f;
        float radians = DEG2RAD * rotation;

        // Обработка вращения и движения
        bool rotateLeft = IsKeyDown(KEY_A);
        bool rotateRight = IsKeyDown(KEY_D);
        bool moveForward = IsKeyDown(KEY_W);
        bool moveBackward = IsKeyDown(KEY_S);

        if (rotateLeft && moveBackward) {
            rotation += rotationSpeed * deltaTime;
            float dirX = cosf(radians);
            float dirY = sinf(radians);
            velocity.x -= dirX * acceleration * deltaTime;
            velocity.y -= dirY * acceleration * deltaTime;
        }
        else if (rotateRight && moveBackward) {
            rotation -= rotationSpeed * deltaTime;
            float dirX = cosf(radians);
            float dirY = sinf(radians);
            velocity.x -= dirX * acceleration * deltaTime;
            velocity.y -= dirY * acceleration * deltaTime;
        }
        else {
            if (rotateLeft) {
                rotation -= rotationSpeed * deltaTime;
            }
            if (rotateRight) {
                rotation += rotationSpeed * deltaTime;
            }
            if (moveForward) {
                float dirX = cosf(radians);
                float dirY = sinf(radians);
                velocity.x += dirX * acceleration * deltaTime;
                velocity.y += dirY * acceleration * deltaTime;
            }
            if (moveBackward) {
                float dirX = cosf(radians);
                float dirY = sinf(radians);
                velocity.x -= dirX * acceleration * deltaTime;
                velocity.y -= dirY * acceleration * deltaTime;
            }
        }

        float speed = sqrtf(velocity.x * velocity.x + velocity.y * velocity.y);
        if (speed > maxSpeed) {
            velocity.x = (velocity.x / speed) * maxSpeed;
            velocity.y = (velocity.y / speed) * maxSpeed;
        }

        velocity.x *= friction;
        velocity.y *= friction;
        if (fabsf(velocity.x) < 0.5f) velocity.x = 0;
        if (fabsf(velocity.y) < 0.5f) velocity.y = 0;

        player.x += velocity.x * deltaTime;
        player.y += velocity.y * deltaTime;

        // границы
        if (player.x < 0) {
            player.x = 0;
            velocity.x = -velocity.x;
        }
        else if (player.x + player.width > mapWidth) {
            player.x = mapWidth - player.width;
            velocity.x = -velocity.x;
        }

        if (player.y < 0) {
            player.y = 0;
            velocity.y = -velocity.y;
        }
        else if (player.y + player.height > mapHeight) {
            player.y = mapHeight - player.height;
            velocity.y = -velocity.y;
        }

        // Стрельба игроком
        if (IsKeyPressed(KEY_ENTER)) {
            Vector2 startPos = { player.x, player.y };
            SpawnBullet(bullets, startPos, rotation, 600.0f, true);
        }

        // --- Обновление врагов --- //
        for (int i = 0; i < NUM_ENEMIES; i++) {
            if (enemies[i].active) {
                // Вращение врага
                enemies[i].rotationAngle += enemies[i].rotationDirection * enemies[i].rotationSpeed * deltaTime;
                if (enemies[i].rotationAngle > 360) enemies[i].rotationAngle -= 360;
                if (enemies[i].rotationAngle < 0) enemies[i].rotationAngle += 360;

                // Иногда менять направление вращения
                if (rand() % 1000 < 5) { // примерно 0.5% шанс за кадр
                    enemies[i].rotationDirection *= -1;
                }

                // Перемещение врага
                Vector2 newPos = {
                    enemies[i].position.x + enemies[i].velocity.x * deltaTime,
                    enemies[i].position.y + enemies[i].velocity.y * deltaTime
                };

                if (newPos.x < 0 || newPos.x > mapWidth - 50) {
                    enemies[i].velocity.x = -enemies[i].velocity.x;
                } else {
                    enemies[i].position.x = newPos.x;
                }
                if (newPos.y < 0 || newPos.y > mapHeight - 50) {
                    enemies[i].velocity.y = -enemies[i].velocity.y;
                } else {
                    enemies[i].position.y = newPos.y;
                }

                // Стрельба по игроку
                Vector2 playerPos = { player.x + player.width / 2, player.y + player.height / 2 };
                Vector2 toPlayer = { playerPos.x - enemies[i].position.x, playerPos.y - enemies[i].position.y };
                float distToPlayer = sqrtf(toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y);

                enemies[i].shootTimer -= deltaTime;

                if (distToPlayer <= ENEMY_VIEW_RADIUS) {
                    enemies[i].hasTarget = true;
                    enemies[i].patrolTarget = playerPos;

                    if (enemies[i].shootTimer <= 0) {
                        Vector2 startPos = { enemies[i].position.x + 25, enemies[i].position.y + 25 };
                        float angle = atan2f(toPlayer.y, toPlayer.x) * RAD2DEG;
                        SpawnBullet(bullets, startPos, angle, 300.0f, false);
                        enemies[i].shootTimer = 2.0f;
                    }
                } else {
                    enemies[i].hasTarget = false;
                    if (sqrtf((enemies[i].patrolTarget.x - enemies[i].position.x) * (enemies[i].patrolTarget.x - enemies[i].position.x) +
                        (enemies[i].patrolTarget.y - enemies[i].position.y) * (enemies[i].patrolTarget.y - enemies[i].position.y)) < 10) {
                        enemies[i].patrolTarget = GetRandomPatrolTarget(enemies[i].position, ENEMY_PATROL_RADIUS);
                        Vector2 dir = { enemies[i].patrolTarget.x - enemies[i].position.x, enemies[i].patrolTarget.y - enemies[i].position.y };
                        float length = sqrtf(dir.x * dir.x + dir.y * dir.y);
                        if (length != 0) {
                            dir.x /= length;
                            dir.y /= length;
                        }
                        enemies[i].velocity.x = dir.x * ENEMY_SPEED;
                        enemies[i].velocity.y = dir.y * ENEMY_SPEED;
                    }
                }
            }
        }

        // --- Обработка пуль --- //
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                bullets[i].position.x += bullets[i].velocity.x * deltaTime;
                bullets[i].position.y += bullets[i].velocity.y * deltaTime;
                if (bullets[i].position.x < 0 || bullets[i].position.x > mapWidth ||
                    bullets[i].position.y < 0 || bullets[i].position.y > mapHeight) {
                    bullets[i].active = false;
                }
                // столкновения с кирпичами и камнями
                for (int b = 0; b < NUM_BRICK_BLOCKS; b++) {
                    if (brickBlocks[b].active) {
                        Rectangle rect = { brickBlocks[b].position.x, brickBlocks[b].position.y, brickBlocks[b].size.x, brickBlocks[b].size.y };
                        if (CheckCollisionPointRec(bullets[i].position, rect)) {
                            bullets[i].active = false;
                            brickBlocks[b].active = false;
                            Vector2 blockCenter = { brickBlocks[b].position.x + brickBlocks[b].size.x / 2,
                                                    brickBlocks[b].position.y + brickBlocks[b].size.y / 2 };
                            Vector2 playerCenter = { player.x + player.width / 2,
                                                     player.y + player.height / 2 };
                            Vector2 diff = { playerCenter.x - blockCenter.x, playerCenter.y - blockCenter.y };
                            float length = sqrtf(diff.x * diff.x + diff.y * diff.y);
                            if (length != 0) {
                                float minDistance = 50.0f;
                                float maxDistance = 200.0f;
                                float impulseStrength;
                                if (length < minDistance) impulseStrength = 1000.0f;
                                else if (length > maxDistance) impulseStrength = 100.0f;
                                else {
                                    float t = (length - minDistance) / (maxDistance - minDistance);
                                    impulseStrength = 1000.0f * (1 - t) + 100.0f * t;
                                }
                                velocity.x += (diff.x / length) * impulseStrength;
                                velocity.y += (diff.y / length) * impulseStrength;
                            }
                            break;
                        }
                    }
                }
                for (int s = 0; s < NUM_STONE_BLOCKS; s++) {
                    if (stoneBlocks[s].active) {
                        Rectangle rect = { stoneBlocks[s].position.x, stoneBlocks[s].position.y, stoneBlocks[s].size.x, stoneBlocks[s].size.y };
                        if (CheckCollisionPointRec(bullets[i].position, rect)) {
                            bullets[i].active = false;
                            break;
                        }
                    }
                }

                // Попадание по врагу (только пули от игрока)
                if (bullets[i].fromPlayer) {
                    for (int j = 0; j < NUM_ENEMIES; j++) {
                        if (enemies[j].active) {
                            Rectangle enemyRect = { enemies[j].position.x, enemies[j].position.y, 50, 50 };
                            if (CheckCollisionPointRec(bullets[i].position, enemyRect)) {
                                enemies[j].active = false;
                                bullets[i].active = false;
                                break;
                            }
                        }
                    }
                }
            }
        }

        // Отталкивание с блоками
        for (int b = 0; b < NUM_BRICK_BLOCKS; b++) {
            if (brickBlocks[b].active) {
                Rectangle rect = { brickBlocks[b].position.x, brickBlocks[b].position.y, brickBlocks[b].size.x, brickBlocks[b].size.y };
                if (CheckCollisionRecs(player, rect)) {
                    Vector2 blockCenter = { brickBlocks[b].position.x + brickBlocks[b].size.x / 2, brickBlocks[b].position.y + brickBlocks[b].size.y / 2 };
                    Vector2 playerCenter = { player.x + player.width / 2, player.y + player.height / 2 };
                    Vector2 diff = { playerCenter.x - blockCenter.x, playerCenter.y - blockCenter.y };
                    float repelStrength = 100.0f;
                    float length = sqrtf(diff.x * diff.x + diff.y * diff.y);
                    if (length != 0) {
                        velocity.x += (diff.x / length) * repelStrength;
                        velocity.y += (diff.y / length) * repelStrength;
                    }
                }
            }
        }

        // Добавляем импульс
        velocity.x += impulse.x;
        velocity.y += impulse.y;
        // Плавное затухание импульса
        float impulseDamping = 0.1f;
        impulse.x += (0 - impulse.x) * impulseDamping;
        impulse.y += (0 - impulse.y) * impulseDamping;

        // Обновление позиции игрока
        player.x += velocity.x * deltaTime;
        player.y += velocity.y * deltaTime;

        // границы
        if (player.x < 0) {
            player.x = 0;
            velocity.x = -velocity.x;
        }
        else if (player.x + player.width > mapWidth) {
            player.x = mapWidth - player.width;
            velocity.x = -velocity.x;
        }

        if (player.y < 0) {
            player.y = 0;
            velocity.y = -velocity.y;
        }
        else if (player.y + player.height > mapHeight) {
            player.y = mapHeight - player.height;
            velocity.y = -velocity.y;
        }

        // --- ОПРОС КЛАВИШ --- //
        // Обнуляем флаг перед проверкой
        // (Вы уже проверяете IsKeyPressed(KEY_ENTER) для стрельбы)

        // --- Обновление врагов --- //
        for (int i = 0; i < NUM_ENEMIES; i++) {
            if (enemies[i].active) {
                // Вращение врага
                enemies[i].rotationAngle += enemies[i].rotationDirection * enemies[i].rotationSpeed * deltaTime;
                if (enemies[i].rotationAngle > 360) enemies[i].rotationAngle -= 360;
                if (enemies[i].rotationAngle < 0) enemies[i].rotationAngle += 360;

                // Иногда менять направление вращения
                if (rand() % 1000 < 5) { // примерно 0.5% шанс за кадр
                    enemies[i].rotationDirection *= -1;
                }

                // Перемещение врага
                Vector2 newPos = {
                    enemies[i].position.x + enemies[i].velocity.x * deltaTime,
                    enemies[i].position.y + enemies[i].velocity.y * deltaTime
                };

                if (newPos.x < 0 || newPos.x > mapWidth - 50) {
                    enemies[i].velocity.x = -enemies[i].velocity.x;
                } else {
                    enemies[i].position.x = newPos.x;
                }
                if (newPos.y < 0 || newPos.y > mapHeight - 50) {
                    enemies[i].velocity.y = -enemies[i].velocity.y;
                } else {
                    enemies[i].position.y = newPos.y;
                }

                // Стрельба по игроку
                Vector2 playerPos = { player.x + player.width / 2, player.y + player.height / 2 };
                Vector2 toPlayer = { playerPos.x - enemies[i].position.x, playerPos.y - enemies[i].position.y };
                float distToPlayer = sqrtf(toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y);

                enemies[i].shootTimer -= deltaTime;

                if (distToPlayer <= ENEMY_VIEW_RADIUS) {
                    enemies[i].hasTarget = true;
                    enemies[i].patrolTarget = playerPos;

                    if (enemies[i].shootTimer <= 0) {
                        Vector2 startPos = { enemies[i].position.x + 25, enemies[i].position.y + 25 };
                        float angle = atan2f(toPlayer.y, toPlayer.x) * RAD2DEG;
                        SpawnBullet(bullets, startPos, angle, 300.0f, false);
                        enemies[i].shootTimer = 2.0f;
                    }
                } else {
                    enemies[i].hasTarget = false;
                    if (sqrtf((enemies[i].patrolTarget.x - enemies[i].position.x) * (enemies[i].patrolTarget.x - enemies[i].position.x) +
                        (enemies[i].patrolTarget.y - enemies[i].position.y) * (enemies[i].patrolTarget.y - enemies[i].position.y)) < 10) {
                        enemies[i].patrolTarget = GetRandomPatrolTarget(enemies[i].position, ENEMY_PATROL_RADIUS);
                        Vector2 dir = { enemies[i].patrolTarget.x - enemies[i].position.x, enemies[i].patrolTarget.y - enemies[i].position.y };
                        float length = sqrtf(dir.x * dir.x + dir.y * dir.y);
                        if (length != 0) {
                            dir.x /= length;
                            dir.y /= length;
                        }
                        enemies[i].velocity.x = dir.x * ENEMY_SPEED;
                        enemies[i].velocity.y = dir.y * ENEMY_SPEED;
                    }
                }
            }
        }

        // --- Обработка пуль --- //
for (int i = 0; i < MAX_BULLETS; i++) {
    if (bullets[i].active) {
        bullets[i].position.x += bullets[i].velocity.x * deltaTime;
        bullets[i].position.y += bullets[i].velocity.y * deltaTime;
        if (bullets[i].position.x < 0 || bullets[i].position.x > mapWidth ||
            bullets[i].position.y < 0 || bullets[i].position.y > mapHeight) {
            bullets[i].active = false;
        }
        // столкновения с кирпичами и камнями
        for (int b = 0; b < NUM_BRICK_BLOCKS; b++) {
            if (brickBlocks[b].active) {
                Rectangle rect = { brickBlocks[b].position.x, brickBlocks[b].position.y, brickBlocks[b].size.x, brickBlocks[b].size.y };
                if (CheckCollisionPointRec(bullets[i].position, rect)) {
                    bullets[i].active = false;
                    brickBlocks[b].active = false;
                    Vector2 blockCenter = { brickBlocks[b].position.x + brickBlocks[b].size.x / 2,
                                                   brickBlocks[b].position.y + brickBlocks[b].size.y / 2 };
                    Vector2 playerCenter = { player.x + player.width / 2,
                                             player.y + player.height / 2 };
                    Vector2 diff = { playerCenter.x - blockCenter.x, playerCenter.y - blockCenter.y };
                    float length = sqrtf(diff.x * diff.x + diff.y * diff.y);
                    if (length != 0) {
                        float minDistance = 50.0f;
                        float maxDistance = 200.0f;
                        float impulseStrength;
                        if (length < minDistance) impulseStrength = 1000.0f;
                        else if (length > maxDistance) impulseStrength = 100.0f;
                        else {
                            float t = (length - minDistance) / (maxDistance - minDistance);
                            impulseStrength = 1000.0f * (1 - t) + 100.0f * t;
                        }
                        velocity.x += (diff.x / length) * impulseStrength;
                        velocity.y += (diff.y / length) * impulseStrength;
                    }
                    break;
                }
            }
        }
        for (int s = 0; s < NUM_STONE_BLOCKS; s++) {
            if (stoneBlocks[s].active) {
                Rectangle rect = { stoneBlocks[s].position.x, stoneBlocks[s].position.y, stoneBlocks[s].size.x, stoneBlocks[s].size.y };
                if (CheckCollisionPointRec(bullets[i].position, rect)) {
                    bullets[i].active = false;
                    break;
                }
            }
        }

        // Попадание по врагу (от игрока)
        if (bullets[i].fromPlayer) {
            for (int j = 0; j < NUM_ENEMIES; j++) {
                if (enemies[j].active) {
                    Rectangle enemyRect = { enemies[j].position.x, enemies[j].position.y, 50, 50 };
                    if (CheckCollisionPointRec(bullets[i].position, enemyRect)) {
                        enemies[j].active = false;
                        bullets[i].active = false;
                        break;
                    }
                }
            }
        }
        // Попадание по игроку (от врагов)
        else { // пули врагов
            Rectangle playerRect = { player.x, player.y, player.width, player.height };
            if (CheckCollisionPointRec(bullets[i].position, playerRect)) {
                bullets[i].active = false;
                // Можно уменьшить здоровье игрока или сделать что-то еще
                // Например: playerHealth--;
                break;
            }
        }
    }
}

        // Отталкивание с блоками
        for (int b = 0; b < NUM_BRICK_BLOCKS; b++) {
            if (brickBlocks[b].active) {
                Rectangle rect = { brickBlocks[b].position.x, brickBlocks[b].position.y, brickBlocks[b].size.x, brickBlocks[b].size.y };
                if (CheckCollisionRecs(player, rect)) {
                    Vector2 blockCenter = { brickBlocks[b].position.x + brickBlocks[b].size.x / 2, brickBlocks[b].position.y + brickBlocks[b].size.y / 2 };
                    Vector2 playerCenter = { player.x + player.width / 2, player.y + player.height / 2 };
                    Vector2 diff = { playerCenter.x - blockCenter.x, playerCenter.y - blockCenter.y };
                    float repelStrength = 100.0f;
                    float length = sqrtf(diff.x * diff.x + diff.y * diff.y);
                    if (length != 0) {
                        velocity.x += (diff.x / length) * repelStrength;
                        velocity.y += (diff.y / length) * repelStrength;
                    }
                }
            }
        }

        // Добавляем импульс
        velocity.x += impulse.x;
        velocity.y += impulse.y;
        // Плавное затухание импульса
        float impulseDamping = 0.1f;
        impulse.x += (0 - impulse.x) * impulseDamping;
        impulse.y += (0 - impulse.y) * impulseDamping;

        // Обновление позиции игрока
        player.x += velocity.x * deltaTime;
        player.y += velocity.y * deltaTime;

        // границы
        if (player.x < 0) {
            player.x = 0;
            velocity.x = -velocity.x;
        }
        else if (player.x + player.width > mapWidth) {
            player.x = mapWidth - player.width;
            velocity.x = -velocity.x;
        }

        if (player.y < 0) {
            player.y = 0;
            velocity.y = -velocity.y;
        }
        else if (player.y + player.height > mapHeight) {
            player.y = mapHeight - player.height;
            velocity.y = -velocity.y;
        }

        // --- ОПРОС КЛАВИШ --- //
        // Стрельба
        if (IsKeyPressed(KEY_ENTER)) {
            Vector2 startPos = { player.x, player.y };
            SpawnBullet(bullets, startPos, rotation, 600.0f, true);
        }

        // --- Обновление врагов --- //
        for (int i = 0; i < NUM_ENEMIES; i++) {
            if (enemies[i].active) {
                // Вращение врага
                enemies[i].rotationAngle += enemies[i].rotationDirection * enemies[i].rotationSpeed * deltaTime;
                if (enemies[i].rotationAngle > 360) enemies[i].rotationAngle -= 360;
                if (enemies[i].rotationAngle < 0) enemies[i].rotationAngle += 360;

                // Иногда менять направление вращения
                if (rand() % 1000 < 5) { // примерно 0.5% шанс за кадр
                    enemies[i].rotationDirection *= -1;
                }

                // Перемещение врага
                Vector2 newPos = {
                    enemies[i].position.x + enemies[i].velocity.x * deltaTime,
                    enemies[i].position.y + enemies[i].velocity.y * deltaTime
                };

                if (newPos.x < 0 || newPos.x > mapWidth - 50) {
                    enemies[i].velocity.x = -enemies[i].velocity.x;
                } else {
                    enemies[i].position.x = newPos.x;
                }
                if (newPos.y < 0 || newPos.y > mapHeight - 50) {
                    enemies[i].velocity.y = -enemies[i].velocity.y;
                } else {
                    enemies[i].position.y = newPos.y;
                }

                // Стрельба по игроку
                Vector2 playerPos = { player.x + player.width / 2, player.y + player.height / 2 };
                Vector2 toPlayer = { playerPos.x - enemies[i].position.x, playerPos.y - enemies[i].position.y };
                float distToPlayer = sqrtf(toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y);

                enemies[i].shootTimer -= deltaTime;

                if (distToPlayer <= ENEMY_VIEW_RADIUS) {
                    enemies[i].hasTarget = true;
                    enemies[i].patrolTarget = playerPos;

                    if (enemies[i].shootTimer <= 0) {
                        Vector2 startPos = { enemies[i].position.x + 25, enemies[i].position.y + 25 };
                        float angle = atan2f(toPlayer.y, toPlayer.x) * RAD2DEG;
                        SpawnBullet(bullets, startPos, angle, 300.0f, false);
                        enemies[i].shootTimer = 2.0f;
                    }
                } else {
                    enemies[i].hasTarget = false;
                    if (sqrtf((enemies[i].patrolTarget.x - enemies[i].position.x) * (enemies[i].patrolTarget.x - enemies[i].position.x) +
                        (enemies[i].patrolTarget.y - enemies[i].position.y) * (enemies[i].patrolTarget.y - enemies[i].position.y)) < 10) {
                        enemies[i].patrolTarget = GetRandomPatrolTarget(enemies[i].position, ENEMY_PATROL_RADIUS);
                        Vector2 dir = { enemies[i].patrolTarget.x - enemies[i].position.x, enemies[i].patrolTarget.y - enemies[i].position.y };
                        float length = sqrtf(dir.x * dir.x + dir.y * dir.y);
                        if (length != 0) {
                            dir.x /= length;
                            dir.y /= length;
                        }
                        enemies[i].velocity.x = dir.x * ENEMY_SPEED;
                        enemies[i].velocity.y = dir.y * ENEMY_SPEED;
                    }
                }
            }
        }

        // --- Отрисовка --- //
        BeginDrawing();
        ClearBackground(RAYWHITE);
        BeginMode2D(camera);

        DrawRectangleLinesEx(Rectangle{ 0, 0, (float)mapWidth, (float)mapHeight }, 3, DARKGRAY);
        Vector2 origin = { player.width / 2, player.height / 2 };
        DrawRectanglePro(player, origin, rotation, BLUE);
        DrawRectangleLinesEx(player, 2, GRAY);

        for (int b = 0; b < NUM_BRICK_BLOCKS; b++) {
            if (brickBlocks[b].active) {
                DrawRectangle(brickBlocks[b].position.x, brickBlocks[b].position.y, brickBlocks[b].size.x, brickBlocks[b].size.y, BROWN);
            }
        }

        for (int s = 0; s < NUM_STONE_BLOCKS; s++) {
            if (stoneBlocks[s].active) {
                DrawRectangle(stoneBlocks[s].position.x, stoneBlocks[s].position.y, stoneBlocks[s].size.x, stoneBlocks[s].size.y, RED);
            }
        }

        for (int i = 0; i < NUM_BUSHES; i++) {
            if (bushes[i].active && bushes[i].type == BLOCK_TYPE_BUSH) {
                Rectangle rect = { bushes[i].position.x, bushes[i].position.y, bushes[i].size.x, bushes[i].size.y };
                Vector2 playerCenter = { player.x + player.width / 2, player.y + player.height / 2 };
                bool inside = CheckCollisionPointRec(playerCenter, rect);

                if (inside && !bushes[i].inTransition) {
                    bushes[i].targetTransparency = 0.5f;
                    bushes[i].transitionProgress = 0.0f;
                    bushes[i].inTransition = true;
                }
                else if (!inside && !bushes[i].inTransition) {
                    bushes[i].targetTransparency = 1.0f;
                    bushes[i].transitionProgress = 0.0f;
                    bushes[i].inTransition = true;
                }

                if (bushes[i].inTransition) {
                    bushes[i].transitionProgress += deltaTime / TRANSITION_DURATION;
                    if (bushes[i].transitionProgress >= 1.0f) {
                        bushes[i].transitionProgress = 1.0f;
                        bushes[i].inTransition = false;
                        bushes[i].currentTransparency = bushes[i].targetTransparency;
                    }
                    else {
                        bushes[i].currentTransparency += (bushes[i].targetTransparency - bushes[i].currentTransparency) * bushes[i].transitionProgress;
                    }
                }
                else {
                    bushes[i].currentTransparency = bushes[i].targetTransparency;
                }

                Color bushColor = { 34, 139, 34, (unsigned char)(255 * bushes[i].currentTransparency) };
                DrawRectangleV(Vector2{ bushes[i].position.x, bushes[i].position.y }, bushes[i].size, bushColor);
            }
        }

        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                DrawCircleV(bullets[i].position, 5, RED);
            }
        }

        for (int i = 0; i < NUM_ENEMIES; i++) {
            if (enemies[i].active) {
                Rectangle rect = { enemies[i].position.x, enemies[i].position.y, 50, 50 };
                Vector2 origin = { 25, 25 };
                DrawRectanglePro(rect, origin, enemies[i].rotationAngle, DARKGREEN);
            }
        }

        EndMode2D();

        DrawText("Use W to move forward", 10, 10, 20, DARKGRAY);
        DrawText("Use S to move backward", 10, 40, 20, DARKGRAY);
        DrawText("Use A/D to rotate", 10, 70, 20, DARKGRAY);
        DrawText("Press ENTER to shoot", 10, 100, 20, DARKGRAY);
        DrawFPS(screenWidth - 90, 10);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
