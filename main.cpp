#include<math.h>
#include<stdio.h>
#include<string>

extern "C" {
#include"SDL2-2.0.10/include/SDL.h"
#include"SDL2-2.0.10/include/SDL_main.h"
}

#define SCREEN_WIDTH	630
#define SCREEN_HEIGHT	630
#define CELL_SIZE		SCREEN_WIDTH/14

#define LOGS_IN_ROW 5
#define TURTLES_IN_ROW 3

#define BOX_OFFSET	30

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

struct Endpoint
{
  Entity entity;
  int activated;
};

int Intersects(Entity first, Entity second)
{
  if (first.X - first.Width/2 < second.X - second.Width/2 + second.Width
    && first.X  - first.Width/2 + first.Width > second.X - second.Width/2
    && first.Y  - first.Height/2< second.Y - second.Height/2 + second.Height
    && first.Y - first.Height/2 + first.Height > second.Y - second.Height/2)
    return 1;
  return 0;
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

  if (x < 0 || y < 0 || x >= surface->w || y >= surface->h)
    return;

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

int InitializeSDL(int * rc,SDL_Window ** window, SDL_Renderer ** renderer, SDL_Surface ** screen, SDL_Texture ** scrtex)
{
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    printf("SDL_Init error: %s\n", SDL_GetError());
    return 1;
  }
  *rc = SDL_CreateWindowAndRenderer(0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP,
    window, renderer);
  //*rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0,
  //  window, renderer);

  if (*rc != 0)
  {
    SDL_Quit();
    printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
    return 1;
  };

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
  SDL_RenderSetLogicalSize(*renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
  SDL_SetRenderDrawColor(*renderer, 0, 0, 0, 255);
  SDL_SetWindowTitle(*window, "Frogger");

  *screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
    0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

  *scrtex = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_ARGB8888,
    SDL_TEXTUREACCESS_STREAMING,
    SCREEN_WIDTH, SCREEN_HEIGHT);

  return 0;
}

void CloseSDL(SDL_Surface *screen, SDL_Texture *scrtex, SDL_Window *window, SDL_Renderer *renderer)
{
  SDL_FreeSurface(screen);
  SDL_DestroyTexture(scrtex);
  SDL_DestroyWindow(window);
  SDL_DestroyRenderer(renderer);
  SDL_Quit();
}

int LoadAsset(SDL_Surface ** asset, char * path)
{
  *asset = SDL_LoadBMP(path);
  if (*asset == NULL) {
    printf("SDL_LoadBMP(%s) error: %s\n",path, SDL_GetError());
    return 1;
  };
  return 0;
}

void InitializeLogs(MovingEntity logs[3][LOGS_IN_ROW], SDL_Surface * log)
{
  for (int i = 0; i < 3; i++)
  {
    for (int j = 0; j < LOGS_IN_ROW; j++)
    {
      logs[i][j].direction = -1;
      logs[i][j].velocity = 100 * (i + 1);
      logs[i][j].entity.Y = i == 0 ? CELL_SIZE + CELL_SIZE / 2 : (2 + i) * CELL_SIZE + CELL_SIZE / 2;
      logs[i][j].entity.X = (j * 5) * CELL_SIZE + SCREEN_WIDTH / 2 - log->w;

      logs[i][j].entity.Width = log->w ;
      logs[i][j].entity.Height = CELL_SIZE;
    }
  }
}

void InitializeTurtles(MovingEntity turtles[2][TURTLES_IN_ROW], SDL_Surface * turtle)
{
  for (int i = 0; i < 2; i++)
  {
    for (int j = 0; j < TURTLES_IN_ROW; j++)
    {
      turtles[i][j].direction = 1;
      turtles[i][j].velocity = 110;
      turtles[i][j].entity.Y = i == 0 ? 2 * CELL_SIZE + CELL_SIZE / 2 : 5 * CELL_SIZE + CELL_SIZE / 2;
      turtles[i][j].entity.X = (j * 8) * CELL_SIZE + SCREEN_WIDTH / 2;

      turtles[i][j].entity.Width = 82 ;
      turtles[i][j].entity.Height = CELL_SIZE;
    }
  }
}

void InitializeVehicles(MovingEntity vehicles[5], SDL_Surface * rocket_car)
{
  for (int i = 0; i < 5; i++)
  {
    vehicles[i].direction = (i % 2) == 0 ? 1 : -1;
    vehicles[i].velocity = 80 * (i + 1);
    vehicles[i].entity.Y = (7 + i) * CELL_SIZE + CELL_SIZE / 2;
    vehicles[i].entity.X = SCREEN_WIDTH / 2;

    vehicles[i].entity.Width = rocket_car->w ;
    vehicles[i].entity.Height = CELL_SIZE;
  }
}

void InitializeEndpoints(Endpoint endpoints[5])
{
  for (int i = 0; i < 5; i++)
  {
    endpoints[i].activated = 0;
    endpoints[i].entity.Y = CELL_SIZE / 2;
    endpoints[i].entity.X = SCREEN_WIDTH / 5 / 2 + i * SCREEN_WIDTH / 5;
    endpoints[i].entity.Width = endpoints[i].entity.Height = CELL_SIZE;
  }
}

void UpdateLogs(MovingEntity logs[][LOGS_IN_ROW],double delta)
{
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < LOGS_IN_ROW; j++)
      logs[i][j].entity.X += logs[i][j].velocity * delta * logs[i][j].direction;

  for (int i = 0; i < 3; i++)
    for (int j = 0; j < LOGS_IN_ROW; j++)
      if (logs[i][j].entity.X < -SCREEN_WIDTH)
        logs[i][j].entity.X = SCREEN_WIDTH + logs[i][j].entity.Width / 2;
}

void UpdateTurtles(MovingEntity turtles[][TURTLES_IN_ROW],double delta)
{
  for (int i = 0; i < 2; i++)
    for (int j = 0; j < TURTLES_IN_ROW; j++)
      turtles[i][j].entity.X += turtles[i][j].velocity * delta * turtles[i][j].direction;

  for (int i = 0; i < 2; i++)
    for (int j = 0; j < TURTLES_IN_ROW; j++)
      if (turtles[i][j].entity.X > 2 * SCREEN_WIDTH)
        turtles[i][j].entity.X = 0 - turtles[i][j].entity.Width / 2;
}

void UpdateVehicles(MovingEntity vehicles[5], double delta)
{
  for (int i = 0; i < 5; i++)
    vehicles[i].entity.X += vehicles[i].velocity * delta * vehicles[i].direction;

  for (int i = 0; i < 5; i++)
  {
    if (vehicles[i].direction == -1)
    {
      if (vehicles[i].entity.X < -SCREEN_WIDTH)
        vehicles[i].entity.X = SCREEN_WIDTH + vehicles[i].entity.Width / 2;
    }
    else
    {
      if (vehicles[i].entity.X > 2 * SCREEN_WIDTH)
        vehicles[i].entity.X = 0 - vehicles[i].entity.Width / 2;
    }
  }
}

void RenderVehicles(MovingEntity * vehicles, SDL_Surface * screen, SDL_Surface * rocket_car, SDL_Surface * space_car)
{
  for (int i = 0; i < 5; i++)
    DrawSurface(screen, vehicles[i].direction == 1 ? rocket_car : space_car,
      vehicles[i].entity.X,
      vehicles[i].entity.Y);
}

void RenderEnvironment(SDL_Surface * screen, SDL_Surface * road, SDL_Surface * brick)
{
  DrawSurface(screen, road,
    SCREEN_WIDTH / 2,
    CELL_SIZE / 2 + 9 * CELL_SIZE);

  for (int i = 0; i < 14; i++)
    DrawSurface(screen, brick,
      CELL_SIZE / 2 + i * CELL_SIZE,
      CELL_SIZE / 2 + 6 * CELL_SIZE);

  for (int i = 0; i < 14; i++)
    DrawSurface(screen, brick,
      CELL_SIZE / 2 + i * CELL_SIZE,
      CELL_SIZE / 2 + 12 * CELL_SIZE);
}

void RenderEndpoints(Endpoint * endpoints, SDL_Surface * screen, SDL_Surface * froggo, SDL_Surface * brick)
{
  for (int i = 0; i < 5; i++)
  {
    if (endpoints[i].activated)
    {
      DrawSurface(screen, froggo,
        endpoints[i].entity.X,
        endpoints[i].entity.Y);
    }
    else
    {
      DrawSurface(screen, brick,
        endpoints[i].entity.X,
        endpoints[i].entity.Y);
    }
  }
}

void RenderTurtles(MovingEntity turtles[][TURTLES_IN_ROW], SDL_Surface * screen, SDL_Surface * turtle)
{
  for (int i = 0; i < 2; i++)
    for (int j = 0; j < TURTLES_IN_ROW; j++)
    {
      DrawSurface(screen, turtle,
        turtles[i][j].entity.X,
        turtles[i][j].entity.Y);
    }

}

void RenderLogs(MovingEntity logs[][LOGS_IN_ROW], SDL_Surface * screen, SDL_Surface * log)
{
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < LOGS_IN_ROW; j++)
    {
      DrawSurface(screen, log,
        logs[i][j].entity.X,
        logs[i][j].entity.Y);
    }
}

#ifdef __cplusplus
extern "C"
#endif

int main(int argc, char **argv) {
  int t1, t2, quit, frames, rc;
  double delta, worldTime, fpsTimer, fps, distance, etiSpeed;
  SDL_Event event;
  SDL_Surface *screen, *charset;
  SDL_Surface *eti;
  SDL_Surface *river, *road, *brick, *frogger, *froggo;
  SDL_Surface *turtle;
  SDL_Surface *froggger;
  SDL_Surface *log;
  SDL_Surface *space_car, *rocket_car;
  SDL_Texture *scrtex;
  SDL_Window *window;
  SDL_Renderer *renderer;

  int initUnsuccessful = InitializeSDL(&rc, &window, &renderer, &screen, &scrtex);
  if (initUnsuccessful)
    return 1;

  int loadingAssetsUnsuccessful = 0;
  loadingAssetsUnsuccessful = LoadAsset(&charset, "cs8x8.bmp");
  loadingAssetsUnsuccessful = LoadAsset(&froggo, "froggie.bmp");
  loadingAssetsUnsuccessful = LoadAsset(&brick, "brick1.bmp");
  loadingAssetsUnsuccessful = LoadAsset(&frogger, "frogger.bmp");
  loadingAssetsUnsuccessful = LoadAsset(&space_car, "space_car.bmp");
  loadingAssetsUnsuccessful = LoadAsset(&rocket_car, "rocket_car.bmp");
  loadingAssetsUnsuccessful = LoadAsset(&log, "log_medium.bmp");
  loadingAssetsUnsuccessful = LoadAsset(&turtle, "double_turtles.bmp");
  loadingAssetsUnsuccessful = LoadAsset(&road, "road.bmp");
  loadingAssetsUnsuccessful = LoadAsset(&froggo, "froggie.bmp");

  if (loadingAssetsUnsuccessful)
  {
    CloseSDL(screen, scrtex, window, renderer);
    return 1;
  }

  SDL_SetColorKey(charset, true, 0x000000);

  char text[128];
  int black = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
  int blue = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);

  t1 = SDL_GetTicks();

  frames = 0;
  fpsTimer = 0;
  fps = 0;
  quit = 0;
  worldTime = 0;
  distance = 0;
  etiSpeed = 1;

  MovingEntity vehicles[5];
  InitializeVehicles(vehicles,rocket_car);

  MovingEntity logs[3][LOGS_IN_ROW];
  InitializeLogs(logs, log);

  MovingEntity turtles[2][3];
  InitializeTurtles(turtles, turtle);

  Endpoint endpoints[5];
  InitializeEndpoints(endpoints);

  Entity player;
  player.X = 7 * CELL_SIZE + CELL_SIZE / 2;
  player.Y = 12 * CELL_SIZE + CELL_SIZE / 2;
  player.Width = player.Height = CELL_SIZE;

  while (!quit) {
    t2 = SDL_GetTicks();

    delta = (t2 - t1) * 0.001;
    t1 = t2;

    worldTime += delta;

    distance += etiSpeed * delta;
    fpsTimer += delta;
    if (fpsTimer > 0.5) {
      fps = frames * 2;
      frames = 0;
      fpsTimer -= 0.5;
    }

    UpdateVehicles(vehicles, delta);
    UpdateTurtles(turtles, delta);
    UpdateLogs(logs, delta);

    //Check if frog intersects with the space_car
    for (int i = 0; i < 5; i++)
      if (Intersects(player, vehicles[i].entity))
        quit = 1;

    //Check if frog intersects with Log/Turtle/Endpoint
    if (player.Y > 0 && player.Y < CELL_SIZE * 6)
    {
      bool intersects = false;
      for (int i = 0; i < 3; i++)
      {
        for (int j = 0; j < LOGS_IN_ROW; j++)
        {
          if (Intersects(player, logs[i][j].entity))
          {
            intersects = true;
            player.X += logs[i][j].velocity * delta * logs[i][j].direction;
          }
        }
        
      }
      for (int i = 0; i < 2; i++)
      {
        for (int j = 0; j < TURTLES_IN_ROW; j++)
        {
          if (Intersects(player, turtles[i][j].entity))
          {
            intersects = true;
            player.X += turtles[i][j].velocity * delta * turtles[i][j].direction;
          }
        }
      }
      for (int i = 0; i < 5; i++)
      {
        if (Intersects(player, endpoints[i].entity))
        {
          intersects = true;
          if (endpoints[i].activated)
          {
            quit = 1;
          }
          else
          {
            endpoints[i].activated = 1;

            //respawn player
            player.X = 7 * CELL_SIZE + CELL_SIZE / 2;
            player.Y = 12 * CELL_SIZE + CELL_SIZE / 2;
          }
        }
      }
      if (!intersects)
        quit = 1;
    }


    //Check if Frog is out of Screen
    if (player.X < -CELL_SIZE || player.X > SCREEN_WIDTH + CELL_SIZE)
      quit = 1;

    SDL_FillRect(screen, NULL, blue);
 
    RenderEnvironment(screen, road, brick);
    RenderEndpoints(endpoints, screen, froggo, brick);
    RenderLogs(logs, screen, log);
    RenderTurtles(turtles, screen, turtle);

    DrawSurface(screen, frogger,
      player.X,
      player.Y);

    RenderVehicles(vehicles, screen, rocket_car, space_car);


    DrawRectangle(screen, 4, SCREEN_HEIGHT - BOX_OFFSET, SCREEN_WIDTH - 8, 18, black, black);
    sprintf(text, "Frogger, time in game: %.1lf s  %.0lf fps", worldTime, fps);
    DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_HEIGHT - BOX_OFFSET + 5, text, charset);
    SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
    SDL_RenderClear(renderer);
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
          if (player.Y - CELL_SIZE >= 0)
            player.Y -= CELL_SIZE;
        }
        else if (event.key.keysym.sym == SDLK_DOWN)
        {
          if (player.Y + CELL_SIZE <= CELL_SIZE * 13)
            player.Y += CELL_SIZE;
        }
        else if (event.key.keysym.sym == SDLK_LEFT)
        {
          if (player.X - CELL_SIZE >= 0)
            player.X -= CELL_SIZE;
        }
        else if (event.key.keysym.sym == SDLK_RIGHT)
        {
          if (player.X + CELL_SIZE <= CELL_SIZE * 14)
            player.X += CELL_SIZE;
        }
        break;
      case SDL_QUIT:
        quit = 1;
        break;
      }
    }
    frames++;
  }
  

  // zwolnienie powierzchni / freeing all surfaces
  SDL_FreeSurface(charset);
  CloseSDL(screen, scrtex, window, renderer);

  return 0;
};