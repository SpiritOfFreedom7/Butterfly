#define SDL_MAIN_HANDLED

#include <iostream>
#include <clocale>
#include <deque>
#include <string>
#include <windows.h>
#include <chrono>
#include <algorithm>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define BUILDING_HEALTH 7000
#define BUILD_W 90
#define BUILD_H 100

using namespace std;

class Sprite {
public:
    SDL_Texture* texture;
    int frameWidth;
    int frameHeight;
    int numFrames;

    Sprite() : texture(nullptr), frameWidth(0), frameHeight(0), numFrames(0) {}
};

class GameObject { //класс объектов
public:
    SDL_Rect rect;
    double speed;

    GameObject(int x, int y, int w, int h, double spd)
        : rect{ x, y, w, h }, speed(spd) {
    }

    virtual void render(SDL_Renderer* renderer, Sprite& sprite) = 0;
    virtual ~GameObject() {}
};

class Butterfly : public GameObject {
public:
    int damage;
    int frame;

    Butterfly(int x, int y, int w, int h, double spd, double dmg)
        :GameObject(x, y, w, h, spd), damage(dmg), frame(0) {
    }

    void render(SDL_Renderer* renderer, Sprite& sprite) override {
        SDL_Rect srcRect = { frame * sprite.frameWidth, 0, sprite.frameWidth, sprite.frameHeight };
        SDL_RenderCopy(renderer, sprite.texture, &srcRect, &rect);
    }
};

class building : public GameObject {
public:
    int health;
    int frame;

    building(int hel, int x, int y, int w, int h, int spd = 0)
        : GameObject(x, y, w, h, spd), health(hel), frame(0) {
        cout << "--Вызван конструктор с парамертами building()" << endl;
    }

    ~building() {
        cout << "--Вызван деструктор building()" << endl;
    }

    void render(SDL_Renderer* renderer, Sprite& sprite) override {
        SDL_Rect srcRect = { frame * sprite.frameWidth, 0, sprite.frameWidth, sprite.frameHeight };
        SDL_RenderCopy(renderer, sprite.texture, &srcRect, &rect);
    }
};

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
Sprite ButterflySprite;
Sprite buildingSprite;
SDL_Texture* backgroundTexture = nullptr;
Mix_Music* music = nullptr;
Mix_Chunk* SrokeSound = nullptr;
Mix_Chunk* DestroySound = nullptr;
Butterfly butterfly(10, 10, 160, 140, 10, 10);
std::deque<building> builds;

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow("Mad Butterfly", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        std::cerr << "Window could not be created! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr) {
        std::cerr << "Renderer could not be created! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }

    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        std::cerr << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;
        return false;
    }

    if (TTF_Init() == -1) {
        std::cerr << "Failed to initialize SDL_ttf! SDL_ttf Error: " << TTF_GetError() << std::endl;
        return false;
    }
    // Инициализация SDL_mixer
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << std::endl;
        return false;
    }

    return true;
}

SDL_Texture* loadTexture(const std::string& path) {
    SDL_Texture* newTexture = nullptr;
    SDL_Surface* loadedSurface = IMG_Load(path.c_str());
    if (loadedSurface == nullptr) {
        std::cerr << "Unable to load image " << path << "! SDL_image Error: " << IMG_GetError() << std::endl;
    }
    else {
        // Установка цвета для прозрачности
        SDL_SetColorKey(loadedSurface, SDL_TRUE, SDL_MapRGB(loadedSurface->format, 0, 0, 0));

        newTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
        if (newTexture == nullptr) {
            std::cerr << "Unable to create texture from " << path << "! SDL Error: " << SDL_GetError() << std::endl;
        }
        else {
            // Устанавливаем режим смешивания для текстуры
            SDL_SetTextureBlendMode(newTexture, SDL_BLENDMODE_BLEND);
        }

        SDL_FreeSurface(loadedSurface);
    }
    return newTexture;
}

bool loadMedia() {
    ButterflySprite.texture = loadTexture("images/butterfly.png");
    ButterflySprite.frameWidth = 160;
    ButterflySprite.frameHeight = 140;
    ButterflySprite.numFrames = 3;

    buildingSprite.texture = loadTexture("images/building.png");
    buildingSprite.frameWidth = 90;
    buildingSprite.frameHeight = 100;
    buildingSprite.numFrames = 15;

    backgroundTexture = loadTexture("images/background_3.png");

    if (ButterflySprite.texture == nullptr || buildingSprite.texture == nullptr || backgroundTexture == nullptr) {
        return false;
    }
    // Загрузка звуков
    music = Mix_LoadMUS("sounds/bckg_2.wav");
    if (music == nullptr) {
        std::cerr << "Failed to load background music! SDL_mixer Error: " << Mix_GetError() << std::endl;
        return false;
    }

    SrokeSound = Mix_LoadWAV("sounds/butterfly_stroke.wav");
    if (SrokeSound == nullptr) {
        std::cerr << "Failed to load stroke sound effect! SDL_mixer Error: " << Mix_GetError() << std::endl;
        return false;
    }

    DestroySound = Mix_LoadWAV("sounds/building_destroing.wav");
    if (DestroySound == nullptr) {
        std::cerr << "Failed to load destroy dound effect! SDL_mixer Error: " << Mix_GetError() << std::endl;
        return false;
    }
    //воспроизведение фоновой музыки
    Mix_PlayMusic(music, -1);

    return true;
}

void close() { // Освобождение памяти
    if (ButterflySprite.texture != nullptr) {
        SDL_DestroyTexture(ButterflySprite.texture);
        ButterflySprite.texture = nullptr;
    }

    if (buildingSprite.texture != nullptr) {
        SDL_DestroyTexture(buildingSprite.texture);
        buildingSprite.texture = nullptr;
    }

    if (backgroundTexture != nullptr) {
        SDL_DestroyTexture(backgroundTexture);
        backgroundTexture = nullptr;
    }

    if (renderer != nullptr) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }

    if (window != nullptr) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    Mix_FreeMusic(music);
    Mix_FreeChunk(SrokeSound);
    Mix_FreeChunk(DestroySound);
    Mix_Quit();

    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}

void drawText(const std::string& text, int x, int y, int fontSize, SDL_Color color) { //подключение шрифта
    TTF_Font* font = TTF_OpenFont("Bender.ttf", fontSize);
    if (font == nullptr) {
        std::cerr << "Failed to load font! SDL_ttf Error: " << TTF_GetError() << std::endl;
        return;
    }

    SDL_Surface* textSurface = TTF_RenderUTF8_Solid(font, text.c_str(), color);
    if (textSurface == nullptr) {
        std::cerr << "Unable to render text surface! SDL_ttf Error: " << TTF_GetError() << std::endl;
        TTF_CloseFont(font);
        return;
    }

    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (textTexture == nullptr) {
        std::cerr << "Unable to create texture from rendered text! SDL Error: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(textSurface);
        TTF_CloseFont(font);
        return;
    }

    SDL_Rect textRect = { x, y, textSurface->w, textSurface->h };
    SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
    //очистка памяти
    SDL_DestroyTexture(textTexture);
    SDL_FreeSurface(textSurface);
    TTF_CloseFont(font);
}


void showGameOverScreen(int score, bool res) { //GAME OVER
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderClear(renderer);
    if (res) drawText(u8"Вы выиграли!", SCREEN_WIDTH / 2 - 180, SCREEN_HEIGHT / 2 - 120, 48, { 255, 0, 0, 255 });
    else { drawText(u8"Вы проиграли!", SCREEN_WIDTH / 2 - 180, SCREEN_HEIGHT / 2 - 120, 48, { 255, 0, 0, 255 }); }
    drawText(u8"Игра окончена!", SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT / 2 - 50, 48, { 255, 0, 0, 255 });
    drawText(u8"Ваш счёт: " + std::to_string(score), SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 + 50, 36, { 255, 0, 0, 255 });
    SDL_RenderPresent(renderer);
    //Нажатие Enter
    SDL_Event event;
    bool enterPressed = false;
    while (!enterPressed) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_RETURN) {
                    enterPressed = true;
                }
            }
        }
    }
}



int main(int argc, char* argv[])
{
    setlocale(LC_ALL, "rus");

    if (!init()) {
        std::cerr << "Failed to initialize!" << std::endl;
        return 1;
    }

    if (!loadMedia()) {
        std::cerr << "Failed to load media!" << std::endl;
        close();
        return 1;
    }

    bool quit = false;
    SDL_Event e;
    int score = 7000;
    int i = 0, j = 0, count = 7;

    builds.push_back(building(BUILDING_HEALTH, 1100, 950, BUILD_H, BUILD_W));
    builds.push_back(building(BUILDING_HEALTH, 1200, 880, BUILD_H, BUILD_W));
    builds.push_back(building(BUILDING_HEALTH, 1180, 710, BUILD_H, BUILD_W));
    builds.push_back(building(BUILDING_HEALTH, 1600, 800, BUILD_H, BUILD_W));
    builds.push_back(building(BUILDING_HEALTH, 1400, 850, BUILD_H, BUILD_W));
    builds.push_back(building(BUILDING_HEALTH, 1500, 900, BUILD_H, BUILD_W));
    builds.push_back(building(BUILDING_HEALTH, 1370, 700, BUILD_H, BUILD_W));


    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderClear(renderer);
    drawText("Mad Butterfly", SCREEN_WIDTH / 2 - 150, 100, 52, { 0, 0, 0, 255 });
    drawText(u8"Автор: Давтян Степан", SCREEN_WIDTH / 2 - 250, 200, 28, { 0, 0, 0, 255 });
    drawText(u8"Цель: Уничтожить все здания за наименьшее количество времени.", SCREEN_WIDTH / 2 - 400, 300, 28, { 0, 0, 0, 255 });
    drawText(u8"Управление: пробел - взмах бабочки, Enter - начать игру.", SCREEN_WIDTH / 2 - 350, 400, 28, { 0, 0, 0, 255 });
    SDL_RenderPresent(renderer);

    //Нажатие Enter
    SDL_Event event;
    bool enterPressed = false;
    while (!enterPressed) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_RETURN) {
                    enterPressed = true;
                }
            }
        }
    }

    /*SDLK_RIGHT || SDLK_LEFT*/

    auto start = chrono::high_resolution_clock::now();
    while (!quit) {

        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) {
                quit = true;
            }
            else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_RIGHT || e.key.keysym.sym == SDLK_LEFT && j % 2 == 0) {
                    butterfly.frame++;
                    builds.front().frame++;
                    if (butterfly.frame == 3) butterfly.frame = 0;
                    butterfly.speed += 5;
                    builds.front().health -= butterfly.damage * butterfly.speed;
                    if (builds.front().health < 0) { builds.pop_front(); count--; butterfly.speed = 10;}
                    if (count == 0) {
                        quit = true;
                        showGameOverScreen(score, true); // Показать экран окончания игры
                        break;
                    }
                    Mix_PlayChannel(-1, SrokeSound, 0); // Воспроизведение звука взмаха
                    i++;
                    if (i % 6 == 0) Mix_PlayChannel(-1, DestroySound, 0);
                }
                j++;
            }
        }
        auto end = chrono::high_resolution_clock::now();
        chrono::duration<double> elapsed = end - start;
        if (static_cast<int>(elapsed.count()) % 2 == 0) {
            score -= 10;
            if (score <= 0) {
                quit = true;
                showGameOverScreen(score, false);
                break;
            }
        }
        SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderClear(renderer);

        SDL_RenderCopy(renderer, backgroundTexture, nullptr, nullptr);

        butterfly.render(renderer, ButterflySprite);

        for (auto& build : builds) {
            build.render(renderer, buildingSprite);
        }

        drawText("Счёт: " + std::to_string(score), 10, 10, 32, { 255, 255, 0, 255 });
        SDL_RenderPresent(renderer);

        SDL_Delay(10);
    }
    close();
    return 0;
}
