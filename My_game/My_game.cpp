#include "raylib.h"
#include <math.h>
#include <stdlib.h> // для rand()
#include <time.h> // для time()

#define MAX_BULLETS 100
#define NUM_BRICK_BLOCKS 10
#define NUM_STONE_BLOCKS 1
#define NUM_BUSHES 3
#define NUM_ENEMIES 3
#define TRANSITION_DURATION 2.0f // время плавного перехода в секундах
#define ENEMY_SPEED 100.0f
#define ENEMY_VIEW_RADIUS 300.0f
#define ENEMY_FIRE_RADIUS 250.0f
#define ENEMY_PATROL_RADIUS 150.0f
int playerHits = 0;
int score = 0;
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
    float shootTimer; // таймер для стрельбы
    Vector2 patrolTarget; // точка патрулирования
    bool hasTarget; // есть ли текущая цель
} Enemy;

// Глобальная переменная для импульса
Vector2 impulse = { 0, 0 };

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

Vector2 GetRandomPatrolTarget(Vector2 currentPos, float radius) {
    float angle = (float)(rand() % 360) * DEG2RAD;
    return Vector2 { currentPos.x + cosf(angle) * radius, currentPos.y + sinf(angle) * radius };
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

    // Инициализация врагов с рандомными патрульными точками
    Enemy enemies[NUM_ENEMIES];
    for (int i = 0; i < NUM_ENEMIES; i++) {
        enemies[i].active = true;
        enemies[i].position.x = rand() % (mapWidth - 50);
        enemies[i].position.y = rand() % (mapHeight - 50);
        enemies[i].hasTarget = false;
        enemies[i].shootTimer = 2.0f;

        // Изначально задаем патрульную точку
        enemies[i].patrolTarget = GetRandomPatrolTarget(enemies[i].position, ENEMY_PATROL_RADIUS);
        // Изначально движение в этом направлении
        float randAngleDeg = (float)(rand() % 360);
        float randRad = DEG2RAD * randAngleDeg;
        enemies[i].velocity.x = cosf(randRad) * ENEMY_SPEED;
        enemies[i].velocity.y = sinf(randRad) * ENEMY_SPEED;
    }

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();
        camera.target = Vector2{ player.x + player.width / 2, player.y + player.height / 2 };
        camera.offset = Vector2{ screenWidth / 2, screenHeight / 2 };
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
            SpawnBullet(bullets, startPos, rotation, 600.0f);
        }

        // --- Обновление врагов --- //
        for (int i = 0; i < NUM_ENEMIES; i++) {
            if (enemies[i].active) {
                Vector2 playerPos = { player.x + player.width / 2, player.y + player.height / 2 };
                Vector2 toPlayer = { playerPos.x - enemies[i].position.x, playerPos.y - enemies[i].position.y };
                float distToPlayer = sqrtf(toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y);

                if (distToPlayer <= ENEMY_VIEW_RADIUS) {
                    // Игрок в зоне видимости - движемся к нему
                    enemies[i].hasTarget = true;
                    enemies[i].patrolTarget = playerPos;
                }
                else {
                    // Игрок вне зоны - патрулируем
                    if (!enemies[i].hasTarget ||
                        sqrtf((enemies[i].patrolTarget.x - enemies[i].position.x) * (enemies[i].patrolTarget.x - enemies[i].position.x) +
                            (enemies[i].patrolTarget.y - enemies[i].position.y) * (enemies[i].patrolTarget.y - enemies[i].position.y)) < 10) {
                        // выбираем новую точку патруля
                        enemies[i].patrolTarget = GetRandomPatrolTarget(enemies[i].position, ENEMY_PATROL_RADIUS);
                        enemies[i].hasTarget = false;
                        // задаем новое движение в сторону этой точки
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

                // Двигаемся
                Vector2 newPos = {
                    enemies[i].position.x + enemies[i].velocity.x * deltaTime,
                    enemies[i].position.y + enemies[i].velocity.y * deltaTime
                };

                // Ограничение границ
                if (newPos.x < 0 || newPos.x > mapWidth - 50) {
                    enemies[i].velocity.x = -enemies[i].velocity.x;
                }
                else {
                    enemies[i].position.x = newPos.x;
                }
                if (newPos.y < 0 || newPos.y > mapHeight - 50) {
                    enemies[i].velocity.y = -enemies[i].velocity.y;
                }
                else {
                    enemies[i].position.y = newPos.y;
                }

                // Стрельба в игрока, когда в зоне
                if (distToPlayer <= ENEMY_FIRE_RADIUS && enemies[i].shootTimer <= 0) {
                    Vector2 startPos = { enemies[i].position.x + 25, enemies[i].position.y + 25 };
                    float angle = atan2f(toPlayer.y, toPlayer.x) * RAD2DEG;
                    SpawnBullet(bullets, startPos, angle, 300.0f);
                    enemies[i].shootTimer = 2.0f; // интервал
                }

                // Таймер для стрельбы
                enemies[i].shootTimer -= deltaTime;
            }
        }

        // Обработка пуль
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
                            // импульс
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
            }
        }

        // Отталкивание с блоками (плавное отталкивание)
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

        // Обновление врагов с проверкой на пересечение друг с другом
        for (int i = 0; i < NUM_ENEMIES; i++) {
            if (enemies[i].active) {
                // Хаотичное движение: меняем направление случайно, если таймер истек
                if (enemies[i].shootTimer <= 0) {
                    float randAngleDeg = (float)(rand() % 360);
                    float randRad = DEG2RAD * randAngleDeg;
                    enemies[i].velocity.x = cosf(randRad) * ENEMY_SPEED;
                    enemies[i].velocity.y = sinf(randRad) * ENEMY_SPEED;
                    enemies[i].shootTimer = 1.0f + (rand() % 2);
                }

                // Обновляем таймер
                enemies[i].shootTimer -= deltaTime;

                // Передвижение
                Vector2 newPos = {
                    enemies[i].position.x + enemies[i].velocity.x * deltaTime,
                    enemies[i].position.y + enemies[i].velocity.y * deltaTime
                };

                // Ограничение по границам
                if (newPos.x < 0 || newPos.x > mapWidth - 50) {
                    enemies[i].velocity.x = -enemies[i].velocity.x;
                }
                else {
                    enemies[i].position.x = newPos.x;
                }

                if (newPos.y < 0 || newPos.y > mapHeight - 50) {
                    enemies[i].velocity.y = -enemies[i].velocity.y;
                }
                else {
                    enemies[i].position.y = newPos.y;
                }

                // Стрельба в игрока
                if (enemies[i].shootTimer <= 0) {
                    Vector2 startPos = { enemies[i].position.x + 25, enemies[i].position.y + 25 };
                    float angle = atan2f((player.y + player.height / 2) - startPos.y, (player.x + player.width / 2) - startPos.x) * RAD2DEG;
                    SpawnBullet(bullets, startPos, angle, 300.0f);
                    enemies[i].shootTimer = 2.0f;
                }
            }
        }

        // Обработка пуль
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
                            // импульс
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
            }
        }

        // Отталкивание с блоками (плавное отталкивание)
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

        // В начале цикла, после всех расчетов, добавляем импульс к velocity
        velocity.x += impulse.x;
        velocity.y += impulse.y;

        // Обновление позиции
        player.x += velocity.x * deltaTime;
        player.y += velocity.y * deltaTime;

        // Плавное затухание импульса
        float impulseDamping = 0.1f;
        impulse.x += (0 - impulse.x) * impulseDamping;
        impulse.y += (0 - impulse.y) * impulseDamping;


        // В конце цикла
        // --- Отрисовка ---
        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode2D(camera);

        // Рисуем границы карты
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
                DrawRectangle(stoneBlocks[s].position.x, stoneBlocks[s].position.y, stoneBlocks[s].size.x, stoneBlocks[s].size.y, DARKGRAY);
            }
        }

        // Обработка кустов с плавным переходом
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

        // Пули
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                DrawCircleV(bullets[i].position, 5, RED);
            }
        }

        // Враги
        for (int i = 0; i < NUM_ENEMIES; i++) {
            if (enemies[i].active) {
                DrawRectangle(enemies[i].position.x, enemies[i].position.y, 50, 50, DARKGREEN);               
            }
        }

        EndMode2D();

        // Тексты и FPS
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
