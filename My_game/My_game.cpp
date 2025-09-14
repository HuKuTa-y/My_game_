#include "raylib.h"

const int screenWidth = 800;
const int screenHeight = 450;

void MoveRectangle(Rectangle& rec, bool controls) {
    if ((controls && IsKeyDown(KEY_UP)) || (!controls && IsKeyDown(KEY_W))) {
        rec.y += -5;
    }
    if ((controls && IsKeyDown(KEY_LEFT)) || (!controls && IsKeyDown(KEY_A))) {
        rec.x += -5;
    }
    if ((controls && IsKeyDown(KEY_RIGHT)) || (!controls && IsKeyDown(KEY_D))) {
        rec.x += 5;
    }
    if ((controls && IsKeyDown(KEY_DOWN)) || (!controls && IsKeyDown(KEY_S))) {
        rec.y += 5;
    }
    if (rec.x + rec.width > screenWidth) {
        rec.x = screenWidth - rec.width - 2;
    }
    if (rec.x < 0) {
        rec.x = 2;
    }
    if (rec.y + rec.height > screenHeight) {
        rec.y = screenHeight - rec.height - 2;
    }
    if (rec.y < 0) {
        rec.y = 2;
    }
}

int main(void)
{
    InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");
    SetTargetFPS(60);

    
    Image image = LoadImage("cat.png");
    Image image2 = LoadImage("dog.png");
    ImageResize(&image, 132, 114);
    ImageResize(&image2, 132, 114);
    Texture2D texture = LoadTextureFromImage(image);
    Texture2D texture2 = LoadTextureFromImage(image2);
    UnloadImage(image);
    UnloadImage(image2);

   
    Rectangle rec = { screenWidth / 2 - 50, screenHeight / 2 - 50, 100, 100 };
    Rectangle rec2 = { screenWidth / 4 - 50, screenHeight / 4 - 50, 100, 100 };

    
    Color backgroundColor = RAYWHITE;

    
    Rectangle buttonRec = { 700, 50, 80, 45 }; 

    while (!WindowShouldClose())
    {
        
        Vector2 mousePoint = GetMousePosition();

        
        if (CheckCollisionPointRec(mousePoint, buttonRec) && IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        {
            
            if (backgroundColor.r == RAYWHITE.r && backgroundColor.g == RAYWHITE.g && backgroundColor.b == RAYWHITE.b)
            {
                backgroundColor = DARKGRAY; 
            }
            else
            {
                backgroundColor = RAYWHITE; 
            }
        }

        
        MoveRectangle(rec, false);
        MoveRectangle(rec2, true);

        
        BeginDrawing();
        ClearBackground(backgroundColor);

       
        DrawRectangleRec(buttonRec, DARKGRAY);
        DrawRectangleRec(buttonRec, BLACK); 
        DrawText("Night", buttonRec.x + 10, buttonRec.y + 10, 23, WHITE);

        
        DrawRectangleRec(rec, BLUE);
        DrawRectangleRec(rec2, RED);
        DrawText("Bob", rec.x, rec.y, 20, BLACK);
        DrawTexture(texture, rec.x, rec.y, WHITE);
        DrawTexture(texture2, rec2.x, rec2.y, WHITE);
        DrawText("John", rec2.x, rec2.y, 20, BLACK);

        EndDrawing();
    }

    
    UnloadTexture(texture);
    UnloadTexture(texture2);
    CloseWindow();

    return 0;
}
