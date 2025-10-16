#include "raylib.h"
#include <math.h>

#define MAX_BULLETS 100
#define NUM_BRICK_BLOCKS 5
#define NUM_STONE_BLOCKS 1
#define NUM_BUSHES 3

typedef struct {
    Vector2 position;
    Vector2 size;
    bool active; // для кирпичных блоков
} BrickBlock;

typedef struct {
    Vector2 position;
    Vector2 size;
    bool active; // для каменных блоков
} StoneBlock;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    bool active;
} Bullet;

typedef enum {
    BLOCK_TYPE_NORMAL,
    BLOCK_TYPE_BRICK,
    BLOCK_TYPE_BUSH // добавляем тип "кусты"
} BlockType;

typedef struct {
    Vector2 position;
    Vector2 size;
    bool active;
    float transparency; // 0..1
    BlockType type;
} BushBlock;

void SpawnBullet(Bullet bullets[], Vector2 startPos, float angleDeg, float bulletSpeed) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            bullets[i].active = true;
            bullets[i].position = startPos;
            float angleRad = DEG2RAD * angleDeg;
            bullets[i].velocity.x = cosf(angleRad) * bulletSpeed;
            bullets[i].velocity.y = sinf(angleRad) * bulletSpeed;
            break;
        }
    }
}

int main(void) {
    const int screenWidth = 800;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "Rotation with A/D keys");
    DisableCursor();
    SetTargetFPS(60);

    // Начальные параметры фигуры
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
    for (int i = 0; i < MAX_BULLETS; i++) {
        bullets[i].active = false;
    }

    float rotation = 0.0f; // угол фигуры в градусах
    const float rotationSpeed = 90.0f; // скорость вращения

    // Создаем кирпичные блоки
    BrickBlock brickBlocks[NUM_BRICK_BLOCKS] = {
        { Vector2 { 100, 100 }, Vector2 { 50, 50 }, true },
        { Vector2 { 300, 200 }, Vector2 { 80, 80 }, true },
        { Vector2 { 500, 400 }, Vector2 { 60, 60 }, true },
        { Vector2 { 200, 500 }, Vector2 { 70, 40 }, true },
        { Vector2 { 600, 150 }, Vector2 { 50, 100 }, true }
    };

    // Создаем каменный блок
    StoneBlock stoneBlocks[NUM_STONE_BLOCKS] = {
        { Vector2 { 400, 100 }, Vector2 { 60, 60 }, true }
    };

    // Создаем "кусты"
    BushBlock bushes[NUM_BUSHES] = {
        { Vector2 { 150, 150 }, Vector2 { 80, 80 }, true, 1.0f, BLOCK_TYPE_BUSH },
        { Vector2 { 400, 300 }, Vector2 { 100, 50 }, true, 1.0f, BLOCK_TYPE_BUSH },
        { Vector2 { 600, 300 }, Vector2 { 100, 100 }, true, 1.0f, BLOCK_TYPE_BUSH }
    };

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();
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

        // Ограничение скорости
        float speed = sqrtf(velocity.x * velocity.x + velocity.y * velocity.y);
        if (speed > maxSpeed) {
            velocity.x = (velocity.x / speed) * maxSpeed;
            velocity.y = (velocity.y / speed) * maxSpeed;
        }

        // Применяем трение
        velocity.x *= friction;
        velocity.y *= friction;

        // Минимальное значение скорости
        if (fabsf(velocity.x) < 0.5f) velocity.x = 0;
        if (fabsf(velocity.y) < 0.5f) velocity.y = 0;

        // Обновляем позицию
        player.x += velocity.x * deltaTime;
        player.y += velocity.y * deltaTime;

        // Ограничение границ экрана
        if (player.x < 0) {
            player.x = 0;
            velocity.x = -velocity.x;
        }
        else if (player.x + player.width > screenWidth) {
            player.x = screenWidth - player.width;
            velocity.x = -velocity.x;
        }

        if (player.y < 0) {
            player.y = 0;
            velocity.y = -velocity.y;
        }
        else if (player.y + player.height > screenHeight) {
            player.y = screenHeight - player.height;
            velocity.y = -velocity.y;
        }

        // Стрельба
        if (IsKeyPressed(KEY_ENTER)) {
            Vector2 startPos = { player.x, player.y };
            SpawnBullet(bullets, startPos, rotation, 600.0f);
        }

        // Обновление пуль и проверка столкновений
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                bullets[i].position.x += bullets[i].velocity.x * deltaTime;
                bullets[i].position.y += bullets[i].velocity.y * deltaTime;

                // Проверка выхода за границы
                if (bullets[i].position.x < 0 || bullets[i].position.x > screenWidth ||
                    bullets[i].position.y < 0 || bullets[i].position.y > screenHeight) {
                    bullets[i].active = false;
                }

                // Столкновение с кирпичными блоками
                for (int b = 0; b < NUM_BRICK_BLOCKS; b++) {
                    if (brickBlocks[b].active) {
                        Rectangle rect = { brickBlocks[b].position.x, brickBlocks[b].position.y, brickBlocks[b].size.x, brickBlocks[b].size.y };
                        if (CheckCollisionPointRec(bullets[i].position, rect)) {
                            bullets[i].active = false;
                            brickBlocks[b].active = false; // исчезает
                            // расчет центра блока и фигуры
                            Vector2 blockCenter = { brickBlocks[b].position.x + brickBlocks[b].size.x / 2,
                                                    brickBlocks[b].position.y + brickBlocks[b].size.y / 2 };
                            Vector2 playerCenter = { player.x + player.width / 2,
                                                     player.y + player.height / 2 };
                            Vector2 diff = { playerCenter.x - blockCenter.x,
                                             playerCenter.y - blockCenter.y };
                            float length = sqrtf(diff.x * diff.x + diff.y * diff.y);
                            if (length != 0) {
                                float minDistance = 50.0f;
                                float maxDistance = 200.0f;
                                float impulseStrength;

                                if (length < minDistance) {
                                    impulseStrength = 1000.0f;
                                }
                                else if (length > maxDistance) {
                                    impulseStrength = 100.0f;
                                }
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

                // Столкновение с каменным блоком
                for (int s = 0; s < NUM_STONE_BLOCKS; s++) {
                    if (stoneBlocks[s].active) {
                        Rectangle rect = { stoneBlocks[s].position.x, stoneBlocks[s].position.y, stoneBlocks[s].size.x, stoneBlocks[s].size.y };
                        if (CheckCollisionPointRec(bullets[i].position, rect)) {
                            bullets[i].active = false;
                            break;
                        }
                    }
                }
            }
        }

        // Проверка столкновений с кирпичными блоками (отталкивание)
        for (int b = 0; b < NUM_BRICK_BLOCKS; b++) {
            if (brickBlocks[b].active) {
                Rectangle rect = { brickBlocks[b].position.x, brickBlocks[b].position.y, brickBlocks[b].size.x, brickBlocks[b].size.y };
                if (CheckCollisionRecs(player, rect)) {
                    Vector2 blockCenter = { brickBlocks[b].position.x + brickBlocks[b].size.x / 2, brickBlocks[b].position.y + brickBlocks[b].size.y / 2 };
                    Vector2 playerCenter = { player.x + player.width / 2, player.y + player.height / 2 };
                    Vector2 diff = { playerCenter.x - blockCenter.x, playerCenter.y - blockCenter.y };
                    float repelStrength = 300.0f;
                    float length = sqrtf(diff.x * diff.x + diff.y * diff.y);
                    if (length != 0) {
                        velocity.x += (diff.x / length) * repelStrength;
                        velocity.y += (diff.y / length) * repelStrength;
                    }
                }
            }
        }

        // Проверка столкновений с каменными блоками (отталкивание)
        for (int s = 0; s < NUM_STONE_BLOCKS; s++) {
            if (stoneBlocks[s].active) {
                Rectangle rect = { stoneBlocks[s].position.x, stoneBlocks[s].position.y, stoneBlocks[s].size.x, stoneBlocks[s].size.y };
                if (CheckCollisionRecs(player, rect)) {
                    Vector2 blockCenter = { stoneBlocks[s].position.x + stoneBlocks[s].size.x / 2, stoneBlocks[s].position.y + stoneBlocks[s].size.y / 2 };
                    Vector2 playerCenter = { player.x + player.width / 2, player.y + player.height / 2 };
                    Vector2 diff = { playerCenter.x - blockCenter.x, playerCenter.y - blockCenter.y };
                    float repelForce = 500.0f;
                    float length = sqrtf(diff.x * diff.x + diff.y * diff.y);
                    if (length != 0) {
                        velocity.x += (diff.x / length) * repelForce;
                        velocity.y += (diff.y / length) * repelForce;
                    }
                }
            }
        }

        // --- Отрисовка ---
        BeginDrawing();
        ClearBackground(RAYWHITE);

        Vector2 origin = { player.width / 2, player.height / 2 };
        DrawRectanglePro(player, origin, rotation, BLUE);
        DrawRectangleLinesEx(player, 2, GRAY);

        // Рисуем кирпичные блоки
        for (int b = 0; b < NUM_BRICK_BLOCKS; b++) {
            if (brickBlocks[b].active) {
                DrawRectangle(brickBlocks[b].position.x, brickBlocks[b].position.y, brickBlocks[b].size.x, brickBlocks[b].size.y, BROWN);
            }
        }

        // Рисуем каменный блок
        for (int s = 0; s < NUM_STONE_BLOCKS; s++) {
            if (stoneBlocks[s].active) {
                DrawRectangle(stoneBlocks[s].position.x, stoneBlocks[s].position.y, stoneBlocks[s].size.x, stoneBlocks[s].size.y, DARKGRAY);
            }
        }

        // Рисуем кусты с учетом прозрачности
        for (int i = 0; i < NUM_BUSHES; i++) {
            if (bushes[i].active) {
                Color bushColor = { 34, 139, 34, (unsigned char)(255 * bushes[i].transparency) };
                DrawRectangleV(Vector2 { bushes[i].position.x, bushes[i].position.y }, bushes[i].size, bushColor);
            }
        }

        // Рисуем пули
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                DrawCircleV(bullets[i].position, 5, RED);
            }
        }

        DrawText("Use W to move forward", 10, 10, 20, DARKGRAY);
        DrawText("Use S to move backward", 10, 40, 20, DARKGRAY);
        DrawText("Use A/D to rotate", 10, 70, 20, DARKGRAY);
        DrawText("Press ENTER to shoot", 10, 100, 20, DARKGRAY);
        DrawFPS(screenWidth - 90, 10);

        // Проверка попадания фигуры внутрь кустов
        for (int i = 0; i < NUM_BUSHES; i++) {
            if (bushes[i].active && bushes[i].type == BLOCK_TYPE_BUSH) {
                Rectangle rect = { bushes[i].position.x, bushes[i].position.y, bushes[i].size.x, bushes[i].size.y };
                Vector2 playerCenter = { player.x + player.width / 2, player.y + player.height / 2 };
                if (CheckCollisionPointRec(playerCenter, rect)) {
                    bushes[i].transparency = 0.5f; // полупрозрачный
                }
                else {
                    bushes[i].transparency = 1.0f; // полностью непрозрачный
                }
            }
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
