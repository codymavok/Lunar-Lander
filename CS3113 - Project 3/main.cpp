/**
* Author: Karan Singh 
* Assignment: Lunar Lander 
* Date due: 2025-10-25, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#include "raylib.h"
#include <math.h>
#include "CS3113/cs3113.h"
#include "CS3113/Entity.h"

// Global Constants
constexpr int SCREEN_WIDTH  = 1600 / 2,
              SCREEN_HEIGHT = 900 / 2,
              FPS           = 60,
              SIZE          = 500 / 2,
              SPEED         = 200;

constexpr float ACCELERATION_OF_GRAVITY = 50.0f,
                FIXED_TIMESTEP          = 1.0f / 60.0f;

constexpr char    BG_COLOUR[]    = "#F8F1C8";
constexpr char BACKGROUND[] = "Assets/4_81.png";
constexpr Vector2 ORIGIN         = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },
                  BASE_SIZE      = { (float) SIZE, (float) SIZE },
                  PLAYER_INIT_POS  = { ORIGIN.x , ORIGIN.y };

constexpr int NUMBER_OF_COLLIDABLES = 2;

enum GameState {IP, WON, LOST}; // Game State is either In Progress (IP), Won, or Lost.
GameState gamestate = IP; 

Texture2D backgroundTexture;

// Global Variables
AppStatus gAppStatus     = RUNNING;
float     gAngle         = 0.0f,
          gPreviousTicks = 0.0f,
          gTimeAccumulator = 0.0f,
          gFuel = 500.0f, // Max Fuel that the ship can have 
          gFuelConsumptionRate = 10.0f; // Each time the player accelerates this is how much fuel is used

Entity *gPlayer = nullptr;
Entity gCollidables[NUMBER_OF_COLLIDABLES];
Entity *gExplosion = nullptr;

bool showExplosion = false; 

unsigned int startTime;
float colourtime = 0.0f;

// Function Declarations
void initialise();
void processInput();
void update();
void render();
void shutdown();
bool isColliding(const Vector2 *positionA, const Vector2 *scaleA, const Vector2 *positionB, const Vector2 *scaleB);

// Function Definitions

/**
 * @brief Checks for a square collision between 2 Rectangle objects.
 * 
 * @see 
 * 
 * @param positionA The position of the first object
 * @param scaleA The scale of the first object
 * @param positionB The position of the second object
 * @param scaleB The scale of the second object
 * @return true if a collision is detected,
 * @return false if a collision is not detected
 */
bool isColliding(const Vector2 *positionA,  const Vector2 *scaleA, 
                 const Vector2 *positionB, const Vector2 *scaleB)
{
    float xDistance = fabs(positionA->x - positionB->x) - ((scaleA->x + scaleB->x) / 2.0f);
    float yDistance = fabs(positionA->y - positionB->y) - ((scaleA->y + scaleB->y) / 2.0f);

    if (xDistance < 0.0f && yDistance < 0.0f) return true;

    return false;
}

void renderObject(const Texture2D *texture, const Vector2 *position, 
                  const Vector2 *scale)
{
    // Whole texture (UV coordinates)
    Rectangle textureArea = {
        // top-left corner
        0.0f, 0.0f,

        // bottom-right corner (of texture)
        static_cast<float>(texture->width),
        static_cast<float>(texture->height)
    };

    // Destination rectangle – centred on gPosition
    Rectangle destinationArea = {
        position->x,
        position->y,
        static_cast<float>(scale->x),
        static_cast<float>(scale->y)
    };

    // Origin inside the source texture (centre of the texture)
    Vector2 originOffset = {
        static_cast<float>(scale->x) / 2.0f,
        static_cast<float>(scale->y) / 2.0f
    };

    // Render the texture on screen
    DrawTexturePro(
        *texture, 
        textureArea, destinationArea, originOffset,
        gAngle, WHITE
    );
}

void initialise()
{
     InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Project 3");
     backgroundTexture = LoadTexture(BACKGROUND);

    startTime = time(NULL);

    float sizeRatio  = 48.0f / 64.0f;

    // Animation for explosion 
    std::map<Direction, std::vector<int>> animationAtlas = {
    {RIGHT, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9,           
             10, 11, 12, 13, 14, 15, 16, 17, 18, 19,  
             20, 21, 22, 23, 24, 25, 26, 27, 28, 29,  
             30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 
             40, 41, 42, 43, 44, 45, 46, 47, 48, 49}} 
    };

    gExplosion = new Entity(
        {0, 0},                    
        {150.0f, 150.0f},          
        "Assets/toon.png",        
        ATLAS,                     
        {5, 10},                   
        animationAtlas,            
        NONE                      
    );

    gExplosion->setFrameSpeed(30);  
    gExplosion->deactivate(); 
    gExplosion->setMovement({0.0f, 1.0f});   


    // UFO 
    gCollidables[0].setTexture("Assets/pngtree-pixel-art-ufo-icon-design-vector-png-image_6125116.png");
    gCollidables[0].setPosition({ORIGIN.x, ORIGIN.y + 50.0f});
    gCollidables[0].setScale({120.0f * sizeRatio, 120.0f * sizeRatio});
    gCollidables[0].setEntityType(UFO);
    gCollidables[0].setColliderDimensions({55.0f, 25.0f}); 
    gCollidables[0].setSpeed(SPEED);
    gCollidables[0].setMovement({1.0f, 0.0f});

    // Platform
    gCollidables[1].setTexture("Assets/platform-2.png");
    gCollidables[1].setPosition({ORIGIN.x, ORIGIN.y + 150.0f});
    gCollidables[1].setScale({150.0f * sizeRatio, 175.0f * sizeRatio});
    gCollidables[1].setEntityType(PLATFORM);
    gCollidables[1].setColliderDimensions({65.0f, 70.0f});
    gCollidables[1].setSpeed(SPEED);
    gCollidables[1].setMovement({0.5f, 0.0f});



    gPlayer = new Entity(
        {ORIGIN.x, 45.0f},
        {100.0f * sizeRatio, 100.0f},
        "Assets/196207-200.png",
        PLAYER
    );
    gPlayer->UseAccelerationPhysics();
    gPlayer->setColliderDimensions({50.0f, 70.0f});


    gPlayer->setAcceleration({0.0f, ACCELERATION_OF_GRAVITY});

    SetTargetFPS(FPS);
}

void processInput() 
{
    if (WindowShouldClose()) gAppStatus = TERMINATED;

    float x_acceleration = 0.0f;
    float y_acceleration = ACCELERATION_OF_GRAVITY;

    if (gFuel > 0){

     if (IsKeyDown(KEY_W)) {
        y_acceleration -= 100.0f;
        gFuel -= gFuelConsumptionRate * FIXED_TIMESTEP;
     } 
     
     if (IsKeyDown(KEY_A)) {
        x_acceleration -= 100.0f;
        gFuel -= gFuelConsumptionRate * FIXED_TIMESTEP;
     }

     if (IsKeyDown(KEY_D)) {
        x_acceleration += 100.0f;
        gFuel -= gFuelConsumptionRate * FIXED_TIMESTEP;
     }


     gPlayer->setAcceleration({x_acceleration, y_acceleration});

     
    }
     
    if (IsKeyPressed(KEY_Q) || WindowShouldClose()) gAppStatus = TERMINATED;

}

void update() 
{
    // Delta time
    float ticks = (float) GetTime();
    float deltaTime = ticks - gPreviousTicks;
    gPreviousTicks  = ticks;

    // Fixed timestamp
    deltaTime += gTimeAccumulator; 


    if (deltaTime < FIXED_TIMESTEP)
    {
        gTimeAccumulator = deltaTime;
        return;
    }

    while (deltaTime >= FIXED_TIMESTEP) {

        if (gamestate == IP){

            gPlayer->update(FIXED_TIMESTEP, gCollidables, NUMBER_OF_COLLIDABLES);

            if (gPlayer->hitUFO()) {
                 gamestate = LOST;

                 // Explosion with happen here 
                 gExplosion->setPosition(gPlayer->getPosition());
                 gExplosion->activate();
                 showExplosion = true;
            }

             if (gPlayer->landedOnPlatform()) {
                gamestate = WON;
            }

            Vector2 playerPos = gPlayer->getPosition();
            if (playerPos.x < 0 || playerPos.x > SCREEN_WIDTH || 
                playerPos.y < 0 || playerPos.y > SCREEN_HEIGHT) {
                gamestate = LOST;  
            }


            Vector2 ufoPos = gCollidables[0].getPosition();
            if (ufoPos.x > SCREEN_WIDTH - 50) {
                gCollidables[0].setMovement({-1.0f, 0.0f}); 
            }
            else if (ufoPos.x < 50) {
                gCollidables[0].setMovement({0.7f, 0.0f});   
            }   
            gCollidables[0].update(FIXED_TIMESTEP, nullptr, 0);

            Vector2 platformPos = gCollidables[1].getPosition();
            if (platformPos.x > SCREEN_WIDTH - 50) {
                gCollidables[1].setMovement({-1.0f, 0.0f}); 
            }
            else if (platformPos.x < 50) {
                gCollidables[1].setMovement({1.0f, 0.0f});
            }   
            gCollidables[1].update(FIXED_TIMESTEP, nullptr, 0);
        }
       
        if (showExplosion) {
            gExplosion->update(FIXED_TIMESTEP, nullptr, 0);
        }

        deltaTime -= FIXED_TIMESTEP;
    }

}

void render()
{
    BeginDrawing();
    ClearBackground(ColorFromHex(BG_COLOUR));

    DrawTexturePro(
        backgroundTexture,
        {0, 0, (float)backgroundTexture.width, (float)backgroundTexture.height},  
        {0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT}, 
        {0, 0}, 
        0.0f,  
        WHITE 
    );

    // Render Player
    if (!showExplosion) {
        gPlayer->render();
    }

     for (int i = 0; i < NUMBER_OF_COLLIDABLES; i++) {
        gCollidables[i].render();
    }

    if (showExplosion) {
        gExplosion->render();
    }

    Color fuelColor = gFuel > 30 ? BLACK : RED;
    DrawText(TextFormat("Fuel: %.0f", gFuel), 10, 10, 20, fuelColor);

    if (gamestate == WON) {
        DrawText("You Won!", 
                 SCREEN_WIDTH/2 - 180, SCREEN_HEIGHT/2, 40, GREEN);
    }
    else if (gamestate == LOST) {
        DrawText("You Lost :(", 
                 SCREEN_WIDTH/2 - 140, SCREEN_HEIGHT/2, 40, RED);
    }


    EndDrawing();
}

void shutdown() { 
    
    CloseWindow(); 
    
  
}

int main(void)
{
    initialise();

    while (gAppStatus == RUNNING)
    {
        processInput();
        update();
        render();
    }

    shutdown();

    return 0;
}
