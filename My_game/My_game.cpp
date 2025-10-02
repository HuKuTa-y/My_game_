#include "raylib.h"
#include <math.h>

#define MAX_BULLETS 100

typedef struct {
    Vector2 position;
    Vector2 velocity;
    bool active;
} Bullet;

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

    float rotation = 0.0f; // угол фигуры в градусах (по умолчанию смотрит вверх)
    const float rotationSpeed = 90.0f; // скорость вращения в градусах в секунду

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        // Обновляем угол
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

        // Минимальное значение скорости для избежания дрожания
        if (fabsf(velocity.x) < 0.5f) velocity.x = 0;
        if (fabsf(velocity.y) < 0.5f) velocity.y = 0;

        // Обновляем позицию
        player.x += velocity.x * deltaTime;
        player.y += velocity.y * deltaTime;

        // Ограничение границ экрана
        if (player.x < 0) {
            player.x = 0;
            velocity.x = 0;
        }
        if (player.y < 0) {
            player.y = 0;
            velocity.y = 0;
        }
        if (player.x + player.width > screenWidth) {
            player.x = screenWidth - player.width;
            velocity.x = 0;
        }
        if (player.y + player.height > screenHeight) {
            player.y = screenHeight - player.height;
            velocity.y = 0;
        }

        // Стрельба
        if (IsKeyPressed(KEY_ENTER)) {
            Vector2 center = { player.x + player.width / 2, player.y + player.height / 2};
            SpawnBullet(bullets, center, rotation, 600.0f);
        }

        // Обновление пуль
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                bullets[i].position.x += bullets[i].velocity.x * deltaTime;
                bullets[i].position.y += bullets[i].velocity.y * deltaTime;

                if (bullets[i].position.x < 0 || bullets[i].position.x > screenWidth ||
                    bullets[i].position.y < 0 || bullets[i].position.y > screenHeight) {
                    bullets[i].active = false;
                }
            }
        }
        
        // --- Отрисовка ---
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Точка вращения — центр фигуры
        Vector2 origin = { player.width / 2, player.height / 2};
  
        // Рисуем фигуру с учетом rotation и origin
        DrawRectanglePro(player, origin, rotation, BLUE);
        DrawRectangleLinesEx(player, 2, GRAY);

       

        // Рисуем пули
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                DrawCircleV(bullets[i].position, 5, RED);
            }
        }

        // Инструкции
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
