#include "raylib.h"
#include <math.h>

int main() {
    const int screenWidth = 800;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "Slow action rectangle - WASD");
    SetTargetFPS(60);

    Rectangle player = {
        (float)screenWidth / 2 - 25,
        (float)screenHeight / 2 - 25,
        50, 50
    };

    // Переменные для плавного движения
    Vector2 velocity = { 0, 0 };
    float speed = 400.0f;
    float acceleration = 1500.0f;
    float friction = 0.89f;

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        // Обработка ввода с плавным ускорением
        Vector2 input = { 0, 0 };

        if (IsKeyDown(KEY_W)) input.y -= 1;
        if (IsKeyDown(KEY_S)) input.y += 1;
        if (IsKeyDown(KEY_A)) input.x -= 1;
        if (IsKeyDown(KEY_D)) input.x += 1;

        // Нормализуем вектор ввода для диагонального движения
        float inputLength = sqrt(input.x * input.x + input.y * input.y);
        if (inputLength > 0) {
            input.x /= inputLength;
            input.y /= inputLength;
        }

        // Применяем ускорение
        velocity.x += input.x * acceleration * deltaTime;
        velocity.y += input.y * acceleration * deltaTime;

        // Ограничиваем максимальную скорость
        float currentSpeed = sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
        if (currentSpeed > speed) {
            velocity.x = (velocity.x / currentSpeed) * speed;
            velocity.y = (velocity.y / currentSpeed) * speed;
        }

        // Применяем трение (плавное замедление)
        velocity.x *= friction;
        velocity.y *= friction;

        // Если скорость очень мала - останавливаем полностью
        if (fabs(velocity.x) < 0.5f) velocity.x = 0;
        if (fabs(velocity.y) < 0.5f) velocity.y = 0;

        // Обновляем позицию
        player.x += velocity.x * deltaTime;
        player.y += velocity.y * deltaTime;

        // Ограничение движения в пределах экрана
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

        // Отрисовка
        BeginDrawing();
        ClearBackground(GRAY);

        // Рисуем прямоугольник
        DrawRectangleRec(player, BLUE);
        DrawRectangleLinesEx(player, 2, DARKBLUE);

        // Отладочная информация
        DrawText("Use WASD for action", 10, 10, 20, LIGHTGRAY);
        DrawText(TextFormat("Position: (%.0f, %.0f)", player.x, player.y), 10, 40, 20, GREEN);
        DrawText(TextFormat("Speed: (%.1f, %.1f)", velocity.x, velocity.y), 10, 70, 20, YELLOW);
        DrawText("Slow action with inert", 10, 100, 20, ORANGE);
        DrawFPS(10, 130);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
