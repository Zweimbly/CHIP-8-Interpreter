#include "CHIP8.h"

void UpdateSDLSurface(std::vector<uint8_t> display, uint8_t* buffer, uint8_t color)
{  
    for (int i = 0; i < display.size(); i++)
    {
        if (display[i] == 1)
        {
            buffer[i] = color;
        }
        else
        {
            buffer[i] = 0;
        }
    }
}

void HandleKeyboard(std::vector<uint8_t>& keyVector, std::vector<uint8_t>& keymap, SDL_Event &e)
{
    auto key = e.key.keysym.scancode;

    for (int i = 0; i < 16; i++)
    {
        if (key == keymap[i])
        {
            if (e.type == SDL_KEYDOWN)
            {
                keyVector[i] = 1;
            }
            else if (e.type == SDL_KEYUP)
            {
                keyVector[i] = 0;
            }
        }
    }
}

//Main function for running the rom - initiates the CHIP8 CPU, then runs the core game loop
//The display and sound/delay timers are updated at 60Hz, while the CPU performs ops at about 500Hz
//This equates to running 8 CPU cycles per screen/timer update (hence cyclesPerUpdate = 8)
void Run(SDL_Window* window, SDL_Renderer* renderer)
{
    std::vector<uint8_t> keymap = 
    {
        SDL_SCANCODE_X,
        SDL_SCANCODE_1,
        SDL_SCANCODE_2,
        SDL_SCANCODE_3,
        SDL_SCANCODE_Q,
        SDL_SCANCODE_W,
        SDL_SCANCODE_E,
        SDL_SCANCODE_A,
        SDL_SCANCODE_S,
        SDL_SCANCODE_D,
        SDL_SCANCODE_Z,
        SDL_SCANCODE_C,
        SDL_SCANCODE_4,
        SDL_SCANCODE_R,
        SDL_SCANCODE_F,
        SDL_SCANCODE_V
    };

    SDL_Event e;

    //SDL_Surface setup - the binary sets the RGB mask; the CHIP8 display state is read into the pixel array of the Surface in UpdateSDLSurface
    SDL_Surface* surface = SDL_CreateRGBSurface(0, 64, 32, 8, 0b11100000, 0b00011100, 0b00000011, 0);
    uint8_t* buffer = (uint8_t*)surface->pixels;
    uint8_t color = SDL_MapRGB(surface->format, 0xFF, 0xFF, 0xFF);
    
    CHIP8 cpu;
    //cpu.Init("Roms/c8_test.c8");
    //cpu.Init("Roms/Space Invaders [David Winter].ch8");
    //cpu.Init("Roms/Astro Dodge [Revival Studios, 2008].ch8");
    cpu.Init("Roms/IBMLogo.ch8");
    //cpu.Init("Roms/Tetris [Fran Dachille, 1991].ch8");
    bool quit = false;

    //Main game loop
    while (!quit)
    {      
        cpu.cyclesPerUpdate = 8;
        auto tStart = std::chrono::high_resolution_clock::now();
        
        while (cpu.cyclesPerUpdate > 0)
        {
            cpu.RunCycle();
            cpu.cyclesPerUpdate--;
        }

        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                quit = true;
            }

            if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
            {
                HandleKeyboard(cpu.keyboardState, keymap, e);
            }
        }

        if (cpu.drawFlag)
        {
            // for (int i = 0; i < cpu.display.size(); i++)
            // {
            //     if (cpu.display[i] == 1)
            //     {
            //         buffer[i] = color;
            //     }
            //     else
            //     {
            //         buffer[i] = 0;
            //     }
            // }

            UpdateSDLSurface(cpu.display, buffer, color);
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);

            cpu.drawFlag = false;
        }

        if (cpu.delayTimer > 0)
        {
            cpu.delayTimer--;
        }
        if (cpu.soundTimer > 0)
        {
            cpu.soundTimer--;
        }

        //Clock calculations and determining how long to sleep for
        auto tEnd = std::chrono::high_resolution_clock::now();
        auto tElapsed = std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(tEnd - tStart);
        auto tTarget = std::chrono::duration<double, std::micro>(16666.66);

        if (tElapsed < tTarget)
        {
            std::this_thread::sleep_for(tTarget - tElapsed);
        }
    }
}

int main(int argc, char* args[])
{
    //Initialize SDL window
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window* window = NULL;

    //Screen dimensions for SDL window
    const int windowWidth = 64*8;
    const int windowHeight = 32*8;

    //Creating window, renderer, and surface (with hardcoded 64 x 32 dimensions for the surface)
    window = SDL_CreateWindow("CHIP8 Interpreter", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    
    //Start running the CHIP8 interpreter via Run()
    try
    {
        Run(window, renderer);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error running in main: " << e.what();
    }

    //Clean up SDL stuff on quit
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}