#include "raylib.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
int bulletsFiredInSeries = 0;
float seriesFireTimer = 0.0f;
bool isSeriesRunning = false;
float seriesCooldownTimer = 0.0f;
float reloadDurationActive1 = 0.0f;
float reloadDurationActive2 = 0.0f;



Color LerpColor(Color start, Color end, float t) {
    return {
        (unsigned char)(start.r + (end.r - start.r) * t),
        (unsigned char)(start.g + (end.g - start.g) * t),
        (unsigned char)(start.b + (end.b - start.b) * t),
        (unsigned char)(start.a + (end.a - start.a) * t)
    };
}


bool CheckLineRectIntersection(Vector2 start, Vector2 end, Rectangle rec) {
    // Координаты вершин прямоугольника
    Vector2 rectPoints[4] = {
        { rec.x, rec.y }, // левый верхний
        { rec.x + rec.width, rec.y }, // правый верхний
        { rec.x + rec.width, rec.y + rec.height }, // правый нижний
        { rec.x, rec.y + rec.height } // левый нижний
    };

    // Проверка пересечения линии с каждым краем прямоугольника
    for (int i = 0; i < 4; i++) {
        Vector2 p1 = rectPoints[i];
        Vector2 p2 = rectPoints[(i + 1) % 4];

        // Проверяем пересечение линий (start-end) и (p1-p2)
        float s1_x = start.x;
        float s1_y = start.y;
        float s2_x = end.x;
        float s2_y = end.y;

        float s3_x = p1.x;
        float s3_y = p1.y;
        float s4_x = p2.x;
        float s4_y = p2.y;

        float denom = (s4_y - s3_y) * (s2_x - s1_x) - (s4_x - s3_x) * (s2_y - s1_y);
        if (denom == 0) continue; // линии параллельны

        float s = ((s4_x - s3_x) * (s1_y - s3_y) - (s4_y - s3_y) * (s1_x - s3_x)) / denom;
        float t = ((s2_x - s1_x) * (s1_y - s3_y) - (s2_y - s1_y) * (s1_x - s3_x)) / denom;

        if (s >= 0 && s <= 1 && t >= 0 && t <= 1) {
            return true; // линии пересекаются
        }
    }
    return false; // пересечения нет
}


#define MAX_BULLETS 100
#define NUM_BRICK_BLOCKS 9
#define NUM_STONE_BLOCKS 4
#define NUM_BUSHES 13
#define NUM_ENEMIES 3
#define TRANSITION_DURATION 2.0f
#define ENEMY_SPEED 100.0f
#define ENEMY_VIEW_RADIUS 300.0f
#define ENEMY_FIRE_RADIUS 250.0f
#define ENEMY_PATROL_RADIUS 150.0f
// Объявление глобальных переменных
int currentAmmoType = 1; // 1 - быстрый, мощный; 2 - медленный, слабый
float slowEffectTimer = 0.0f; // таймер замедления
float reloadDurationFast = 2.5f; // стандартный перезаряд
float reloadDurationSlow = 2.5f; // замедленная перезарядка
float bulletAngle = 0.0f;
int enemiesKilled = 0;
int playerHits = 0;
int score = 0; // счетчик очков
float reloadTimer1 = 0.0f; // таймер перезарядки для первого игрока
float reloadTimer2 = 0.0f; // таймер перезарядки для второго игрока
float reloadDuration = 2.5f; // длительность перезарядки
float reloadDurationTripleShot = 8.0f; // перезарядка нового танка
float tripleShotBulletSpeed = 600.0f; // скорость пуль для нового танка
int maxTripleShotBullets = 3; // кол-во пуль в серии
int initialTankType = 1; // по умолчанию
int currentTankType = initialTankType;

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
    int damage;           // урон
    float damageMultiplier; // множитель урона
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

void SpawnBullet(Bullet bullets[], Vector2 startPos, float angleDeg, float bulletSpeed, bool fromPlayer, float damageMultiplier, int damage) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            bullets[i].active = true;
            bullets[i].position = startPos;
            float angleRad = DEG2RAD * angleDeg;
            bullets[i].velocity.x = cosf(angleRad) * bulletSpeed;
            bullets[i].velocity.y = sinf(angleRad) * bulletSpeed;
            bullets[i].fromPlayer = fromPlayer;
            // Можно добавить свойство damage и damageMultiplier, если нужно
            // Например, через расширенную структуру Bullet
            // Но для этого потребуется изменить структуру Bullet
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
    int playerLives1 = 20; // жизни первого игрока
    int playerLives2 = 20; // жизни второго игрока
    int points = 0; // счетчик очков
    srand((unsigned int)time(NULL));
    const int screenWidth = 1400;
    const int screenHeight = 900;
    const int mapWidth = 1400;
    const int mapHeight = 900;
    const int scalle = 0.05f;
    InitWindow(screenWidth, screenHeight, "AI Enemies");
    Texture2D playerTexture = LoadTexture("tanks.png");
    Texture2D bulletTexture = LoadTexture("pull.png");
    Texture2D map = LoadTexture("maps.png");
    Texture2D bushTexture = LoadTexture("kust.png");
    Texture2D stoneTexture = LoadTexture("stone.png");
    Texture2D enemyTexture = LoadTexture("e.png");
    Texture2D bricks = LoadTexture("broke.png");
    Texture2D bulletTextureType2 = LoadTexture("22.png");
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
        { Vector2 { 1250, 270 }, Vector2 { 100, 100 }, true, 1.0f, 1.0f, 0.0f, false, BLOCK_TYPE_BUSH },
        { Vector2 { 1250, 580}, Vector2 { 100, 100 }, true, 1.0f, 1.0f, 0.0f, false, BLOCK_TYPE_BUSH },
    { Vector2 { 700, 950 }, Vector2 { 100, 100 }, true, 1.0f, 1.0f, 0.0f, false, BLOCK_TYPE_BUSH },
    { Vector2 { 1100, 50}, Vector2 { 100, 100 }, true, 1.0f, 1.0f, 0.0f, false, BLOCK_TYPE_BUSH },
    { Vector2 { 230, 290 }, Vector2 { 100, 100 }, true, 1.0f, 1.0f, 0.0f, false, BLOCK_TYPE_BUSH },
    { Vector2 { 900, 900 }, Vector2 { 100, 100 }, true, 1.0f, 1.0f, 0.0f, false, BLOCK_TYPE_BUSH },
    { Vector2 { 720, 100}, Vector2 { 100, 100 }, true, 1.0f, 1.0f, 0.0f, false, BLOCK_TYPE_BUSH },
    { Vector2 { 1100, 800 }, Vector2 { 100, 100 }, true, 1.0f, 1.0f, 0.0f, false, BLOCK_TYPE_BUSH },
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
    // Меню выбора типа танка
    while (true) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("Choose Tank Type:", screenWidth / 2 - 80, screenHeight / 2 - 80, 20, DARKGRAY);
        DrawText("1 - Standard (single shot, 2.5s)", screenWidth / 2 - 150, screenHeight / 2 - 50, 20, DARKGRAY);
        DrawText("2 - Rapid Fire (fast, 2.5s)", screenWidth / 2 - 150, screenHeight / 2 - 20, 20, DARKGRAY);
        DrawText("3 - Triple Shot (3 bullets, 8s)", screenWidth / 2 - 150, screenHeight / 2 + 10, 20, DARKGRAY);
        DrawText("Press 1, 2, or 3 to select", screenWidth / 2 - 130, screenHeight / 2 + 50, 20, DARKGRAY);
        EndDrawing();

        if (IsKeyPressed(KEY_ONE)) {
            currentTankType = 1;
            break;
        }
        if (IsKeyPressed(KEY_TWO)) {
            currentTankType = 2;
            break;
        }
        if (IsKeyPressed(KEY_THREE)) {
            currentTankType = 3;
            break;
        }
    }
    while (!WindowShouldClose()) {
        // В начале цикла, перед обработкой ввода
        bool isBulletActive = false;
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                isBulletActive = true;
                break;
            }
        }

        // Обработка смены типа
        if (!isBulletActive) {
            if (IsKeyPressed(KEY_ONE)) {
                currentAmmoType = 1;
            }
            if (IsKeyPressed(KEY_TWO)) {
                currentAmmoType = 2;
            }
        }
        bool playerHitEnemy1 = false;
        bool player2HitEnemy1 = false;
        float deltaTime = GetFrameTime();
        if (IsKeyDown(KEY_ENTER) && !isSeriesRunning && seriesCooldownTimer <= 0.0f && reloadTimer1 == 0.0f) {
            if (currentTankType == 3) {
                // запуск серии
                bulletsFiredInSeries = 0;
                seriesFireTimer = 0.0f;
                isSeriesRunning = true;
                // Устанавливаем таймер только один раз
                reloadTimer1 = reloadDuration;
                reloadDurationActive1 = reloadTimer1;
            }
            else {
                // одиночный выстрел
                Vector2 startPos = { player.x, player.y };
                float angleDeg = rotation;
                SpawnBullet(bullets, startPos, angleDeg, 600.0f, true, 1.0f, 10);
                reloadTimer1 = (currentAmmoType == 1) ? reloadDurationFast : reloadDurationSlow;
                reloadDurationActive1 = reloadTimer1;
            }
        }
        // В начале цикла
        if (slowEffectTimer > 0.0f) {
            slowEffectTimer -= deltaTime;
            if (slowEffectTimer <= 0.0f) {
                // Восстановить стандартные значения
                reloadDuration = reloadDurationFast; // или первоначальное значение
            }
            else {
                // В процессе замедления, можно дополнительно уменьшать скорость стрельбы или другие параметры
            }
        }
        //camera.target = Vector2{ player.x + player.width / 2, player.y + player.height / 2 };
        //camera.offset = Vector2{ (float)screenWidth / 2, (float)screenHeight / 2 };
        //camera.zoom = 1.0f;
        // Обновление таймеров перезарядки
        if (reloadTimer1 > 0.0f) {
            reloadTimer1 -= deltaTime;
            if (reloadTimer1 < 0.0f) reloadTimer1 = 0.0f;
        }
        if (reloadTimer2 > 0.0f) {
            reloadTimer2 -= deltaTime;
            if (reloadTimer2 < 0.0f) reloadTimer2 = 0.0f;
        }
        float radians = DEG2RAD * rotation;
        // Обработка стрельбы для первого игрока
        // В начале основного цикла (внутри while), для стрельбы
        // Для первого игрока

        float currentReloadDuration = 2.5f; // по умолчанию
        int bulletsPerShot = 1; // по умолчанию

        if (currentTankType == 1) {
            currentReloadDuration = reloadDurationFast; // 2.5 сек
            bulletsPerShot = 1;
        }
        else if (currentTankType == 2) {
            currentReloadDuration = reloadDurationFast; // тоже 2.5 сек
            bulletsPerShot = 1;
        }
        else if (currentTankType == 3) {
            currentReloadDuration = reloadDurationTripleShot; // 8 сек
            bulletsPerShot = maxTripleShotBullets; // 3 пули подряд
        }
        // Для второго игрока
        if (IsKeyDown(KEY_RIGHT_SHIFT) && reloadTimer2 == 0.0f) {
            Vector2 startPos = { player2.x, player2.y }; // верхний левый угол второго игрока
            float angleDeg = rotation2; // тоже вправо
            SpawnBullet(bullets, startPos, angleDeg, 600.0f, true, 1.0f, 10);
            reloadTimer2 = (currentAmmoType == 1) ? reloadDurationFast : reloadDurationSlow;
        }
        // при выстреле (например, при нажатии ENTER для серии)

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



        // Перед обработкой ввода проверяем наличие геймпадов
        int gamepad1 = 0;
        int gamepad2 = 1;
        bool gp1Available = IsGamepadAvailable(gamepad1);
        bool gp2Available = IsGamepadAvailable(gamepad2);


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


        // Обработка серии выстрелов, если выбран тип 3
        // Обработка серии из 7 пуль
        // Обработка серии из 7 пуль

        if (currentTankType == 3 && isSeriesRunning) {
            if (bulletsFiredInSeries < 7) {
                if (seriesFireTimer > 0) {
                    seriesFireTimer -= deltaTime;
                }
                else {
                    // Выстрелить пулю
                    Vector2 startPos = { player.x, player.y };
                    float angleDeg = rotation;
                    SpawnBullet(bullets, startPos, angleDeg, tripleShotBulletSpeed, true, 1.0f, 10);
                    bulletsFiredInSeries++;
                    seriesFireTimer = 0.2f; // задержка между пулями
                }
            }
            else {
                // Серия закончена
                isSeriesRunning = false; // отключаем серию
                seriesCooldownTimer = 8.0f; // запускаем перезарядку
            }
        }
        if (seriesCooldownTimer > 0) {
            seriesCooldownTimer -= deltaTime;
        }

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

                // Проверка столкновений пуль между игроками
       // Пуля от первого игрока по второму
                // Проверка столкновений пуль между игроками
// Пуля от первого игрока по второму
                if (bullets[i].active && bullets[i].fromPlayer) {
                    Rectangle rectPlayer2 = { player2.x, player2.y, player2.width, player2.height };
                    if (CheckCollisionPointRec(bullets[i].position, rectPlayer2)) {
                        bullets[i].active = false;
                        int damage = (int)(bullets[i].damage * bullets[i].damageMultiplier);
                        playerLives2 -= damage;
                        printf("Пуля первого игрока попала во второго! Урон: %d, Жизни второго: %d\n", damage, playerLives2);
                    }
                }
                // Пуля от второго игрока по первому
                if (bullets[i].active && !bullets[i].fromPlayer) {
                    Rectangle rectPlayer1 = { player.x, player.y, player.width, player.height };
                    if (CheckCollisionPointRec(bullets[i].position, rectPlayer1)) {
                        bullets[i].active = false;
                        int damage = (int)(bullets[i].damage * bullets[i].damageMultiplier);
                        playerLives1 -= damage;
                        printf("Пуля второго игрока попала в первого! Урон: %d, Жизни первого: %d\n", damage, playerLives1);
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


        // --- Столкновение между игроками и блоками уже обработано выше, импульс только для первого --- //
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                Vector2 prevPos = bullets[i].position; // запоминаем старую позицию

                // Обновляем позицию
                bullets[i].position.x += bullets[i].velocity.x * deltaTime;
                bullets[i].position.y += bullets[i].velocity.y * deltaTime;

                // Проверяем выход за границы
                if (bullets[i].position.x < 0 || bullets[i].position.x > mapWidth ||
                    bullets[i].position.y < 0 || bullets[i].position.y > mapHeight) {
                    bullets[i].active = false;
                    continue;
                }

                // Проверка попадания по первому игроку
                Rectangle rectPlayer1 = { player.x, player.y, player.width, player.height };
                if (CheckLineRectIntersection(prevPos, bullets[i].position, rectPlayer1)) {
                    if (!bullets[i].fromPlayer) { // не от этого же игрока
                        bullets[i].active = false;
                        int damage = (int)(bullets[i].damage * bullets[i].damageMultiplier);
                        playerLives1 -= damage;
                        printf("Пуля второго игрока попала в первого! Урон: %d, Жизни: %d\n", damage, playerLives1);
                    }
                }

                // Проверка попадания по второму игроку
                Rectangle rectPlayer2 = { player2.x, player2.y, player2.width, player2.height };
                if (CheckLineRectIntersection(prevPos, bullets[i].position, rectPlayer2)) {
                    if (bullets[i].fromPlayer) { // пуля от второго игрока
                        bullets[i].active = false;
                        int damage = (int)(bullets[i].damage * bullets[i].damageMultiplier);
                        playerLives2 -= damage;
                        printf("Пуля второго игрока попала в второго! Урон: %d, Жизни: %d\n", damage, playerLives2);
                    }
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
                        Rectangle destRec = { enemies[i].position.x, enemies[i].position.y, 50, 50 };
                        Vector2 origin = { 25, 25 }; // центр
                        DrawTexturePro(enemyTexture, Rectangle{ 0, 0, (float)enemyTexture.width, (float)enemyTexture.height }, destRec, origin, enemies[i].rotationAngle, WHITE);




                        // Проверка столкновения первого игрока
                        // В месте, где проверяете столкновение врага с игроком
                        Rectangle enemyRect = { enemies[i].position.x, enemies[i].position.y, 50, 50 };
                        if (CheckCollisionRecs(player, enemyRect))
                        {
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
                        if (CheckCollisionRecs(player, enemyRect) && !player2HitEnemy1) {
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
        // Отображение таймеров перезарядки
        // В цикле рендеринга
        // Для первого игрока
        if (reloadTimer1 > 0.0f) {
            reloadTimer1 -= deltaTime;
            if (reloadTimer1 < 0.0f) reloadTimer1 = 0.0f;
        }
        if (reloadDurationActive1 > 0.0f) {
            // прогресс перезарядки
            float progress = (reloadDurationActive1 - reloadTimer1) / reloadDurationActive1;
            // рисуем прогресс
            Color color;
            if (progress < 0.33f) {
                float localT = progress / 0.33f;
                color = LerpColor(RED, ORANGE, localT);
            }
            else if (progress < 0.66f) {
                float localT = (progress - 0.33f) / 0.33f;
                color = LerpColor(ORANGE, YELLOW, localT);
            }
            else {
                float localT = (progress - 0.66f) / 0.34f;
                color = LerpColor(YELLOW, GREEN, localT);
            }
            DrawText(TextFormat("Player 1 Reload: %.1f", reloadTimer1), 10, 290, 20, color);
        }
        else {
            DrawText("Player 1 Ready", 10, 290, 20, GREEN);
        }

        // Для второго игрока
        if (reloadTimer2 > 0.0f) {
            float t = 1.0f - (reloadTimer2 / reloadDuration);
            Color color;
            if (t < 0.33f) {
                // от красного к оранжевому
                float localT = t / 0.33f;
                color = LerpColor(RED, ORANGE, localT);
            }
            else if (t < 0.66f) {
                // от оранжевого к желтому
                float localT = (t - 0.33f) / 0.33f;
                color = LerpColor(ORANGE, YELLOW, localT);
            }
            else {
                // от желтого к зеленому
                float localT = (t - 0.66f) / 0.34f;
                color = LerpColor(YELLOW, GREEN, localT);
            }
            DrawText(TextFormat("Player 2 Reload: %.1f", reloadTimer2), 10, 320, 20, color);
        }
        else {
            DrawText("Player 2 Ready", 10, 320, 20, GREEN);
        }
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
                    stoneBlocks[s].position.x - 25,
                    stoneBlocks[s].position.y - 25,
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



        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                float bulletAngle = atan2f(bullets[i].velocity.y, bullets[i].velocity.x) * RAD2DEG;
                float scale = (currentAmmoType == 2) ? 0.025f : 0.2f; // масштаб для пуль второго типа
                Texture2D textureToDraw = (currentAmmoType == 2) ? bulletTextureType2 : bulletTexture; // выбираем текстуру

                DrawTextureEx(
                    textureToDraw,
                    bullets[i].position,
                    bulletAngle,
                    scale,
                    WHITE
                );
            }
        }


        //EndMode2D();
        // Отрисовка границ первого игрока
        DrawRectangleLines(
            (int)player.x,
            (int)player.y,
            (int)player.width,
            (int)player.height,
            RED
        );
        // Отрисовка границ пуль
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                // Размер границы
                float borderSize = 10.0f; // Можно изменить размер по желанию
                Rectangle borderRect = {
                    bullets[i].position.x - borderSize / 2,
                    bullets[i].position.y - borderSize / 2,
                    borderSize,
                    borderSize
                };
                DrawRectangleLines(
                    (int)borderRect.x,
                    (int)borderRect.y,
                    (int)borderRect.width,
                    (int)borderRect.height,
                    GREEN // Цвет границы
                );
            }
        }
        // Отрисовка границ второго игрока
        DrawRectangleLines(
            (int)player2.x,
            (int)player2.y,
            (int)player2.width,
            (int)player2.height,
            RED
        );
        // Отображение жизней каждого игрока
        DrawText(TextFormat("Player 1 Lives: %d", playerLives1), 10, 200, 20, WHITE);
        DrawText(TextFormat("Player 2 Lives: %d", playerLives2), 10, 230, 20, WHITE);

        // Отображение очков
        DrawText(TextFormat("Score: %d", score), 10, 260, 20, DARKGRAY);

        DrawFPS(screenWidth - 90, 10);
        DrawText(TextFormat("Patron type: %d", currentAmmoType), 10, screenHeight - 30, 20, DARKGRAY);
        EndDrawing();

    }

    UnloadTexture(bricks);
    UnloadTexture(map);
    UnloadTexture(bushTexture);
    UnloadTexture(stoneTexture);
    UnloadTexture(bulletTexture);
    UnloadTexture(enemyTexture);
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
    // В конце цикла перед EndDrawing()

    EndDrawing();

    return 0;
} 
