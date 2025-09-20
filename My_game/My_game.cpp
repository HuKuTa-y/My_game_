
/*#include "raylib.h"

const int screenWidth = 1000;
const int screenHeight = 600;

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
    Image image3 = LoadImage("brick.png");
    Image image4 = LoadImage("grass.png");
    ImageResize(&image, 132, 114);
    ImageResize(&image3, 60, 55);
    Texture2D texture = LoadTextureFromImage(image);
    Texture2D bricktexture = LoadTextureFromImage(image3);
    Texture2D gras = LoadTextureFromImage(image4);
    UnloadImage(image);
  


    Rectangle rec = { screenWidth / 2 - 50, screenHeight / 2 - 50, 100, 100 };
    


    Color backgroundColor = RAYWHITE;


    Rectangle buttonRec = { 700, 50, 80, 45 };

    while (!WindowShouldClose())
    {

        Vector2 mousePoint = GetMousePosition();

        for (int y = 0; y < screenHeight; y += gras.height) {
            for (int x = 0; x < screenWidth; x += gras.width) {
                DrawTexture(gras, x, y, WHITE);
            }
        }

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
      


        BeginDrawing();
        ClearBackground(backgroundColor);


        DrawRectangleRec(buttonRec, DARKGRAY);
        DrawRectangleRec(buttonRec, BLACK);
        DrawText("Night", buttonRec.x + 10, buttonRec.y + 10, 23, WHITE);

        DrawTexture(bricktexture, 10, 10, WHITE);
        DrawRectangleRec(rec, BLUE);
 
        DrawText("Bob", rec.x, rec.y, 20, BLACK);
        DrawTexture(texture, rec.x, rec.y, WHITE);
   

        EndDrawing();
    }


    UnloadTexture(texture);
    UnloadTexture(bricktexture);
    UnloadTexture(gras);
    CloseWindow();

    return 0;
}

#include <iostream>
#include "raylib.h"
#include "raymath.h"
#include <vector>

const int MAX_WIDTH = 800;
const int MAX_HEIGHT = 600;
const float damping = -0.05f;

struct Circle {
    Vector2 position;
    Vector2 velocity;
    Vector2 accelerate;
    float radius;
    float weight;
};

int GenerateCircles(std::vector<Circle>& circles, int count = 10, int minRadius = 30, int maxRadius = 60,
    int minVelocity = 0, int maxVelocity = 0) {
    for (int i = 0; i < count; i++)
    {
        Vector2 velocity = {
           GetRandomValue(minVelocity,maxVelocity),
           GetRandomValue(minVelocity,maxVelocity)
        };
        Vector2 accelerate = { 0,0 };
        float radius = GetRandomValue(minRadius, maxRadius);
        float weight = radius / 3;
        Vector2 position = {
            GetRandomValue(0 + radius,MAX_WIDTH - radius),
            GetRandomValue(0 + radius,MAX_HEIGHT - radius)
        };
        circles.push_back(Circle{ position,velocity,accelerate,radius,weight });
    }
    return count;
}

void StaticCollisionResolution(Circle& a, Circle& b) {
    Vector2 delta = {
        b.position.x - a.position.x,
        b.position.y - a.position.y
    };
    float distance = Vector2Length(delta);
    float overlap = distance - a.radius - b.radius;
    Vector2 direction = Vector2Scale(Vector2Normalize(delta), overlap / 2.0);
    a.position = Vector2Add(a.position, direction);
    b.position = Vector2Add(b.position, Vector2Negate(direction));
}

void DynamicCollisionResolution(Circle& a, Circle& b) {
    Vector2 first = a.position;
    Vector2 second = b.position;
    //îñè ñòîëêíîâåíèÿ - íîðìàëü è êàñàòåëüíàÿ
    Vector2 dir = Vector2Subtract(second, first);
    Vector2 normal = Vector2Normalize(dir);
    Vector2 tangent = { -normal.y,normal.x };
    //Ïðîåêöèè íà îñè ñòîëêíîâåíèÿ
    float dpNormA = Vector2DotProduct(a.velocity, normal);
    float dpNormB = Vector2DotProduct(b.velocity, normal);
    float dpTangA = Vector2DotProduct(a.velocity, tangent);
    float dpTangB = Vector2DotProduct(b.velocity, tangent);
    //Ñîõðàíåíèå èìïóëüñà â 1ìåðíîì ïðîñòðàíñòâå
    float p1 = (dpNormA * (a.weight - b.weight) + 2 * b.weight * dpNormB) / (a.weight + b.weight);
    float p2 = (dpNormB * (b.weight - a.weight) + 2 * a.weight * dpNormA) / (a.weight + b.weight);
    //Ïðèìåíÿåì èçìåíåííûé èìïóëüñ ê ñêîðîñòÿì êðóãîâ
    a.velocity = Vector2Add(Vector2Scale(tangent, dpTangA), Vector2Scale(normal, p1));
    b.velocity = Vector2Add(Vector2Scale(tangent, dpTangB), Vector2Scale(normal, p2));
}

void HandleCollision(std::vector<std::pair<Circle*, Circle*>>& collisions, Circle& a, Circle& b) {
    Vector2 delta = { b.position.x - a.position.x,b.position.y - a.position.y, };
    float distance = Vector2Length(delta);
    float minDistance = a.radius + b.radius;
    if (distance < minDistance) {
        StaticCollisionResolution(a, b);
        Circle* pa = &a;
        Circle* pb = &b;
        collisions.push_back({ pa,pb });
    }
}

int main()
{
    InitWindow(MAX_WIDTH, MAX_HEIGHT, "Bounce");
    SetTargetFPS(60);

    bool dragging = false;
    Circle* selectedCircle = nullptr;

    std::vector<Circle> circles;
    GenerateCircles(circles, 5, 10, 20, 0, 0);

    while (!WindowShouldClose()) {
        Vector2 mousePosition = GetMousePosition();

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            for (auto& circle : circles) {
                if (CheckCollisionPointCircle(mousePosition, circle.position, circle.radius)) {
                    dragging = true;
                    selectedCircle = &circle;
                    break;
                }
            }
        }

        //if(selectedCircle!=nullptr)
        //    selectedCircle->position = mousePosition;

        std::vector<std::pair<Circle*, Circle*>> collisions;

        for (int i = 0; i < circles.size() - 1; ++i) {
            for (int j = i + 1; j < circles.size(); ++j) {
                HandleCollision(collisions, circles[i], circles[j]);
            }
        }

        for (auto& collision : collisions) {
            DynamicCollisionResolution(*collision.first, *collision.second);
        }

        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            if (selectedCircle != nullptr) {
                Vector2 direction = Vector2Subtract(mousePosition, selectedCircle->position);
                float distance = Vector2Length(direction);
                Vector2 acceleration = Vector2Negate(Vector2Scale(Vector2Normalize(direction), distance * 0.1f));
                selectedCircle->velocity = acceleration;
            }
            dragging = false;
            selectedCircle = nullptr;
        }

        for (auto& circle : circles) {
            circle.accelerate = Vector2Scale(circle.velocity, damping);
            circle.velocity = Vector2Add(circle.velocity, circle.accelerate);
            circle.position = Vector2Add(circle.position, circle.velocity);
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);
        //for circle in circles
        for (const auto& circle : circles) {
            DrawCircleV(circle.position, circle.radius, BLUE);
        }
        EndDrawing();
    }
}*/
