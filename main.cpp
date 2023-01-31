#include "sdlx11.hpp"

enum State
{
    IDLE,
    IDLE2,
    IDLE3,
    IDLE4,
    IDLE5,
    WALK,
};

enum Direction
{
    LEFT,
    RIGHT
};

class Cat
{
    public:
        Cat(SDL_Renderer *_renderer, SDL_Window* _window, SDL_DisplayMode _dm)
        {
            renderer = _renderer;
            window = _window;
            dm = _dm;
            image = IMG_Load("cat.png");
            texture = SDL_CreateTextureFromSurface(renderer, image);
            state = State::WALK;
            direction = Direction::RIGHT;

            x = dm.w / 3;
            y = dm.h - 64;
            SDL_SetWindowPosition(window, dm.w / 2, dm.h - 64);
        }

        ~Cat()
        {
            SDL_DestroyTexture(texture);
            SDL_FreeSurface(image);
        }

        void update()
        {
            ticks = SDL_GetTicks();

            if (state == State::IDLE) {
                sprite = (ticks / SPEED) % 4;
                srcrect = { sprite * 32, 0, 32, 32 };
                dstrect = { 0, 0, 32, 32 };
            }
            else if (state == State::IDLE2) {
                sprite = (ticks / SPEED) % 4;
                srcrect = { sprite * 32, 32, 32, 32 };
                dstrect = { 0, 0, 32, 32 };
            }
            else if (state == State::IDLE3) {
                sprite = (ticks / SPEED) % 4;
                srcrect = { sprite * 32, 64, 32, 32 };
                dstrect = { 0, 0, 32, 32 };
            }
            else if (state == State::IDLE4) {
                sprite = (ticks / SPEED) % 4;
                srcrect = { sprite * 32, 96, 32, 32 };
                dstrect = { 0, 0, 32, 32 };
            }
            else if (state == State::IDLE5) {
                sprite = (ticks / SPEED) % 4;
                srcrect = { sprite * 32, 192, 32, 32 };
                dstrect = { 0, 0, 32, 32 };
            }
            else if (state == State::WALK && direction == Direction::RIGHT) {
                sprite = (ticks / SPEED) % 8;
                srcrect = { sprite * 32, 128, 32, 32 };
                dstrect = { 0, 0, 32, 32 };

                if (x < dm.w - 32) {
                    x++;
                    SDL_SetWindowPosition(window, x, y);
                }
                else {
                    direction = Direction::LEFT;
                }
            }
            else if (state == State::WALK && direction == Direction::LEFT) {
                sprite = (ticks / SPEED) % 8;
                srcrect = { sprite * 32, 128, 32, 32 };
                dstrect = { 0, 0, 32, 32 };

                if (x > 0) {
                    x--;
                    SDL_SetWindowPosition(window, x, y);
                }
                else {
                    direction = Direction::RIGHT;
                }
            }
        }

        void draw()
        {
            if (direction == Direction::LEFT) {
                SDL_RenderCopyEx(renderer, texture, &srcrect, &dstrect, 0, NULL, SDL_FLIP_HORIZONTAL);
            }
            else if (direction == Direction::RIGHT) {
                SDL_RenderCopyEx(renderer, texture, &srcrect, &dstrect, 0, NULL, SDL_FLIP_NONE);
            }
        }

    private:
        SDL_Renderer *renderer;
        SDL_Window* window;
        SDL_DisplayMode dm;

        SDL_Texture *texture;
        SDL_Surface *image;

        int sprite;
        int ticks;
        SDL_Rect srcrect;
        SDL_Rect dstrect;

        int x;
        int y;
        State state;
        Direction direction;
};

class MySDLx11App : public SDLx11
{
    public:
        void init()
        {
            SDL_Create("Cat", 0, 0, 32, 32, 0, false, 1.0f);

            if (SDL_GetDesktopDisplayMode(0, &dm) != 0)
            {
                SDL_Log("SDL_GetDesktopDisplayMode failed: %s", SDL_GetError());
                return quit();
            }
        }

        void run()
        {
            SDL_Event event;
            bool done = false;

            init();

            Cat *cat = new Cat(renderer_, sdl_window_, dm);

            while (!done)
            {
                cat->update();

                while (SDL_PollEvent(&event) != NULL)
                {
                    switch (event.type)
                    {
                        case SDL_QUIT:
                            done = true;
                            break;
                    }
                }

                SDL_RenderClear(renderer_);
                cat->draw();
                SDL_RenderPresent(renderer_);
            }

            quit();
        }

        void quit()
        {
            SDL_Destroy();
        }
};

int main(int argc, char** argv)
{
    MySDLx11App app;
    app.run();
    return 0;
}