#include "raylib.h"
#include <math.h>

#define MAX_BULLETS 100

typedef struct {
    Vector2 position;
    Vector2 velocity;
    bool active;
} Bullet;

// Вынесем функцию SpawnBullet наружу
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

    InitWindow(screenWidth, screenHeight, "Smooth movement and shooting");
    SetTargetFPS(60);

    Rectangle player = {
        (float)screenWidth / 2 - 25,
        (float)screenHeight / 2 - 25,
        50, 50
    };

    Vector2 velocity = { 0.0f, 0.0f };
    const float maxSpeed = 400.0f;
    const float acceleration = 1500.0f;
    const float friction = 0.89f;

    Bullet bullets[MAX_BULLETS] = { 0 };
    for (int i = 0; i < MAX_BULLETS; i++) {
        bullets[i].active = false;
    }

    bool wasMouseDown = false;

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        // Обработка ввода движения
        Vector2 input = { 0, 0 };

        if (IsKeyDown(KEY_W)) input.y -= 1;
        if (IsKeyDown(KEY_S)) input.y += 1;
        if (IsKeyDown(KEY_A)) input.x -= 1;
        if (IsKeyDown(KEY_D)) input.x += 1;

        // Нормализуем ввод, чтобы движение не было быстрее по диагонали
        float inputLength = sqrtf(input.x * input.x + input.y * input.y);
        if (inputLength > 0) {
            input.x /= inputLength;
            input.y /= inputLength;
        }

        // Ускорение + ограничение скорости
        velocity.x += input.x * acceleration * deltaTime;
        velocity.y += input.y * acceleration * deltaTime;

        float speed = sqrtf(velocity.x * velocity.x + velocity.y * velocity.y);
        if (speed > maxSpeed) {
            velocity.x = (velocity.x / speed) * maxSpeed;
            velocity.y = (velocity.y / speed) * maxSpeed;
        }

        // Применяем трение
        velocity.x *= friction;
        velocity.y *= friction;

        // Остановить при очень маленькой скорости
        if (fabsf(velocity.x) < 0.5f) velocity.x = 0;
        if (fabsf(velocity.y) < 0.5f) velocity.y = 0;

        // Обновление позиции игрока
        player.x += velocity.x * deltaTime;
        player.y += velocity.y * deltaTime;

        // Ограничиваем игрока границами экрана
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

        // Обработка стрельбы по нажатию мыши (только при новом нажатии)
        bool mouseDown = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
        if (mouseDown && !wasMouseDown) {
            Vector2 mousePos = GetMousePosition();
            Vector2 startPos = { player.x + player.width / 2, player.y + player.height / 2 };
            SpawnBullet(bullets, startPos, mousePos, 600.0f);
        }
        wasMouseDown = mouseDown;

        // Обновление пуль
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                bullets[i].position.x += bullets[i].velocity.x * deltaTime;
                bullets[i].position.y += bullets[i].velocity.y * deltaTime;

                // Если пуля вышла за пределы экрана — деактивируем её
                if (bullets[i].position.x < 0 || bullets[i].position.x > screenWidth ||
                    bullets[i].position.y < 0 || bullets[i].position.y > screenHeight) {
                    bullets[i].active = false;
                }
            }
        }

        // Отрисовка
        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawRectangleRec(player, BLUE);
        DrawRectangleLinesEx(player, 2, DARKBLUE);

        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                DrawCircleV(bullets[i].position, 5, RED);
            }
        }

        DrawText("Use WASD to move", 10, 10, 20, DARKGRAY);
        DrawText("Click LEFT MOUSE BUTTON to shoot", 10, 40, 20, DARKGRAY);
        DrawFPS(screenWidth - 90, 10);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}
