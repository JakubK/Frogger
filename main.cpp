#include<math.h>
#include<stdio.h>
#include<string>

extern "C" {
#include"SDL2-2.0.10/include/SDL.h"
#include"SDL2-2.0.10/include/SDL_main.h"
}

#define SCREEN_WIDTH	630
#define SCREEN_HEIGHT	630
#define INFOBOX_OFFSET	30
#define CELL_SIZE		SCREEN_WIDTH/14
#define FROG_SPEED		2

struct Entity
{
  float X, Y, Width, Height;
};

struct MovingEntity
{
  Entity entity;
  int direction;
  int velocity;
};

bool Intersects(Entity first, Entity second)
{
  if (first.X < second.X + second.Width 
    && first.X + first.Width > second.X
    && first.Y < second.Y + second.Height
    && first.Y + first.Height > second.Y)
    return true;
  return false;
}

// narysowanie napisu txt na powierzchni screen, zaczynajπc od punktu (x, y)
// charset to bitmapa 128x128 zawierajπca znaki
// draw a text txt on surface screen, starting from the point (x, y)
// charset is a 128x128 bitmap containing character images
void DrawString(SDL_Surface *screen, int x, int y, const char *text,
  SDL_Surface *charset) {
  int px, py, c;
  SDL_Rect s, d;
  s.w = 8;
  s.h = 8;
  d.w = 8;
  d.h = 8;
  while (*text) {
    c = *text & 255;
    px = (c % 16) * 8;
    py = (c / 16) * 8;
    s.x = px;
    s.y = py;
    d.x = x;
    d.y = y;
    SDL_BlitSurface(charset, &s, screen, &d);
    x += 8;
    text++;
  };
};


// narysowanie na ekranie screen powierzchni sprite w punkcie (x, y)
// (x, y) to punkt úrodka obrazka sprite na ekranie
// draw a surface sprite on a surface screen in point (x, y)
// (x, y) is the center of sprite on screen
void DrawSurface(SDL_Surface *screen, SDL_Surface *sprite, int x, int y) {
  SDL_Rect dest;
  dest.x = x - sprite->w / 2;
  dest.y = y - sprite->h / 2;
  dest.w = sprite->w;
  dest.h = sprite->h;
  SDL_BlitSurface(sprite, NULL, screen, &dest);
};


// rysowanie pojedynczego pixela
// draw a single pixel
void DrawPixel(SDL_Surface *surface, int x, int y, Uint32 color) {
  int bpp = surface->format->BytesPerPixel;
  Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
  *(Uint32 *)p = color;
};


// rysowanie linii o d≥ugoúci l w pionie (gdy dx = 0, dy = 1) 
// bπdü poziomie (gdy dx = 1, dy = 0)
// draw a vertical (when dx = 0, dy = 1) or horizontal (when dx = 1, dy = 0) line
void DrawLine(SDL_Surface *screen, int x, int y, int l, int dx, int dy, Uint32 color) {
  for (int i = 0; i < l; i++) {
    DrawPixel(screen, x, y, color);
    x += dx;
    y += dy;
  };
};


// rysowanie prostokπta o d≥ugoúci bokÛw l i k
// draw a rectangle of size l by k
void DrawRectangle(SDL_Surface *screen, int x, int y, int l, int k,
  Uint32 outlineColor, Uint32 fillColor) {
  int i;
  DrawLine(screen, x, y, k, 0, 1, outlineColor);
  DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
  DrawLine(screen, x, y, l, 1, 0, outlineColor);
  DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
  for (i = y + 1; i < y + k - 1; i++)
    DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
};

void CloseSDL(SDL_Surface *screen, SDL_Texture *scrtex, SDL_Window *window, SDL_Renderer *renderer)
{
  SDL_FreeSurface(screen);
  SDL_DestroyTexture(scrtex);
  SDL_DestroyWindow(window);
  SDL_DestroyRenderer(renderer);
  SDL_Quit();
}

// main
#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char **argv) {
  int t1, t2, quit, frames, rc;
  double delta, worldTime, fpsTimer, fps, distance, etiSpeed;
  SDL_Event event;
  SDL_Surface *screen, *charset;
  SDL_Surface *eti;
  SDL_Surface *river, *road, *brick1, *frogger;
  SDL_Surface *froggger;
  SDL_Surface *crocodilebody, *car;
  SDL_Texture *scrtex;
  SDL_Window *window;
  SDL_Renderer *renderer;

  // okno konsoli nie jest widoczne, jeøeli chcemy zobaczyÊ
  // komunikaty wypisywane printf-em trzeba w opcjach:
  // project -> szablon2 properties -> Linker -> System -> Subsystem
  // zmieniÊ na "Console"
  // console window is not visible, to see the printf output
  // the option:
  // project -> szablon2 properties -> Linker -> System -> Subsystem
  // must be changed to "Console"
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    printf("SDL_Init error: %s\n", SDL_GetError());
    return 1;
  }
  // tryb pe≥noekranowy / fullscreen mode
  //rc = SDL_CreateWindowAndRenderer(0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP,
  //  &window, &renderer);
  	rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0,
  	                                 &window, &renderer);
  if (rc != 0)
  {
    SDL_Quit();
    printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
    return 1;
  };

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
  SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

  SDL_SetWindowTitle(window, "Frogger");


  screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
    0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

  scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
    SDL_TEXTUREACCESS_STREAMING,
    SCREEN_WIDTH, SCREEN_HEIGHT);


  // wy≥πczenie widocznoúci kursora myszy
  //SDL_ShowCursor(SDL_DISABLE);

  // wczytanie obrazka cs8x8.bmp
  charset = SDL_LoadBMP("cs8x8.bmp");
  if (charset == NULL) {
    printf("SDL_LoadBMP(cs8x8.bmp) error: %s\n", SDL_GetError());
    CloseSDL(screen, scrtex, window, renderer);
    return 1;
  };
  SDL_SetColorKey(charset, true, 0x000000);

  eti = SDL_LoadBMP("eti.bmp");
  if (eti == NULL) {
    printf("SDL_LoadBMP(eti.bmp) error: %s\n", SDL_GetError());
    CloseSDL(screen, scrtex, window, renderer);
    return 1;
  };
  river = SDL_LoadBMP("river.bmp");
  if (river == NULL) {
    printf("SDL_LoadBMP(river.bmp) error: %s\n", SDL_GetError());
    CloseSDL(screen, scrtex, window, renderer);
    return 1;
  };
  road = SDL_LoadBMP("road.bmp");
  if (road == NULL) {
    printf("SDL_LoadBMP(river.bmp) error: %s\n", SDL_GetError());
    CloseSDL(screen, scrtex, window, renderer);
    return 1;
  };
  brick1 = SDL_LoadBMP("brick1.bmp");
  if (brick1 == NULL) {
    printf("SDL_LoadBMP(brick1.bmp) error: %s\n", SDL_GetError());
    CloseSDL(screen, scrtex, window, renderer);
    return 1;
  };
  frogger = SDL_LoadBMP("frogger.bmp");
  if (frogger == NULL) {
    printf("SDL_LoadBMP(frogger.bmp) error: %s\n", SDL_GetError());
    CloseSDL(screen, scrtex, window, renderer);
    return 1;
  };
  crocodilebody = SDL_LoadBMP("crocodilebody.bmp");
  if (crocodilebody == NULL) {
    printf("SDL_LoadBMP(crocodilebody.bmp) error: %s\n", SDL_GetError());
    CloseSDL(screen, scrtex, window, renderer);
    return 1;
  };
  car = SDL_LoadBMP("car1.bmp");
  if (car == NULL) {
    printf("SDL_LoadBMP(car.bmp) error: %s\n", SDL_GetError());
    CloseSDL(screen, scrtex, window, renderer);
    return 1;
  };

  char text[128];
  int czarny = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
  int zielony = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
  int czerwony = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
  int niebieski = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);
  int ciemnoszary = SDL_MapRGB(screen->format, 0x26, 0x26, 0x26);
  int zieloniutki = SDL_MapRGB(screen->format, 0xA8, 0xD1, 0x6F);

  t1 = SDL_GetTicks();

  frames = 0;
  fpsTimer = 0;
  fps = 0;
  quit = 0;
  worldTime = 0;
  distance = 0;
  etiSpeed = 1;

  MovingEntity vehicles[5];
  for (int i = 0; i < 5; i++)
  {
    vehicles[i].direction = (i % 2) == 0 ? 1 : -1;
    vehicles[i].velocity = 80 * (i+1);
    vehicles[i].entity.Y = (7 + i) * CELL_SIZE + CELL_SIZE/2;
    vehicles[i].entity.X = SCREEN_WIDTH / 2;

    vehicles[i].entity.Width = CELL_SIZE;
    vehicles[i].entity.Height = CELL_SIZE;
  }

  MovingEntity logs[3];
  for (int i = 0; i < 3; i++)
  {
    logs[i].direction = -1;
    logs[i].velocity = 100;
    logs[i].entity.Y = (1 + i) * CELL_SIZE + CELL_SIZE / 2;
    logs[i].entity.X = SCREEN_WIDTH / 2;

    logs[i].entity.Width = CELL_SIZE;
    logs[i].entity.Height = CELL_SIZE;
  }
  

  Entity player;
  player.X = 7 * CELL_SIZE + CELL_SIZE/2;
  player.Y = 12 * CELL_SIZE + CELL_SIZE / 2;
  player.Width = player.Height = CELL_SIZE;

  while (!quit) {
    t2 = SDL_GetTicks();

    // w tym momencie t2-t1 to czas w milisekundach,
    // jaki uplyna≥ od ostatniego narysowania ekranu
    // delta to ten sam czas w sekundach
    // here t2-t1 is the time in milliseconds since
    // the last screen was drawn
    // delta is the same time in seconds
    delta = (t2 - t1) * 0.001;
    t1 = t2;

    worldTime += delta;

    distance += etiSpeed * delta;

    //move each vehicle
    for (int i = 0; i < 5; i++)
      vehicles[i].entity.X += vehicles[i].velocity * delta * vehicles[i].direction;

    //move each log
    for(int i = 0;i < 3;i++)
      logs[i].entity.X += logs[i].velocity * delta * logs[i].direction;
    //check if it's urgent to respawn it on the opposite side
    for (int i = 0; i < 5; i++)
    {
      if (vehicles[i].direction == -1)
      {
        if (vehicles[i].entity.X < -SCREEN_WIDTH)
          vehicles[i].entity.X = SCREEN_WIDTH;
      }
      else
      {
        if (vehicles[i].entity.X > 2*SCREEN_WIDTH)
          vehicles[i].entity.X = 0;
      }
    }

    for (int i = 0; i < 3; i++)
    {
        if (logs[i].entity.X > 2 * SCREEN_WIDTH)
          logs[i].entity.X = 0;
    }

    //Check if frog intersects with the car
    for (int i = 0; i < 5; i++)
    {
      if (Intersects(player, vehicles[i].entity))
      {
        SDL_Quit();
      }
    }

    //Check if frog intersects with Log/Turtle
    for (int i = 0; i < 3; i++)
    {
      if (Intersects(player, logs[i].entity))
      {
        player.X += logs[i].velocity * delta * logs[i].direction;
      }
    }

    SDL_FillRect(screen, NULL, ciemnoszary);

    //Check if Frog is out of Screen
    if (player.X < -CELL_SIZE || player.X > SCREEN_WIDTH + CELL_SIZE)
      SDL_Quit();


    DrawSurface(screen, river,
      SCREEN_WIDTH / 2,
      3 * CELL_SIZE);
    DrawSurface(screen, road,
      SCREEN_WIDTH / 2,
      CELL_SIZE / 2 + 9 * CELL_SIZE);

    for (int i = 0; i < 14; i++)
    {
        DrawSurface(screen, brick1,
          CELL_SIZE / 2 + i * CELL_SIZE,
          CELL_SIZE / 2 + 6 * CELL_SIZE);
    }
    for (int i = 0; i < 14; i++)
    {
        DrawSurface(screen, brick1,
          CELL_SIZE / 2 + i * CELL_SIZE,
          CELL_SIZE / 2 + 12 * CELL_SIZE);
    }
    DrawSurface(screen, crocodilebody,
      CELL_SIZE / 2 + 6 * CELL_SIZE,
      CELL_SIZE / 2 + 3 * CELL_SIZE);

    DrawSurface(screen, frogger,
      player.X,
      player.Y);

    for (int i = 0; i < 5; i++)
    {
      DrawSurface(screen, car,
        vehicles[i].entity.X,
        vehicles[i].entity.Y);
    }

    for (int i = 0; i < 3; i++)
    {
      DrawSurface(screen, car,
        logs[i].entity.X,
        logs[i].entity.Y);
    }

    fpsTimer += delta;
    if (fpsTimer > 0.5) {
      fps = frames * 2;
      frames = 0;
      fpsTimer -= 0.5;
    };


    // tekst informacyjny / info text
    DrawRectangle(screen, 4, SCREEN_HEIGHT - INFOBOX_OFFSET, SCREEN_WIDTH - 8, 18, zieloniutki, czarny);
    //            "template for the second project, elapsed time = %.1lf s  %.0lf frames / s"
    sprintf(text, "Frogger, time in game: %.1lf s  %.0lf fps", worldTime, fps);
    DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_HEIGHT - INFOBOX_OFFSET + 5, text, charset);
    //	      "Esc - exit, \030 - faster, \031 - slower"
    //sprintf(text, "Esc - wyjscie, \030 - przyspieszenie, \031 - zwolnienie");
    //DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 26, text, charset);

    SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
    //		SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, scrtex, NULL, NULL);
    SDL_RenderPresent(renderer);

    // obs≥uga zdarzeÒ (o ile jakieú zasz≥y) / handling of events (if there were any)
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_q)
          quit = 1;
        else if (event.key.keysym.sym == SDLK_UP)
        {
          if(player.Y - CELL_SIZE >= 0)
            player.Y -= CELL_SIZE;
        }
        else if (event.key.keysym.sym == SDLK_DOWN)
        {
          if(player.Y + CELL_SIZE <= CELL_SIZE * 13)
            player.Y += CELL_SIZE;
        }
        else if (event.key.keysym.sym == SDLK_LEFT)
        {
          if(player.X - CELL_SIZE >= 0)
            player.X -= CELL_SIZE;
        }
        else if (event.key.keysym.sym == SDLK_RIGHT)
        {
          if(player.X + CELL_SIZE <= CELL_SIZE * 13)
            player.X += CELL_SIZE;
        }
        break;
      };
    };
    frames++;
  };

  // zwolnienie powierzchni / freeing all surfaces
  SDL_FreeSurface(charset);
  SDL_FreeSurface(screen);
  SDL_DestroyTexture(scrtex);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  SDL_Quit();
  return 0;
};