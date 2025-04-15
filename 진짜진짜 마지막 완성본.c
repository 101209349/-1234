#include "raylib.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

#define MAX_BLOCKS 150
#define MAX_ITEMS 20
#define SCREEN_WIDTH 1600
#define SCREEN_HEIGHT 900

#define CLAMP(value, min, max) ((value < min) ? min : ((value > max) ? max : value))
#define FMAX(a, b) ((a) > (b) ? (a) : (b))

typedef enum { CONTROL_KEYBOARD, CONTROL_MOUSE } ControlMode;
typedef enum { EASY, NORMAL, HARD } Difficulty;
typedef enum { STATE_START, STATE_PLAYING, STATE_GAMEOVER, STATE_ENDING } GameState;
typedef enum { ITEM_NONE, ITEM_SHRINK_PADDLE, ITEM_SPEED_UP } ItemType;

typedef struct {
    Vector2 position;
    ItemType type;
    int active;
} Item;

// 사운드 전역 변수
Sound blockBreakSound;
Sound gameOverSound;
Sound gameClearSound;
Music bgm;

int LoadHighScore(const char* filename) {
    FILE* file = NULL;
    errno_t err = fopen_s(&file, filename, "r");
    int score = 0;
    if (err == 0 && file != NULL) {
        fscanf_s(file, "%d", &score);
        fclose(file);
    }
    return score;
}

void SaveHighScore(const char* filename, int score) {
    FILE* file = NULL;
    errno_t err = fopen_s(&file, filename, "w");
    if (err == 0 && file != NULL) {
        fprintf(file, "%d", score);
        fclose(file);
    }
}

void ResetBlocks(Rectangle blocks[MAX_BLOCKS], int active[MAX_BLOCKS], int health[MAX_BLOCKS], Difficulty difficulty) {
    for (int i = 0; i < MAX_BLOCKS; i++) {
        blocks[i].x = (float)(15 + (i % 25) * 63);
        blocks[i].y = (float)(50 + (i / 25) * 30);
        blocks[i].width = 60.0f;
        blocks[i].height = 20.0f;
        active[i] = 1;

        if (difficulty == EASY) health[i] = 1;
        else if (difficulty == NORMAL) health[i] = 3;
        else if (difficulty == HARD) health[i] = 5;
    }
}

void SpawnItem(Item items[MAX_ITEMS], Vector2 pos) {
    for (int i = 0; i < MAX_ITEMS; i++) {
        if (!items[i].active) {
            items[i].active = 1;
            items[i].position = pos;
            items[i].type = (GetRandomValue(1, 100) <= 50) ? ITEM_SHRINK_PADDLE : ITEM_SPEED_UP;
            break;
        }
    }
}

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Breakout Game with Sound");
    SetTargetFPS(60);

    InitAudioDevice();

    blockBreakSound = LoadSound("block_break.wav");
    gameOverSound = LoadSound("game_over.wav");
    gameClearSound = LoadSound("game_clear.wav");
    bgm = LoadMusicStream("background_music.wav");
    PlayMusicStream(bgm);

    Rectangle paddle = { SCREEN_WIDTH / 2 - 100, 800, 150, 20 };
    Vector2 ball = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };
    Vector2 speed = { 5.0f, -5.0f };
    float radius = 10.0f;

    Rectangle blocks[MAX_BLOCKS];
    int blockHealth[MAX_BLOCKS];
    int active[MAX_BLOCKS] = { 0 };
    Item items[MAX_ITEMS];
    memset(items, 0, sizeof(items));
    Color healthColors[6] = { GRAY, GREEN, BLUE, ORANGE, RED, MAROON };

    int score = 0, lifes = 5;
    int highScore = LoadHighScore("highscore.txt");

    Difficulty difficulty = NORMAL;
    ControlMode controlMode = CONTROL_KEYBOARD;
    GameState gamestate = STATE_START;

    while (!WindowShouldClose()) {
        UpdateMusicStream(bgm);

        if (gamestate == STATE_START) {
            BeginDrawing();
            ClearBackground(BLACK);

            DrawText("Select Difficulty", SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 - 150, 40, WHITE);
            DrawText(TextFormat("High Score: %d", highScore), SCREEN_WIDTH / 2 - 100, 50, 20, GOLD);

            Rectangle easyBtn = { SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 - 80, 300, 60 };
            Rectangle normalBtn = { SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 + 0, 300, 60 };
            Rectangle hardBtn = { SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 + 80, 300, 60 };

            Vector2 mouse = GetMousePosition();

            DrawRectangleRec(easyBtn, CheckCollisionPointRec(mouse, easyBtn) ? LIGHTGRAY : DARKGRAY);
            DrawText("EASY", easyBtn.x + 100, easyBtn.y + 15, 30, WHITE);

            DrawRectangleRec(normalBtn, CheckCollisionPointRec(mouse, normalBtn) ? LIGHTGRAY : DARKGRAY);
            DrawText("NORMAL", normalBtn.x + 85, normalBtn.y + 15, 30, WHITE);

            DrawRectangleRec(hardBtn, CheckCollisionPointRec(mouse, hardBtn) ? LIGHTGRAY : DARKGRAY);
            DrawText("HARD", hardBtn.x + 100, hardBtn.y + 15, 30, WHITE);

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (CheckCollisionPointRec(mouse, easyBtn)) {
                    difficulty = EASY;
                    gamestate = STATE_PLAYING;
                    score = 0;
                    lifes = 5;
                    memset(items, 0, sizeof(items));
                    ResetBlocks(blocks, active, blockHealth, difficulty);
                }
                if (CheckCollisionPointRec(mouse, normalBtn)) {
                    difficulty = NORMAL;
                    gamestate = STATE_PLAYING;
                    score = 0;
                    lifes = 5;
                    memset(items, 0, sizeof(items));
                    ResetBlocks(blocks, active, blockHealth, difficulty);
                }
                if (CheckCollisionPointRec(mouse, hardBtn)) {
                    difficulty = HARD;
                    gamestate = STATE_PLAYING;
                    score = 0;
                    lifes = 5;
                    memset(items, 0, sizeof(items));
                    ResetBlocks(blocks, active, blockHealth, difficulty);
                }
            }

            if (IsKeyPressed(KEY_ONE)) {
                difficulty = EASY;
                gamestate = STATE_PLAYING;
                score = 0;
                lifes = 5;
                memset(items, 0, sizeof(items));
                ResetBlocks(blocks, active, blockHealth, difficulty);
            }
            if (IsKeyPressed(KEY_TWO)) {
                difficulty = NORMAL;
                gamestate = STATE_PLAYING;
                score = 0;
                lifes = 5;
                memset(items, 0, sizeof(items));
                ResetBlocks(blocks, active, blockHealth, difficulty);
            }
            if (IsKeyPressed(KEY_THREE)) {
                difficulty = HARD;
                gamestate = STATE_PLAYING;
                score = 0;
                lifes = 5;
                memset(items, 0, sizeof(items));
                ResetBlocks(blocks, active, blockHealth, difficulty);
            }

            EndDrawing();
            continue;
        }

        if (gamestate == STATE_PLAYING) {
            if (controlMode == CONTROL_MOUSE)
                paddle.x = (float)(GetMouseX() - paddle.width / 2);
            else {
                if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) paddle.x -= 8.0f;
                if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) paddle.x += 8.0f;
            }

            paddle.x = CLAMP(paddle.x, 0, SCREEN_WIDTH - paddle.width);
            ball.x += speed.x;
            ball.y += speed.y;

            if (ball.x < radius || ball.x > SCREEN_WIDTH - radius) speed.x *= -1.0f;
            if (ball.y < radius) speed.y *= -1.0f;
            if (ball.y > SCREEN_HEIGHT) {
                lifes--;
                ball = Vector2{ (float)SCREEN_WIDTH / 2, (float)SCREEN_HEIGHT / 2 };
                speed = Vector2{ 5.0f, -5.0f };
                if (lifes <= 0) {
                    gamestate = STATE_GAMEOVER;
                    PlaySound(gameOverSound);
                }
            }

            if (CheckCollisionCircleRec(ball, radius, paddle)) {
                speed.y *= -1.0f;
                ball.y = paddle.y - radius;
            }

            for (int i = 0; i < MAX_BLOCKS; i++) {
                if (active[i] && CheckCollisionCircleRec(ball, radius, blocks[i])) {
                    blockHealth[i]--;
                    if (blockHealth[i] <= 0) {
                        active[i] = 0;
                        score += 10;
                        PlaySound(blockBreakSound);
                        if (GetRandomValue(1, 100) <= 20)
                            SpawnItem(items, Vector2{ blocks[i].x + 30.0f, blocks[i].y + 20.0f });
                    }
                    speed.y *= -1.0f;
                    break;
                }
            }

            for (int i = 0; i < MAX_ITEMS; i++) {
                if (items[i].active) {
                    items[i].position.y += 3.0f;
                    Rectangle itemBox = { items[i].position.x, items[i].position.y, 20.0f, 20.0f };
                    DrawRectangleRec(itemBox, items[i].type == ITEM_SHRINK_PADDLE ? PURPLE : YELLOW);

                    if (CheckCollisionRecs(itemBox, paddle)) {
                        if (items[i].type == ITEM_SHRINK_PADDLE)
                            paddle.width = FMAX(paddle.width - 30.0f, 60.0f);
                        else if (items[i].type == ITEM_SPEED_UP) {
                            speed.x = CLAMP(speed.x * 1.2f, -12.0f, 12.0f);
                            speed.y = CLAMP(speed.y * 1.2f, -12.0f, 12.0f);
                        }
                        items[i].active = 0;
                    }
                    if (items[i].position.y > SCREEN_HEIGHT)
                        items[i].active = 0;
                }
            }

            ClearBackground(BLACK);
            DrawRectangleRec(paddle, WHITE);
            DrawCircleV(ball, radius, WHITE);

            for (int i = 0; i < MAX_BLOCKS; i++) {
                if (active[i]) {
                    int colorIndex = CLAMP(blockHealth[i], 0, 5);
                    DrawRectangleRec(blocks[i], healthColors[colorIndex]);
                }
            }

            DrawText(TextFormat("Score: %d", score), 20, 20, 20, WHITE);
            DrawText(TextFormat("Lifes: %d", lifes), SCREEN_WIDTH - 120, 20, 20, RED);
            DrawText(TextFormat("High Score: %d", highScore), SCREEN_WIDTH / 2 - 100, 20, 20, GOLD);

            if (score >= 1500) {
                gamestate = STATE_ENDING;
                PlaySound(gameClearSound);
                if (score > highScore) {
                    highScore = score;
                    SaveHighScore("highscore.txt", highScore);
                }
            }

            EndDrawing();
        }
        else if (gamestate == STATE_GAMEOVER || gamestate == STATE_ENDING) {
            BeginDrawing();
            ClearBackground(BLACK);
            DrawText(gamestate == STATE_GAMEOVER ? "GAME OVER" : "YOU WON!", SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 - 60, 60, gamestate == STATE_GAMEOVER ? RED : GREEN);
            DrawText("Press ENTER to Restart", SCREEN_WIDTH / 2 - 180, SCREEN_HEIGHT / 2 + 10, 30, LIGHTGRAY);
            if (IsKeyPressed(KEY_ENTER)) gamestate = STATE_START;
            EndDrawing();
        }
    }

    UnloadSound(blockBreakSound);
    UnloadSound(gameOverSound);
    UnloadSound(gameClearSound);
    UnloadMusicStream(bgm);
    CloseAudioDevice();

    CloseWindow();
    return 0;
}
