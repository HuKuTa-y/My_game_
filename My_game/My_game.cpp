#include "raylib.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#define MAX_BULLETS 100
#define NUM_BRICK_BLOCKS 9
#define NUM_STONE_BLOCKS 4
#define NUM_BUSHES 6
#define NUM_ENEMIES 3
#define TRANSITION_DURATION 2.0f
#define ENEMY_SPEED 100.0f
#define ENEMY_VIEW_RADIUS 300.0f
#define ENEMY_FIRE_RADIUS 250.0f
#define ENEMY_PATROL_RADIUS 150.0f

float bulletAngle = 0.0f;
int enemiesKilled = 0;
int playerHits = 0;
int score = 0; // счетчик очков

typedef struct {
    Vector2 position;
    Vector2 size;
    bool active;
} BrickBlock;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    bool active;
    float shootTimer;
    Vector2 patrolTarget;
    bool hasTarget;
    float rotationAngle;
    float rotationSpeed;
    int rotationDirection;
    bool exploding;            // идет анимация взрыва
    float explosionTimer;      // таймер анимации
    int explosionFrame;        // текущий кадр
} EnemyExtended;

typedef struct {
    Vector2 position;
    Vector2 size;
    bool active;
} StoneBlock;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    bool active;
    bool fromPlayer;
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
    float rotation = 0.0f;
    float rotation2 = 0.0f; // вращение второго игрока
    int playerLives1 = 150; // жизни первого игрока
    int playerLives2 = 500; // жизни второго игрока
    int points = 0; // счетчик очков
    srand((unsigned int)time(NULL));
    const int screenWidth = 1400;
    const int screenHeight = 1000;
    const int mapWidth = 1400;
    const int mapHeight = 1000;

    InitWindow(screenWidth, screenHeight, "AI Enemies");
    Texture2D playerTexture = LoadTexture("tanks.png");
    Texture2D bulletTexture = LoadTexture("pull.png");
    Texture2D map = LoadTexture("maps.png");
    Texture2D bushTexture = LoadTexture("kust.png");
    Texture2D stoneTexture = LoadTexture("stone.png");
    Texture2D bricks = LoadTexture("broke.png");
    DisableCursor();
    SetTargetFPS(60);

    Rectangle player = {
        (float)screenWidth / 2 - 35,
        (float)screenHeight / 2 - 25,
        50, 40
    };

    Rectangle player2 = {
        (float)screenWidth / 2 + 100,
        (float)screenHeight / 2 - 25,
        50, 40
    };
    Vector2 velocity2 = { 0.0f, 0.0f };

    Vector2 velocity = { 0.0f, 0.0f };
    const float maxSpeed = 400.0f;
    const float acceleration = 1200.0f;
    const float friction = 0.89f;

    Bullet bullets[MAX_BULLETS] = { 0 };
    for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = false;

    float rotationSpeed = 90.0f;

    // Размер кирпичных блоков
    const int brickSize = 50; // Размер стороны квадрата кирпича

    BrickBlock brickBlocks[NUM_BRICK_BLOCKS] = {
        { Vector2 { 300, 100 }, Vector2 { brickSize, brickSize }, true },
        { Vector2 { 900, 600 }, Vector2 { brickSize, brickSize }, true },
        { Vector2 { 1200, 700 }, Vector2 { 40, 40 }, true },
        { Vector2 { 600, 300 }, Vector2 { 50, 100 }, true },
        { Vector2 { 100, 750 }, Vector2 { 50, 100 }, true },
        { Vector2 { 200, 50 }, Vector2 { 50, 100 }, true },
        { Vector2 { 500, 450 }, Vector2 { 50, 100 }, true },
        { Vector2 { 1350, 450 }, Vector2 { 50, 100 }, true },
        { Vector2 { 850, 950 }, Vector2 { 50, 100 }, true },
    };

    StoneBlock stoneBlocks[NUM_STONE_BLOCKS] = {
        { Vector2 { 1300, 900 }, Vector2 { 60, 60 }, true },
        { Vector2 { 500, 350 }, Vector2 { 60, 60 }, true },
        { Vector2 { 890, 690}, Vector2 { 60, 60 }, true },
        { Vector2 { 1000, 550}, Vector2 { 60, 60 }, true },
    };

    BushBlock bushes[NUM_BUSHES] = {
        { Vector2 { 150, 150 }, Vector2 { 80, 80 }, true, 1.0f, 1.0f, 0.0f, false, BLOCK_TYPE_BUSH },
        { Vector2 { 400, 300 }, Vector2 { 100, 50 }, true, 1.0f, 1.0f, 0.0f, false, BLOCK_TYPE_BUSH },
        { Vector2 { 600, 850 }, Vector2 { 100, 100 }, true, 1.0f, 1.0f, 0.0f, false, BLOCK_TYPE_BUSH },
        { Vector2 { 900, 650 }, Vector2 { 80, 80 }, true, 1.0f, 1.0f, 0.0f, false, BLOCK_TYPE_BUSH },
        { Vector2 { 1250, 970 }, Vector2 { 100, 50 }, true, 1.0f, 1.0f, 0.0f, false, BLOCK_TYPE_BUSH },
        { Vector2 { 1250, 270 }, Vector2 { 100, 100 }, true, 1.0f, 1.0f, 0.0f, false, BLOCK_TYPE_BUSH }
    };

    //Camera2D camera = { 0 };

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

    while (!WindowShouldClose()) {
        bool playerHitEnemy1 = false;
        bool player2HitEnemy1 = false;
        float deltaTime = GetFrameTime();
        //camera.target = Vector2{ player.x + player.width / 2, player.y + player.height / 2 };
        //camera.offset = Vector2{ (float)screenWidth / 2, (float)screenHeight / 2 };
        //camera.zoom = 1.0f;

        float radians = DEG2RAD * rotation;

        // Управление первым игроком
        bool rotateLeft = IsKeyDown(KEY_A);
        bool rotateRight = IsKeyDown(KEY_D);
        bool moveForward = IsKeyDown(KEY_W);
        bool moveBackward = IsKeyDown(KEY_S);

        // Управление вторым игроком стрелками
        bool rotateLeft2 = IsKeyDown(KEY_LEFT);
        bool rotateRight2 = IsKeyDown(KEY_RIGHT);
        bool moveForward2 = IsKeyDown(KEY_UP);
        bool moveBackward2 = IsKeyDown(KEY_DOWN);

        // Обработка стрельбы для первого
        static bool prevShoot1 = false;
        bool shoot1 = IsKeyDown(KEY_ENTER);
        if (shoot1 && !prevShoot1) {
            Vector2 startPos = { player.x + player.width / 2, player.y + player.height / 2 };
            SpawnBullet(bullets, startPos, rotation, 600.0f, true);
        }
        prevShoot1 = shoot1;

        // Обработка стрельбы для второго
        static bool prevShoot2 = false;
        bool shoot2 = IsKeyDown(KEY_RIGHT_SHIFT);
        if (shoot2 && !prevShoot2) {
            Vector2 startPos = { player2.x + player2.width / 2, player2.y + player2.height / 2 };
            SpawnBullet(bullets, startPos, rotation2, 600.0f, false);
        }
        prevShoot2 = shoot2;

        // Перед обработкой ввода проверяем наличие геймпадов
        int gamepad1 = 0;
        int gamepad2 = 1;
        bool gp1Available = IsGamepadAvailable(gamepad1);
        bool gp2Available = IsGamepadAvailable(gamepad2);

        // Управление через джойстик для первого
        if (gp1Available) {
            float axisX = GetGamepadAxisMovement(gamepad1, GAMEPAD_AXIS_LEFT_X);
            float axisY = GetGamepadAxisMovement(gamepad1, GAMEPAD_AXIS_LEFT_Y);
            if (fabsf(axisX) > 0.2f) {
                rotation += axisX * rotationSpeed * deltaTime;
            }
            if (fabsf(axisY) > 0.2f) {
                float dirX = cosf(DEG2RAD * rotation);
                float dirY = sinf(DEG2RAD * rotation);
                velocity.x += dirX * (-axisY) * acceleration * deltaTime;
                velocity.y += dirY * (-axisY) * acceleration * deltaTime;
            }
            if (IsGamepadButtonDown(gamepad1, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
                Vector2 startPos = { player.x + player.width / 2, player.y + player.height / 2 };
                SpawnBullet(bullets, startPos, rotation, 600.0f, true);
            }
        }
        // Управление через джойстик для второго
        if (gp2Available) {
            float axisX2 = GetGamepadAxisMovement(gamepad2, GAMEPAD_AXIS_LEFT_X);
            float axisY2 = GetGamepadAxisMovement(gamepad2, GAMEPAD_AXIS_LEFT_Y);
            if (fabsf(axisX2) > 0.2f) {
                rotation2 += axisX2 * rotationSpeed * deltaTime;
            }
            if (fabsf(axisY2) > 0.2f) {
                float dirX = cosf(DEG2RAD * rotation2);
                float dirY = sinf(DEG2RAD * rotation2);
                velocity2.x += dirX * (-axisY2) * acceleration * deltaTime;
                velocity2.y += dirY * (-axisY2) * acceleration * deltaTime;
            }
            if (IsGamepadButtonDown(gamepad2, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
                Vector2 startPos = { player2.x + player2.width / 2, player2.y + player2.height / 2 };
                SpawnBullet(bullets, startPos, rotation2, 600.0f, false);
            }
        }

        // Вращение стрелками
        if (rotateLeft2) rotation2 -= rotationSpeed * deltaTime;
        if (rotateRight2) rotation2 += rotationSpeed * deltaTime;
        if (rotateLeft) rotation -= rotationSpeed * deltaTime;
        if (rotateRight) rotation += rotationSpeed * deltaTime;

        // Движение игроков
        if (moveForward2) {
            float dirX = cosf(DEG2RAD * rotation2);
            float dirY = sinf(DEG2RAD * rotation2);
            velocity2.x += dirX * acceleration * deltaTime;
            velocity2.y += dirY * acceleration * deltaTime;
        }
        if (moveBackward2) {
            float dirX = cosf(DEG2RAD * rotation2);
            float dirY = sinf(DEG2RAD * rotation2);
            velocity2.x -= dirX * acceleration * deltaTime;
            velocity2.y -= dirY * acceleration * deltaTime;
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

        // Ограничение скорости
        float speed = sqrtf(velocity.x * velocity.x + velocity.y * velocity.y);
        if (speed > maxSpeed) {
            velocity.x = (velocity.x / speed) * maxSpeed;
            velocity.y = (velocity.y / speed) * maxSpeed;
        }
        float speed2 = sqrtf(velocity2.x * velocity2.x + velocity2.y * velocity2.y);
        if (speed2 > maxSpeed) {
            velocity2.x = (velocity2.x / speed2) * maxSpeed;
            velocity2.y = (velocity2.y / speed2) * maxSpeed;
        }

        // Фрикция
        velocity.x *= friction;
        velocity.y *= friction;
        if (fabsf(velocity.x) < 0.5f) velocity.x = 0;
        if (fabsf(velocity.y) < 0.5f) velocity.y = 0;
        velocity2.x *= friction;
        velocity2.y *= friction;
        if (fabsf(velocity2.x) < 0.5f) velocity2.x = 0;
        if (fabsf(velocity2.y) < 0.5f) velocity2.y = 0;

        // Обновление позиций игроков
        player.x += velocity.x * deltaTime;
        player.y += velocity.y * deltaTime;
        player2.x += velocity2.x * deltaTime;
        player2.y += velocity2.y * deltaTime;

        // Ограничения границ для первого
        if (player.x < 0) { player.x = 0; velocity.x = -velocity.x; }
        if (player.x + player.width > mapWidth) { player.x = mapWidth - player.width; velocity.x = -velocity.x; }
        if (player.y < 0) { player.y = 0; velocity.y = -velocity.y; }
        if (player.y + player.height > mapHeight) { player.y = mapHeight - player.height; velocity.y = -velocity.y; }

        // Ограничения границ для второго
        if (player2.x < 0) { player2.x = 0; velocity2.x = -velocity2.x; }
        if (player2.x + player2.width > mapWidth) { player2.x = mapWidth - player2.width; velocity2.x = -velocity2.x; }
        if (player2.y < 0) { player2.y = 0; velocity2.y = -velocity2.y; }
        if (player2.y + player2.height > mapHeight) { player2.y = mapHeight - player2.height; velocity2.y = -velocity2.y; }

        // --- Обновление врагов --- //
        for (int i = 0; i < NUM_ENEMIES; i++) {
            if (enemies[i].active) {
                // Вращение врага
                enemies[i].rotationAngle += enemies[i].rotationDirection * enemies[i].rotationSpeed * deltaTime;
                if (enemies[i].rotationAngle > 360) enemies[i].rotationAngle -= 360;
                if (enemies[i].rotationAngle < 0) enemies[i].rotationAngle += 360;

                // Иногда менять направление вращения
                if (rand() % 1000 < 5) {
                    enemies[i].rotationDirection *= -1;
                }

                // Обновление позиции на основе текущего velocity
                enemies[i].position.x += enemies[i].velocity.x * deltaTime;
                enemies[i].position.y += enemies[i].velocity.y * deltaTime;

                // Проверка границ и отражение
                bool changedDirection = false;
                if (enemies[i].position.x < 0) {
                    enemies[i].position.x = 0;
                    enemies[i].velocity.x = -enemies[i].velocity.x;
                    changedDirection = true;
                }
                if (enemies[i].position.x > mapWidth - 50) {
                    enemies[i].position.x = mapWidth - 50;
                    enemies[i].velocity.x = -enemies[i].velocity.x;
                    changedDirection = true;
                }
                if (enemies[i].position.y < 0) {
                    enemies[i].position.y = 0;
                    enemies[i].velocity.y = -enemies[i].velocity.y;
                    changedDirection = true;
                }
                if (enemies[i].position.y > mapHeight - 50) {
                    enemies[i].position.y = mapHeight - 50;
                    enemies[i].velocity.y = -enemies[i].velocity.y;
                    changedDirection = true;
                }

                // Если враг повернул (отражение или смена цели), то можно чуть изменить rotation для плавности

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
                }
                else {
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

        // Обработка столкновения между игроками и отталкивания
        {
            Rectangle rect1 = { player.x, player.y, player.width, player.height };
            Rectangle rect2 = { player2.x, player2.y, player2.width, player2.height };

            if (CheckCollisionRecs(rect1, rect2)) {
                Vector2 center1 = { player.x + player.width / 2, player.y + player.height / 2 };
                Vector2 center2 = { player2.x + player2.width / 2, player2.y + player2.height / 2 };
                Vector2 diff = { center1.x - center2.x, center1.y - center2.y };
                float dist = sqrtf(diff.x * diff.x + diff.y * diff.y);
                if (dist != 0) {
                    float pushStrength = 20.0f; // сила отталкивания
                    float overlap = (player.width + player2.width) / 2 - dist;
                    // Расчет направления
                    Vector2 pushDir = { diff.x / dist, diff.y / dist };
                    // Отталкиваем каждого игрока
                    player.x += pushDir.x * overlap * 0.5f;
                    player.y += pushDir.y * overlap * 0.5f;
                    player2.x -= pushDir.x * overlap * 0.5f;
                    player2.y -= pushDir.y * overlap * 0.5f;
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
                            // Импульс только если пуля от игрока
                            if (bullets[i].fromPlayer) {
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
                            }
                            break;
                        }
                    }
                }
                // Аналогично для каменных блоков
                for (int s = 0; s < NUM_STONE_BLOCKS; s++) {
                    if (stoneBlocks[s].active) {
                        Rectangle rect = { stoneBlocks[s].position.x, stoneBlocks[s].position.y, stoneBlocks[s].size.x, stoneBlocks[s].size.y };
                        if (CheckCollisionRecs(player, rect)) {
                            Vector2 blockCenter = { stoneBlocks[s].position.x + stoneBlocks[s].size.x / 2,
                                                    stoneBlocks[s].position.y + stoneBlocks[s].size.y / 2 };
                            Vector2 playerCenter = { player.x + player.width / 2, player.y + player.height / 2 };
                            Vector2 diff = { playerCenter.x - blockCenter.x, playerCenter.y - blockCenter.y };
                            float length = sqrtf(diff.x * diff.x + diff.y * diff.y);
                            if (length != 0) {
                                float pushStrength = 100.0f;
                                velocity.x += (diff.x / length) * pushStrength;
                                velocity.y += (diff.y / length) * pushStrength;
                            }
                        }
                    }
                }
                // столкновение с врагами
                if (bullets[i].fromPlayer) {
                    for (int j = 0; j < NUM_ENEMIES; j++) {
                        if (enemies[j].active) {
                            Rectangle enemyRect = { enemies[j].position.x, enemies[j].position.y, 50, 50 };
                            if (CheckCollisionPointRec(bullets[i].position, enemyRect)) {
                                enemies[j].active = false;
                                bullets[i].active = false;
                                enemiesKilled++;
                                score += 100; // очки за врага
                                break;
                            }
                        }
                    }
                }
                else {
                    // Враговые пули по игрокам
                    Rectangle rectPlayer1 = { player.x, player.y, player.width, player.height };
                    Rectangle rectPlayer2 = { player2.x, player2.y, player2.width, player2.height };
                    if (CheckCollisionPointRec(bullets[i].position, rectPlayer1)) {
                        printf("Пуля врага попала в первого игрока! Координаты пули: (%.2f, %.2f)\n", bullets[i].position.x, bullets[i].position.y);
                        printf("До попадания: Жизни первого игрока: %d\n", playerLives1);
                        bullets[i].active = false;
                        playerLives1--;
                        printf("После попадания: Жизни первого игрока: %d\n", playerLives1);
                    }
                    if (CheckCollisionPointRec(bullets[i].position, rectPlayer2)) {
                        printf("Пуля врага попала во второго игрока! Координаты пули: (%.2f, %.2f)\n", bullets[i].position.x, bullets[i].position.y);
                        printf("До попадания: Жизни второго игрока: %d\n", playerLives2);
                        bullets[i].active = false;
                        playerLives2--;
                        printf("После попадания: Жизни второго игрока: %d\n", playerLives2);
                    }
                }
            }
        }

        // Отталкивание с блоками для игроков (только для игроков)
        for (int b = 0; b < NUM_BRICK_BLOCKS; b++) {
            if (brickBlocks[b].active) {
                Rectangle rect = { brickBlocks[b].position.x, brickBlocks[b].position.y, brickBlocks[b].size.x, brickBlocks[b].size.y };
                if (CheckCollisionRecs(player, rect)) {
                    Vector2 blockCenter = { brickBlocks[b].position.x + brickBlocks[b].size.x / 2,
                                              brickBlocks[b].position.y + brickBlocks[b].size.y / 2 };
                    Vector2 playerCenter = { player.x + player.width / 2, player.y + player.height / 2 };
                    Vector2 diff = { playerCenter.x - blockCenter.x, playerCenter.y - blockCenter.y };
                    float length = sqrtf(diff.x * diff.x + diff.y * diff.y);
                    if (length != 0) {
                        float pushStrength = 100.0f;
                        velocity.x += (diff.x / length) * pushStrength;
                        velocity.y += (diff.y / length) * pushStrength;
                    }
                }
            }
        }

        // После движения игроков — повторно проверяем столкновение и импульс только для первого игрока
        for (int b = 0; b < NUM_BRICK_BLOCKS; b++) {
            if (brickBlocks[b].active) {
                Rectangle rect = { brickBlocks[b].position.x, brickBlocks[b].position.y, brickBlocks[b].size.x, brickBlocks[b].size.y };
                if (CheckCollisionRecs(player, rect)) {
                    Vector2 blockCenter = { brickBlocks[b].position.x + brickBlocks[b].size.x / 2,
                                              brickBlocks[b].position.y + brickBlocks[b].size.y / 2 };
                    Vector2 playerCenter = { player.x + player.width / 2, player.y + player.height / 2 };
                    Vector2 diff = { playerCenter.x - blockCenter.x, playerCenter.y - blockCenter.y };
                    float length = sqrtf(diff.x * diff.x + diff.y * diff.y);
                    if (length != 0) {
                        float pushStrength = 100.0f;
                        velocity.x += (diff.x / length) * pushStrength;
                        velocity.y += (diff.y / length) * pushStrength;
                    }
                }
            }
        }
        // Проверка столкновений с камнями для первого игрока
        for (int b = 0; b < NUM_STONE_BLOCKS; b++) {
            if (stoneBlocks[b].active) {
                Rectangle stoneRect = { stoneBlocks[b].position.x, stoneBlocks[b].position.y, stoneBlocks[b].size.x, stoneBlocks[b].size.y };
                Rectangle playerRect = { player.x, player.y, player.width, player.height };
                if (CheckCollisionRecs(playerRect, stoneRect)) {
                    // Вычисляем пересечение
                    float overlapX = 0;
                    float overlapY = 0;

                    // Центральные точки
                    Vector2 playerCenter = { player.x + player.width / 2, player.y + player.height / 2 };
                    Vector2 blockCenter = { stoneBlocks[b].position.x + stoneBlocks[b].size.x / 2, stoneBlocks[b].position.y + stoneBlocks[b].size.y / 2 };
                    Vector2 diff = { playerCenter.x - blockCenter.x, playerCenter.y - blockCenter.y };

                    // Находим минимальное смещение, чтобы убрать пересечение
                    float dx = (stoneBlocks[b].size.x / 2 + player.width / 2) - fabsf(diff.x);
                    float dy = (stoneBlocks[b].size.y / 2 + player.height / 2) - fabsf(diff.y);

                    if (dx < dy) {
                        // Смещаем по X
                        if (diff.x > 0) {
                            player.x += dx;
                        }
                        else {
                            player.x -= dx;
                        }
                    }
                    else {
                        // Смещаем по Y
                        if (diff.y > 0) {
                            player.y += dy;
                        }
                        else {
                            player.y -= dy;
                        }
                    }
                }
            }
        }

        // Аналогично для второго игрока
        for (int b = 0; b < NUM_STONE_BLOCKS; b++) {
            if (stoneBlocks[b].active) {
                Rectangle stoneRect = { stoneBlocks[b].position.x, stoneBlocks[b].position.y, stoneBlocks[b].size.x, stoneBlocks[b].size.y };
                Rectangle playerRect2 = { player2.x, player2.y, player2.width, player2.height };
                if (CheckCollisionRecs(playerRect2, stoneRect)) {
                    float overlapX = 0;
                    float overlapY = 0;

                    Vector2 player2Center = { player2.x + player2.width / 2, player2.y + player2.height / 2 };
                    Vector2 blockCenter = { stoneBlocks[b].position.x + stoneBlocks[b].size.x / 2, stoneBlocks[b].position.y + stoneBlocks[b].size.y / 2 };
                    Vector2 diff = { player2Center.x - blockCenter.x, player2.y + player2.height / 2 - blockCenter.y };

                    float dx = (stoneBlocks[b].size.x / 2 + player2.width / 2) - fabsf(diff.x);
                    float dy = (stoneBlocks[b].size.y / 2 + player2.height / 2) - fabsf(diff.y);

                    if (dx < dy) {
                        if (diff.x > 0) {
                            player2.x += dx;
                        }
                        else {
                            player2.x -= dx;
                        }
                    }
                    else {
                        if (diff.y > 0) {
                            player2.y += dy;
                        }
                        else {
                            player2.y -= dy;
                        }
                    }
                }
            }
        }
        // Обновление позиций игроков
        player.x += velocity.x * deltaTime;
        player.y += velocity.y * deltaTime;
        player2.x += velocity2.x * deltaTime;
        player2.y += velocity2.y * deltaTime;

        // Ограничения границ
        if (player.x < 0) { player.x = 0; velocity.x = -velocity.x; }
        if (player.x + player.width > mapWidth) { player.x = mapWidth - player.width; velocity.x = -velocity.x; }
        if (player.y < 0) { player.y = 0; velocity.y = -velocity.y; }
        if (player.y + player.height > mapHeight) { player.y = mapHeight - player.height; velocity.y = -velocity.y; }

        if (player2.x < 0) { player2.x = 0; velocity2.x = -velocity2.x; }
        if (player2.x + player2.width > mapWidth) { player2.x = mapWidth - player2.width; velocity2.x = -velocity2.x; }
        if (player2.y < 0) { player2.y = 0; velocity2.y = -velocity2.y; }
        if (player2.y + player2.height > mapHeight) { player2.y = mapHeight - player2.height; velocity2.y = -velocity2.y; }

        // --- Обновление врагов --- //
        for (int i = 0; i < NUM_ENEMIES; i++) {
            if (enemies[i].active) {
                enemies[i].rotationAngle += enemies[i].rotationDirection * enemies[i].rotationSpeed * deltaTime;
                if (enemies[i].rotationAngle > 360) enemies[i].rotationAngle -= 360;
                if (enemies[i].rotationAngle < 0) enemies[i].rotationAngle += 360;

                if (rand() % 1000 < 5) {
                    enemies[i].rotationDirection *= -1;
                }

                Vector2 newPos = {
                    enemies[i].position.x + enemies[i].velocity.x * deltaTime,
                    enemies[i].position.y + enemies[i].velocity.y * deltaTime
                };

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
                }
                else {
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

        // --- Столкновение между игроками и блоками уже обработано выше, импульс только для первого --- //

        // --- Обработка пуль --- //
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                bullets[i].position.x += bullets[i].velocity.x * deltaTime;
                bullets[i].position.y += bullets[i].velocity.y * deltaTime;
                if (bullets[i].position.x < 0 || bullets[i].position.x > mapWidth ||
                    bullets[i].position.y < 0 || bullets[i].position.y > mapHeight) {
                    bullets[i].active = false;
                }

                // Столкновения с кирпичами
                for (int b = 0; b < NUM_BRICK_BLOCKS; b++) {
                    if (brickBlocks[b].active) {
                        Rectangle rect = { brickBlocks[b].position.x, brickBlocks[b].position.y, brickBlocks[b].size.x, brickBlocks[b].size.y };
                        if (CheckCollisionPointRec(bullets[i].position, rect)) {
                            bullets[i].active = false;
                            brickBlocks[b].active = false;
                            // Импульс только если пуля от игрока
                            if (bullets[i].fromPlayer) {
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
                                    if (length < minDistance) impulseStrength = 600.0f;
                                    else if (length > maxDistance) impulseStrength = 100.0f;
                                    else {
                                        float t = (length - minDistance) / (maxDistance - minDistance);
                                        impulseStrength = 600.0f * (1 - t) + 100.0f * t;
                                    }
                                    velocity.x += (diff.x / length) * impulseStrength;
                                    velocity.y += (diff.y / length) * impulseStrength;
                                }
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



                for (int i = 0; i < NUM_ENEMIES; i++) {
                    if (enemies[i].active) {
                        Rectangle enemyRect = { enemies[i].position.x, enemies[i].position.y, 50, 50 };

                        // Проверка столкновения первого игрока
                        // В месте, где проверяете столкновение врага с игроком
                        if (CheckCollisionRecs(player, enemyRect)) {
                            printf("Первый игрок столкнулся с врагом! Жизни до: %d\n", playerLives1);
                            playerLives1--;
                            playerHitEnemy1 = true;

                            // Плавное отталкивание для игрока
                            Vector2 diffPlayer = { (player.x + player.width / 2) - (enemies[i].position.x + 25),
                                                     (player.y + player.height / 2) - (enemies[i].position.y + 25) };
                            float distPlayer = sqrtf(diffPlayer.x * diffPlayer.x + diffPlayer.y * diffPlayer.y);
                            if (distPlayer != 0) {
                                float pushDist = 5.0f; // растояние смещения
                                player.x += (diffPlayer.x / distPlayer) * pushDist;
                                player.y += (diffPlayer.y / distPlayer) * pushDist;
                            }

                            // Добавьте смещение врага в противоположную сторону
                            Vector2 diffEnemy = { (enemies[i].position.x + 25) - (player.x + player.width / 2),
                                                    (enemies[i].position.y + 25) - (player.y + player.height / 2) };
                            float distEnemy = sqrtf(diffEnemy.x * diffEnemy.x + diffEnemy.y * diffEnemy.y);
                            if (distEnemy != 0) {
                                float pushDist = 5.0f; // растояние смещения врага
                                enemies[i].position.x += (diffEnemy.x / distEnemy) * pushDist;
                                enemies[i].position.y += (diffEnemy.y / distEnemy) * pushDist;
                            }
                        }

                        // Проверка столкновения второго игрока
                        if (CheckCollisionRecs(player2, enemyRect) && !player2HitEnemy1) {
                            printf("Второй игрок столкнулся с врагом! Жизни до: %d\n", playerLives2);
                            playerLives2--;
                            player2HitEnemy1 = true;

                            // Плавное отталкивание
                            Vector2 diff = { (player2.x + player2.width / 2) - (enemies[i].position.x + 25),
                                             (player2.y + player2.height / 2) - (enemies[i].position.y + 25) };
                            float dist = sqrtf(diff.x * diff.x + diff.y * diff.y);
                            if (dist != 0) {
                                float pushDist = 5.0f;
                                player2.x += (diff.x / dist) * pushDist;
                                player2.y += (diff.y / dist) * pushDist;
                            }
                        }
                    }
                }
                // После этого в начале следующего кадра сбрасывайте флаги
                playerHitEnemy1 = false;
                player2HitEnemy1 = false;

                // столкновение с врагами
                if (bullets[i].fromPlayer) {
                    for (int j = 0; j < NUM_ENEMIES; j++) {
                        if (enemies[j].active) {
                            Rectangle enemyRect = { enemies[j].position.x, enemies[j].position.y, 50, 50 };
                            if (CheckCollisionPointRec(bullets[i].position, enemyRect)) {
                                enemies[j].active = false;
                                bullets[i].active = false;
                                enemiesKilled++;
                                score += 100;
                                break;
                            }
                        }
                    }
                }
                else {
                    // Враговые пули по игрокам
                    Rectangle rectPlayer1 = { player.x, player.y, player.width, player.height };
                    Rectangle rectPlayer2 = { player2.x, player2.y, player2.width, player2.height };
                    if (CheckCollisionPointRec(bullets[i].position, rectPlayer1)) {
                        printf("Пуля врага попала в первого игрока! Координаты пули: (%.2f, %.2f)\n", bullets[i].position.x, bullets[i].position.y);
                        printf("До попадания: Жизни первого игрока: %d\n", playerLives1);
                        bullets[i].active = false;
                        playerLives1--;
                        printf("После попадания: Жизни первого игрока: %d\n", playerLives1);
                    }
                    if (CheckCollisionPointRec(bullets[i].position, rectPlayer2)) {
                        printf("Пуля врага попала во второго игрока! Координаты пули: (%.2f, %.2f)\n", bullets[i].position.x, bullets[i].position.y);
                        printf("До попадания: Жизни второго игрока: %d\n", playerLives2);
                        bullets[i].active = false;
                        playerLives2--;
                        printf("После попадания: Жизни второго игрока: %d\n", playerLives2);
                    }
                }
            }
        }

        if (playerLives1 <= 0 || playerLives2 <= 0) {
            break; // Завершить игру, если жизни закончились
        }

        // --- Отрисовка --- //
        BeginDrawing();
        ClearBackground(RAYWHITE);
        //BeginMode2D(camera);
        // Перед отрисовкой карты
        DrawRectangle(0, 0, screenWidth, screenHeight, BLUE); // полностью синий фон

        DrawTexture(map, 0, 0, BLUE);
        Rectangle sourceRec = { 0, 0, (float)playerTexture.width, (float)playerTexture.height };
        Rectangle destRec = { player.x, player.y, player.width, player.height };
        Vector2 origin = { player.width / 2, player.height / 2 };
        Rectangle destRec2 = { player2.x, player2.y, player2.width, player2.height };

        // Отрисовка первого игрока
        DrawTexturePro(playerTexture, sourceRec, destRec, origin, rotation, WHITE);
        // Отрисовка второго игрока стрелками
        DrawTexturePro(playerTexture, sourceRec, destRec2, origin, rotation2, WHITE);

        // Отрисовка кирпичных блоков с уменьшенными размерами
        // Размеры текстуры кирпича
        float scaleFactor = 0.08f; // например, уменьшить вдвое
        for (int b = 0; b < NUM_BRICK_BLOCKS; b++) {
            if (brickBlocks[b].active) {
                Rectangle sourceRec = { 0, 0, (float)bricks.width, (float)bricks.height };
                Rectangle destRec = {
                    brickBlocks[b].position.x,
                    brickBlocks[b].position.y,
                    bricks.width * scaleFactor,
                    bricks.height * scaleFactor
                };
                Vector2 origin = { (float)(bricks.width * scaleFactor) / 2, (float)(bricks.height * scaleFactor) / 2 };
                DrawTexturePro(bricks, sourceRec, destRec, origin, 0.0f, WHITE);
            }
        }

        float stoneScale = 0.9f; // уменьшить вдвое

        for (int s = 0; s < NUM_STONE_BLOCKS; s++) {
            if (stoneBlocks[s].active) {
                int stoneSize = (int)stoneBlocks[s].size.x;
                Rectangle sourceRec = { 0, 0, (float)stoneTexture.width, (float)stoneTexture.height };
                Rectangle destRec = {
                    stoneBlocks[s].position.x,
                    stoneBlocks[s].position.y,
                    (float)stoneSize * stoneScale,
                    (float)stoneSize * stoneScale
                };
                Vector2 origin = { 0, 0 };
                DrawTexturePro(stoneTexture, sourceRec, destRec, origin, 0.0f, WHITE);
            }
        }

        for (int i = 0; i < NUM_BUSHES; i++) {
            if (bushes[i].active && bushes[i].type == BLOCK_TYPE_BUSH) {
                Rectangle destRec = { bushes[i].position.x, bushes[i].position.y, bushes[i].size.x, bushes[i].size.y };
                Vector2 origin = { 0, 0 };
                float rotation = 0.0f; // при необходимости можете добавить вращение
                Color tintColor = { 255, 255, 255, (unsigned char)(255 * bushes[i].currentTransparency) };
                DrawTexturePro(bushTexture, Rectangle{ 0, 0, (float)bushTexture.width, (float)bushTexture.height }, destRec, origin, rotation, tintColor);
            }
        }



        // Пуль
        float bulletAngle = 0.0f;
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                bulletAngle = atan2f(bullets[i].velocity.y, bullets[i].velocity.x) * RAD2DEG;
                DrawTextureEx(
                    bulletTexture,
                    Vector2{
                        bullets[i].position.x - bulletTexture.width / 10,
                        bullets[i].position.y - bulletTexture.height / 10
                    },
                    bulletAngle,
                    0.2f,
                    WHITE
                );
            }
        }

        for (int i = 0; i < NUM_ENEMIES; i++) {
            if (enemies[i].active) {
                Rectangle rect = { enemies[i].position.x, enemies[i].position.y, 50, 50 };
                Vector2 origin = { 25, 25 };
                DrawRectanglePro(rect, origin, enemies[i].rotationAngle, DARKGREEN);
            }
        }

        //EndMode2D();

        // Отображение жизней каждого игрока
        DrawText(TextFormat("Player 1 Lives: %d", playerLives1), 10, 200, 20, DARKGRAY);
        DrawText(TextFormat("Player 2 Lives: %d", playerLives2), 10, 230, 20, DARKGRAY);

        // Отображение очков
        DrawText(TextFormat("Score: %d", score), 10, 260, 20, DARKGRAY);

        DrawFPS(screenWidth - 90, 10);
        EndDrawing();

    }

    UnloadTexture(bricks);
    UnloadTexture(map);
    UnloadTexture(bushTexture);
    UnloadTexture(stoneTexture);
    UnloadTexture(bulletTexture);
    UnloadTexture(playerTexture);
    CloseWindow();

    // после завершения
    double startTime = GetTime();
    double delaySeconds = 2.0; // задержка 2 секунды
    while (GetTime() - startTime < delaySeconds) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("Game Over", screenWidth / 2 - 80, screenHeight / 2 - 20, 40, RED);
        EndDrawing();
    }
    EndDrawing();

    return 0;
}
