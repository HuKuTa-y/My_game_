#include "raylib.h"
#include <math.h>

#define MAX_BULLETS 100

typedef struct {
    Vector2 position;
    Vector2 velocity;
    bool active;
} Bullet;

void SpawnBullet(Bullet bullets[], Vector2 startPos, Vector2 targetPos, float bulletSpeed) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            bullets[i].active = true;
            bullets[i].position = startPos;

            Vector2 dir = { targetPos.x - startPos.x, targetPos.y - startPos.y };
            float length = sqrtf(dir.x * dir.x + dir.y * dir.y);
            if (length != 0) {
                bullets[i].velocity.x = (dir.x / length) * bulletSpeed;
                bullets[i].velocity.y = (dir.y / length) * bulletSpeed;
            }
            else {
                bullets[i].velocity = Vector2{ 0, 0 };
            }
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

    Rectangle player = {
        (float)screenWidth / 2 - 25,
        (float)screenHeight / 2 - 25,
        70, 50
    };

    Vector2 velocity = { 0.0f, 0.0f };
    const float maxSpeed = 400.0f;
    const float acceleration = 1500.0f;
    const float friction = 0.89f;

    Bullet bullets[MAX_BULLETS] = { 0 };
    for (int i = 0; i < MAX_BULLETS; i++) {
        bullets[i].active = false;
    }

    float rotation = 0.0f; // угол игрока в градусах
    const float rotationSpeed = 90.0f; // скорость вращения в градусах в секунду

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        // -- Обработка вращения --
        if (IsKeyDown(KEY_A)) {
            rotation -= rotationSpeed * deltaTime; // вращать налево
        }
        if (IsKeyDown(KEY_D)) {
            rotation += rotationSpeed * deltaTime; // вращать направо
        }

        // -- Обработка движения вперёд и назад по rotation --
        float radians = DEG2RAD * rotation;

        if (IsKeyDown(KEY_W)) {
            float dirX = cosf(radians);
            float dirY = sinf(radians);
            // Вперёд
            velocity.x += dirX * acceleration * deltaTime;
            velocity.y += dirY * acceleration * deltaTime;
        }
        if (IsKeyDown(KEY_S)) {
            float dirX = cosf(radians);
            float dirY = sinf(radians);
            // Назад (обратное направление)
            velocity.x -= dirX * acceleration * deltaTime;
            velocity.y -= dirY * acceleration * deltaTime;
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

        // Небольшое залипание скорости в 0 для избежания дрожания
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

        // -- Обработка стрельбы --
        bool mouseDown = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
        static bool lastMouseDown = false;
        if (mouseDown && !lastMouseDown) {
            Vector2 center = { player.x + player.width / 2, player.y + player.height / 2 };
            Vector2 mousePos = GetMousePosition();
            SpawnBullet(bullets, center, mousePos, 600.0f);
        }
        lastMouseDown = mouseDown;

        // Обновляем пули
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

        Vector2 origin = { player.width / 2, player.height / 2 };
        DrawRectanglePro(player, origin, rotation, BLUE);
        DrawRectangleLinesEx(player, 2, DARKBLUE);

        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                DrawCircleV(bullets[i].position, 5, RED);
            }
        }

        DrawText("Use W to move forward", 10, 10, 20, DARKGRAY);
        DrawText("Use S to move backward", 10, 40, 20, DARKGRAY);
        DrawText("Use A/D to rotate", 10, 70, 20, DARKGRAY);
        DrawText("Click LEFT MOUSE BUTTON to shoot", 10, 100, 20, DARKGRAY);
        DrawFPS(screenWidth - 90, 10);

        // Рисуем прицел мышью
        Vector2 mousePos = GetMousePosition();
        int crosshairSize = 10;
        DrawLine(mousePos.x - crosshairSize, mousePos.y, mousePos.x + crosshairSize, mousePos.y, BLACK);
        DrawLine(mousePos.x, mousePos.y - crosshairSize, mousePos.x, mousePos.y + crosshairSize, BLACK);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}
