#include <inttypes.h>
#include <raylib.h>
#include <raymath.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

#define FPS 40
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
#define BLOCK_TYPE_COUNT 6
#define SLIDER_RESISTANCE 1.1f
#define BOUNCER_RESISTANCE 1.5f
#define SPAWNER_FREQUENCY 10

typedef struct {
    bool enabled; // Wether or not the particle exists
    Vector2 pos;
    Vector2 vel;
    Color color;
    Vector2 tail[PARTICLE_TAIL_LENGTH];
} Particle;

typedef struct {
    uint8_t type; // 0 = disabled; 1 = spawner; 2 = block; 3 = bouncer; 4 = slider; 5 = despawner
    Rectangle rect;
} Block;

const char* BLOCK_NAMES[BLOCK_TYPE_COUNT] = {
    "Disabled",
    "Spawner",
    "Block",
    "Bouncer",
    "Slider",
    "Despawner"
};

const Color BLOCK_COLORS[BLOCK_TYPE_COUNT] = { BLACK, BLUE, BLACK, YELLOW, GREEN, RED };

size_t spawnerTimer = 0;
size_t particleCount;
Particle particles[PARTICLE_MAX] = { 0 };
Block blocks[BLOCK_MAX] = { 0 };

Vector2 blockStartPos;
Vector2 blockCurrentPos;
bool drawingBlock = false;
char particleCountBuffer[20];
uint8_t blockType = 1;

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

bool outOfBounds(Vector2 pos)
{
    return pos.x < 0 || pos.x > X || pos.y < 0 || pos.y > Y;
}

void handleCollision(Particle* p)
{
    Block* b;
    p->pos.x += p->vel.x;
    for (size_t i = 0; i < BLOCK_MAX; i++) {
        b = &blocks[i];
        if (b->type < 2)
            continue;
        if (particleCollidesWithBlock(p, b)) {
            switch (b->type) {
            case 2 ... 4:
                p->pos.x -= p->vel.x;
                p->vel.x *= -1;
                if (b->type == 3)
                    p->vel.x *= BOUNCER_RESISTANCE;
                else
                    p->vel.x *= PARTICLE_BOUNCE_LOSS;
                break;
            case 5:
                p->enabled = false;
                return;
            }
        }
    }

    p->pos.y += p->vel.y;
    for (size_t i = 0; i < BLOCK_MAX; i++) {
        b = &blocks[i];
        if (b->type < 2)
            continue;
        if (particleCollidesWithBlock(p, b)) {
            switch (b->type) {
            case 2 ... 4:
                p->pos.y -= p->vel.y;
                if (fabsf(p->vel.y) < 4) {
                    if (b->type == 4)
                        p->vel.x *= SLIDER_RESISTANCE;
                    else
                        p->vel.x *= PARTICLE_GLIDE_RESISTANCE;
                } else {
                    if (b->type == 3)
                        p->vel.y *= BOUNCER_RESISTANCE;
                    else
                        p->vel.y *= PARTICLE_BOUNCE_LOSS;
                }
                p->vel.y *= -1;
                if (fabs(p->vel.x) < PARTICLE_MIN_VELOCITY && fabs(p->vel.y) < PARTICLE_MIN_VELOCITY)
                    p->enabled = false;
                break;
            case 5:
                p->enabled = false;
                return;
            }
            break;
        }
    }
}

void tickParticle(Particle* p)
{
    if (outOfBounds(p->pos)) {
        p->enabled = false;
        return;
    }
    p->tail[0] = p->pos;

    handleCollision(p);

    p->vel = Vector2Add(p->vel, PARTICLE_GRAVITY_VECTOR);
    p->vel.x *= PARTICLE_AIR_RESISTANCE;
    p->vel.y *= PARTICLE_AIR_RESISTANCE;
    for (int8_t i = (PARTICLE_TAIL_LENGTH - 1); i > 0; i--) {
        p->tail[i] = p->tail[i - 1];
    }
}

Particle* getFreeParticle(void)
{
    for (size_t i = 0; i < PARTICLE_MAX; i++)
        if (!particles[i].enabled)
            return &particles[i];
    return NULL;
}

Block* getFreeBlock(void)
{
    for (size_t i = 0; i < BLOCK_MAX; i++)
        if (blocks[i].type == 0)
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

Vector2 getRandomVelocity(void)
{
    return getRandomVector2(-PARTICLE_INITIAL_VELOCITY, PARTICLE_INITIAL_VELOCITY, -PARTICLE_INITIAL_VELOCITY, PARTICLE_INITIAL_VELOCITY);
}

void drawBlock(Block* b)
{
    DrawRectangleRec(b->rect, BLOCK_COLORS[b->type]);
}

void handleMouseClick(void)
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
    initParticle(p, pos, getRandomVelocity(), getRandomColor(NULL, 128));
}

Vector2 getRectMiddle(Rectangle rect)
{
    return (Vector2) { rect.x + rect.width / 2, rect.y + rect.height / 2 };
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
        if (b->type == 0)
            continue;
        drawBlock(b);
        if (b->type == 1)
            if (spawnerTimer % SPAWNER_FREQUENCY == 0)
                initParticle(getFreeParticle(), getRectMiddle(b->rect), getRandomVelocity(), getRandomColor(NULL, 128));
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

void UpdateDrawFrame(void)
{
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        if (drawingBlock)
            // Cancel drawing the block
            drawingBlock = false;
        handleMouseClick();
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        blockStartPos = GetMousePosition();
        drawingBlock = true;
    }

    if (drawingBlock) {
        blockCurrentPos = GetMousePosition();
        // Display the to be placed block
        DrawRectangleRec(rectangleFromVectors(blockStartPos, Vector2Subtract(GetMousePosition(), blockStartPos)), BLOCK_COLORS[blockType]);
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) {
        if (drawingBlock) {
            *getFreeBlock() = (Block) { blockType, rectangleFromVectors(blockStartPos, Vector2Subtract(blockCurrentPos, blockStartPos)) };
            drawingBlock = false;
        }
    }

    if (IsKeyPressed(KEY_C)) {
        // Clear all particles
        for (size_t i = 0; i < PARTICLE_MAX; i++)
            particles[i].enabled = false;
    }

    if (IsKeyPressed(KEY_RIGHT)) {
        blockType++;
        if (blockType >= BLOCK_TYPE_COUNT)
            blockType = 1;
    }
    if (IsKeyPressed(KEY_LEFT)) {
        if (blockType <= 1)
            blockType = BLOCK_TYPE_COUNT - 1;
        else
            blockType--;
    }

    BeginDrawing();
    ClearBackground(GRAY);
    tick();
    DrawFPS(10, 10);
    snprintf(particleCountBuffer, sizeof(particleCountBuffer), "%zu", particleCount);
    DrawText(particleCountBuffer, 10, 40, 20, DARKGREEN);
    if (drawingBlock)
        DrawText(BLOCK_NAMES[blockType], 10, 80, 20, DARKGREEN);
    EndDrawing();
    spawnerTimer++;
}

int main(void)
{
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(X, Y, "Particles");
    SetRandomSeed(time(NULL));

    // Set all particles and blocks to disabled
    for (size_t i = 0; i < PARTICLE_MAX; i++)
        particles[i] = (Particle) { false };
    for (size_t i = 0; i < BLOCK_MAX; i++)
        blocks[i] = (Block) { 0 };

#if defined(PLATFORM_WEB)
    // Let the browser handle updates
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    SetTargetFPS(FPS);
    while (!WindowShouldClose()) {
        UpdateDrawFrame();
    }
#endif

    CloseWindow();
    return EXIT_SUCCESS;
}
