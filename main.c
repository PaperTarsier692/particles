#include <inttypes.h>
#include <raylib.h>
#include <raymath.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define FPS 60
// Window setup
#define X 1280
#define Y 720

#define PARTICLE_TAIL_LENGTH 5 // 0 <= ... <= 255
#define PARTICLE_RADIUS 5.0f
#define PARTICLE_TAIL_DECREMENT (PARTICLE_RADIUS / PARTICLE_TAIL_LENGTH)
#define PARTICLE_GRAVITY_VECTOR ((Vector2) { 0.0f, 0.5f })
#define PARTICLE_MAX 10000
#define PARTICLE_INITIAL_VELOCITY 10
#define PARTICLE_AIR_RESISTANCE 0.99f
#define PARTICLE_BOUNCE_LOSS 0.8f
#define PARTICLE_GLIDE_RESISTANCE 0.999999f
#define PARTICLE_MIN_VELOCITY 0.5f

#define BLOCK_MAX 1000

typedef struct {
    bool enabled; // Wether or not the particle exists
    Vector2 pos;
    Vector2 vel;
    Color color;
    Vector2 tail[PARTICLE_TAIL_LENGTH];
} Particle;

typedef struct {
    bool enabled;
    Rectangle rect;
} Block;

size_t particleCount;
Particle particles[PARTICLE_MAX] = { 0 };
Block blocks[BLOCK_MAX] = { 0 };

Vector2 getRandomVector2(int x_min, int x_max, int y_min, int y_max)
{
    return (Vector2) { GetRandomValue(x_min, x_max), GetRandomValue(y_min, y_max) };
}

void initParticle(Particle* p, Vector2 pos, Vector2 vel, Color color)
{
    *p = (Particle) { true, pos, vel, color };
    for (uint8_t i = 0; i < PARTICLE_TAIL_LENGTH; i++)
        p->tail[i] = pos;
}

void drawParticle(Particle* p)
{
    // Draw in reverse order
    double tailSize = 0;
    Rectangle rect;
    for (int8_t i = (PARTICLE_TAIL_LENGTH - 1); i > 0; i--) {
        tailSize += PARTICLE_TAIL_DECREMENT;
        DrawCircleV(p->tail[i], tailSize, p->color);
    }
    DrawCircleV(p->pos, PARTICLE_RADIUS, p->color);
}

bool particleCollidesWithBlock(Particle* p, Block* b)
{
    return CheckCollisionPointRec(p->pos, b->rect);
}

bool pointCollidesWithBlock(Vector2* point, Block* b)
{
    return CheckCollisionPointRec(*point, b->rect);
}

void handleCollision(Particle* p)
{
    Block* b;
    p->pos.x += p->vel.x;
    for (size_t i = 0; i < BLOCK_MAX; i++) {
        b = &blocks[i];
        if (!b->enabled)
            continue;
        if (particleCollidesWithBlock(p, b)) {
            p->pos.x -= p->vel.x;
            p->vel.x *= -1;
            p->vel.x *= PARTICLE_BOUNCE_LOSS;
            break;
        }
    }

    p->pos.y += p->vel.y;
    for (size_t i = 0; i < BLOCK_MAX; i++) {
        b = &blocks[i];
        if (!b->enabled)
            continue;
        if (particleCollidesWithBlock(p, b)) {
            p->pos.y -= p->vel.y;
            if (fabsf(p->vel.y) < 0.5)
                p->vel.y *= PARTICLE_GLIDE_RESISTANCE;
            else
                p->vel.y *= PARTICLE_BOUNCE_LOSS;
            p->vel.y *= -1;
            if (fabs(p->vel.x) < PARTICLE_MIN_VELOCITY && fabs(p->vel.y) < PARTICLE_MIN_VELOCITY)
                p->enabled = false;
            break;
        }
    }
}

void tickParticle(Particle* p)
{
    p->tail[0] = p->pos;

    handleCollision(p);

    p->vel = Vector2Add(p->vel, PARTICLE_GRAVITY_VECTOR);
    p->vel.x *= PARTICLE_AIR_RESISTANCE;
    p->vel.y *= PARTICLE_AIR_RESISTANCE;
    for (int8_t i = (PARTICLE_TAIL_LENGTH - 1); i > 0; i--) {
        p->tail[i] = p->tail[i - 1];
    }
}

Particle* getFreeParticle()
{
    for (size_t i = 0; i < PARTICLE_MAX; i++)
        if (!particles[i].enabled)
            return &particles[i];
    return NULL;
}

Block* getFreeBlock()
{
    for (size_t i = 0; i < BLOCK_MAX; i++)
        if (!blocks[i].enabled)
            return &blocks[i];
    return NULL;
}

Color getRandomColor(Color* original, double diversion)
{
    if (original == NULL)
        original = &(Color) { 128, 128, 128 };
    return (Color) {
        Clamp(original->r + GetRandomValue(-diversion, diversion), 0, 255),
        Clamp(original->g + GetRandomValue(-diversion, diversion), 0, 255),
        Clamp(original->b + GetRandomValue(-diversion, diversion), 0, 255),
        255
    };
}

void drawBlock(Block* b)
{
    DrawRectangleRec(b->rect, BLACK);
}

void handleMouseClick()
{
    Vector2 pos = GetMousePosition();
    Block* b;
    for (size_t i = 0; i < BLOCK_MAX; i++) {
        b = &blocks[i];
        if (pointCollidesWithBlock(&pos, b))
            return;
    }
    Particle* p = getFreeParticle();
    if (p == NULL) {
        printf("Out of particles\n");
        return;
    }
    initParticle(p, pos, getRandomVector2(-PARTICLE_INITIAL_VELOCITY, PARTICLE_INITIAL_VELOCITY, -PARTICLE_INITIAL_VELOCITY, PARTICLE_INITIAL_VELOCITY), getRandomColor(NULL, 128));
}

void tick(void)
{
    particleCount = 0;
    Particle* p;
    for (size_t i = 0; i < PARTICLE_MAX; i++) {
        p = &particles[i];
        if (!p->enabled)
            continue;
        tickParticle(p);
        drawParticle(p);
        particleCount++;
    }

    Block* b;
    for (size_t i = 0; i < BLOCK_MAX; i++) {
        b = &blocks[i];
        if (!b->enabled)
            continue;
        drawBlock(b);
    }
}
Rectangle rectangleFromVectors(Vector2 position, Vector2 size)
{
    // ensure the size is always positive
    if (size.x < 0) {
        position.x += size.x;
        size.x = -size.x;
    }

    if (size.y < 0) {
        position.y += size.y;
        size.y = -size.y;
    }

    return (Rectangle) { position.x, position.y, size.x, size.y };
}

int main(void)
{
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(X, Y, "Particles");
    SetTargetFPS(FPS);
    SetRandomSeed(time(NULL));

    Vector2 blockStartPos;
    Vector2 blockCurrentPos;
    bool drawingBlock = false;
    char particleCountBuffer[20];

    while (!WindowShouldClose()) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            if (drawingBlock)
                drawingBlock = false;
            handleMouseClick();
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            blockStartPos = GetMousePosition();
            drawingBlock = true;
        }

        if (drawingBlock) {
            blockCurrentPos = GetMousePosition();
            DrawRectangleRec(rectangleFromVectors(blockStartPos, Vector2Subtract(GetMousePosition(), blockStartPos)), BLACK);
        }

        if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) {
            if (drawingBlock) {
                *getFreeBlock() = (Block) { true, rectangleFromVectors(blockStartPos, Vector2Subtract(blockCurrentPos, blockStartPos)) };
                drawingBlock = false;
            }
        }

        if (IsKeyPressed(KEY_C)) {
            for (size_t i = 0; i < PARTICLE_MAX; i++)
                particles[i].enabled = false;
        }

        BeginDrawing();
        ClearBackground(GRAY);
        tick();
        DrawFPS(10, 10);
        snprintf(particleCountBuffer, sizeof(particleCountBuffer), "%" PRIuMAX, particleCount);
        DrawText(particleCountBuffer, 10, 40, 20, DARKGREEN);
        EndDrawing();
    }
    CloseWindow();
    return EXIT_SUCCESS;
}
