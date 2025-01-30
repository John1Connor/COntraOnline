#define main SDL_main
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <process.h>
#include "network/network.h"

typedef struct
{
    SDL_Rect rect;
    SDL_Texture *texture;
    int isHovered;
} Button;

NetworkManager networkManager = {0};
int isNetworkInitialized = 0;
int gameStarted = 0;
int playersReady = 0;

SDL_Texture *create_text_texture(SDL_Renderer *renderer, TTF_Font *font, const char *text, SDL_Color color)
{
    SDL_Surface *surface = TTF_RenderText_Solid(font, text, color);
    if (!surface)
    {
        SDL_Log("TTF_RenderText error: %s", TTF_GetError());
        return NULL;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

Button create_button(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y)
{
    SDL_Color color = {255, 255, 255, 255};
    SDL_Texture *texture = create_text_texture(renderer, font, text, color);

    int w, h;
    SDL_QueryTexture(texture, NULL, NULL, &w, &h);
    SDL_Rect rect = {x, y, w, h};

    return (Button){rect, texture, 0};
}

unsigned int __stdcall network_thread(void *data)
{
    NetworkManager *nm = (NetworkManager *)data;
    char buffer[BUFFER_SIZE];

    while (nm->isRunning)
    {
        int bytesReceived = recv(nm->socket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived > 0)
        {
            buffer[bytesReceived] = '\0';

            if (strcmp(buffer, "START") == 0)
            {
                gameStarted = 1;
            }
            else if (strncmp(buffer, "PLAYERS:", 8) == 0)
            {
                playersReady = atoi(buffer + 8);
            }
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
    {
        SDL_Log("SDL_Init Error: %s", SDL_GetError());
        return 1;
    }

    if (TTF_Init() != 0)
    {
        SDL_Log("TTF_Init Error: %s", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    {
        SDL_Log("Mix_OpenAudio Error: %s", Mix_GetError());
        SDL_Quit();
        return 1;
    }

    Mix_Music *bgMusic = Mix_LoadMUS("C:/Users/sspvn/OneDrive/Desktop/ContraOnline/assets/music/title.mp3");
    if (!bgMusic)
    {
        SDL_Log("Failed to load music: %s", Mix_GetError());
    }
    else
    {
        Mix_PlayMusic(bgMusic, -1);
    }

    SDL_DisplayMode displayMode;
    if (SDL_GetCurrentDisplayMode(0, &displayMode) != 0)
    {
        SDL_Log("SDL_GetCurrentDisplayMode Error: %s", SDL_GetError());
        Mix_FreeMusic(bgMusic);
        SDL_Quit();
        return 1;
    }

    int screenWidth = displayMode.w;
    int screenHeight = displayMode.h;

    SDL_Window *window = SDL_CreateWindow("Contra Online", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenWidth, screenHeight, SDL_WINDOW_FULLSCREEN);
    if (!window)
    {
        SDL_Log("SDL_CreateWindow Error: %s", SDL_GetError());
        Mix_FreeMusic(bgMusic);
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
    {
        SDL_Log("SDL_CreateRenderer Error: %s", SDL_GetError());
        Mix_FreeMusic(bgMusic);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    TTF_Font *font = TTF_OpenFont("C:/Users/sspvn/OneDrive/Desktop/ContraOnline/assets/fonts/retro.ttf", 48);
    if (!font)
    {
        SDL_Log("TTF_OpenFont Error: %s", TTF_GetError());
        TTF_Quit();
        Mix_FreeMusic(bgMusic);
        SDL_Quit();
        return 1;
    }

    SDL_Surface *image = IMG_Load("C:/Users/sspvn/OneDrive/Desktop/ContraOnline/assets/images/mainmenu.png");
    if (!image)
    {
        SDL_Log("IMG_Load Error: %s", IMG_GetError());
        TTF_CloseFont(font);
        Mix_FreeMusic(bgMusic);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, image);
    SDL_FreeSurface(image);

    if (!texture)
    {
        SDL_Log("SDL_CreateTextureFromSurface Error: %s", SDL_GetError());
        TTF_CloseFont(font);
        Mix_FreeMusic(bgMusic);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    const int buttonMargin = 100;
    Button btnHost = create_button(renderer, font, "CREAR PARTIDA", buttonMargin, 500);
    Button btnJoin = create_button(renderer, font, "UNIRSE A PARTIDA", buttonMargin, 600);
    Button btnStart = create_button(renderer, font, "COMENZAR", screenWidth - 300, screenHeight - 200);

    int running = 1;
    SDL_Event event;
    int showMainMenu = 1;
    int inLobby = 0;

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
            {
                running = 0;
            }

            if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);

                if (showMainMenu)
                {
                    if (SDL_PointInRect(&(SDL_Point){mouseX, mouseY}, &btnHost.rect))
                    {
                        if (!isNetworkInitialized)
                        {
                            network_init();
                            isNetworkInitialized = 1;
                        }

                        networkManager.role = ROLE_SERVER;
                        if (server_start(&networkManager) == 0)
                        {
                            _beginthreadex(
                                NULL,
                                0,
                                (_beginthreadex_proc_type)server_accept_clients,
                                &networkManager,
                                0,
                                NULL);
                            showMainMenu = 0;
                            inLobby = 1;
                        }
                    }
                    else if (SDL_PointInRect(&(SDL_Point){mouseX, mouseY}, &btnJoin.rect))
                    {
                        if (!isNetworkInitialized)
                        {
                            network_init();
                            isNetworkInitialized = 1;
                        }

                        networkManager.role = ROLE_CLIENT;
                        if (client_connect(&networkManager, "127.0.0.1") == 0)
                        {
                            _beginthreadex(
                                NULL,
                                0,
                                (_beginthreadex_proc_type)network_thread,
                                &networkManager,
                                0,
                                NULL);
                            showMainMenu = 0;
                            inLobby = 1;
                        }
                    }
                }
                else if (inLobby && networkManager.role == ROLE_SERVER)
                {
                    if (SDL_PointInRect(&(SDL_Point){mouseX, mouseY}, &btnStart.rect))
                    {
                        send(networkManager.socket, "START", 5, 0);
                        gameStarted = 1;
                        inLobby = 0;
                    }
                }
            }
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);

        if (showMainMenu)
        {
            SDL_RenderCopy(renderer, btnHost.texture, NULL, &btnHost.rect);
            SDL_RenderCopy(renderer, btnJoin.texture, NULL, &btnJoin.rect);
        }
        else if (inLobby)
        {
            char playerText[50];
            sprintf(playerText, "Jugadores listos: %d/2", playersReady);

            SDL_Color textColor = {255, 255, 255, 255};
            SDL_Texture *playersTexture = create_text_texture(renderer, font, playerText, textColor);
            SDL_Rect playersRect = {buttonMargin, 300, 0, 0};
            SDL_QueryTexture(playersTexture, NULL, NULL, &playersRect.w, &playersRect.h);
            SDL_RenderCopy(renderer, playersTexture, NULL, &playersRect);
            SDL_DestroyTexture(playersTexture);

            if (networkManager.role == ROLE_SERVER)
            {
                SDL_RenderCopy(renderer, btnStart.texture, NULL, &btnStart.rect);
            }
        }
        else if (gameStarted)
        {
            SDL_Color textColor = {255, 255, 255, 255};
            SDL_Texture *gameTexture = create_text_texture(renderer, font, "¡PARTIDA INICIADA!", textColor);
            SDL_Rect gameRect = {(screenWidth - 400) / 2, (screenHeight - 100) / 2, 400, 100};
            SDL_RenderCopy(renderer, gameTexture, NULL, &gameRect);
            SDL_DestroyTexture(gameTexture);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(10);
    }

    networkManager.isRunning = 0;
    if (isNetworkInitialized)
    {
        closesocket(networkManager.socket);
        network_cleanup();
    }

    SDL_DestroyTexture(btnHost.texture);
    SDL_DestroyTexture(btnJoin.texture);
    SDL_DestroyTexture(btnStart.texture);
    TTF_CloseFont(font);
    Mix_FreeMusic(bgMusic);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    Mix_CloseAudio();
    SDL_Quit();

    return 0;
}