#include <raylib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* RayLib BMI Calculator */

// Convert a macro to a string (stringification)
#define STR(x) #x
#define XSTR(x) STR(x)

// Definitions

#define SCREEN_WIDTH 380
#define SCREEN_HEIGHT 270
#define APP_TITLE "BMI Calculator (Raylib)"
#define APP_ICON "icon.png"
#define APP_BACKGROUND_COLOR \
  (Color) {                  \
    230, 230, 230, 255       \
  }
#define TARGET_FPS 30

#define FONT_COUNT 2
#define FONT_PATHS                                                     \
  {                                                                    \
    "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", \
        "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf" \
  }
#define FONT_SIZES \
  { 32, 32 }

#define MAX_WIDGETS 9

#define MAX_WEIGHT 200
#define MIN_WEIGHT 30
#define DFT_WEIGHT 70
#define MAX_HEIGHT 220
#define MIN_HEIGHT 120
#define DFT_HEIGHT 170

/* ----------------------------------------------------------------*/

static void click(size_t id);
static void print(size_t id);
static void check(size_t id);
static void calcBMI(void);
static void int2string(const int i, char* str);
static void string2int(const char* str, int* i);

static void Startup(void);
static void Update(void);
static void Render(void);
static void Shutdown(void);
static void AddCodepointRange(Font* font,
                              const char* fontPath,
                              int start,
                              int stop);
static void paintRectangle(Vector2 pos,
                           Vector2 dim,
                           Color bgcolor,
                           bool hover,
                           bool sunken,
                           bool active);
static void paintTextButton(const char* text,
                            Vector2 pos,
                            Vector2 dim,
                            Font font,
                            Color textcolor,
                            bool hover);
static size_t u32_to_utf8(const uint32_t cp, char out[5]);
static void remove_last_utf8_char(char* str);
static size_t utf8_strlen(const char* s);
static void set_input_cursor(bool yes, char* text);
static void process_keys(void);
static void stoptyping(void);

int main(void);

/* ----------------------------------------------------------------*/

Font font[FONT_COUNT];
Vector2 mousepos;
bool mousepressed;
bool exitstate = false;
int typing_widget = -1;

typedef enum {
  NONE = 0,
  LABEL,
  BUTTON,
  IMAGE,
  CROSSBUTTON,
  PANEL,
  INPUT
} WidgetType;

typedef void (*callback_print_func)(size_t widget_index);
typedef void (*callback_event_func)(size_t widget_index);
typedef void (*callback_check_func)(size_t widget_index);

typedef struct {
  WidgetType type;
  char filename[128];                  // IMAGE
  Texture2D texture;                   // IMAGE
  char text[256];                      // LABEL, BUTTON, INPUT
  size_t textmaxcount;                 // INPUT
  Vector2 position;                    // all
  Color backgroundcolor;               // LABEL, PANEL, BUTTON
  Color textcolor;                     // LABEL, BUTTON, CROSSBUTTON, INPUT
  Vector2 size;                        // BUTTON, PANEL
  size_t fontindex;                    // LABEL, BUTTON, CROSSBUTTON, TEXT
  bool sunken;                         // PANEL
  bool mouse_on_widget;                // all
  bool typing;                         // INPUT
  char allowed[256];                   // INPUT
  callback_print_func print_event_fn;  // LABEL, BUTTON
  callback_event_func click_event_fn;  // LABEL, BUTTON, IMAGE, CROSSBUTTON
  callback_check_func check_event_fn;  // INPUT
} WidgetData;

/* ----------------------------------------------------------------*/

// Widget Definitions

WidgetData widgets[MAX_WIDGETS] = {
    {
        // 0
        .type = LABEL,
        .text = "Weight [kg]:",
        .backgroundcolor = BLANK,
        .position = {50, 50},
        .textcolor = BLACK,
        .fontindex = 0,
    },
    {
        // 1
        .type = LABEL,
        .text = "Height [cm]:",
        .backgroundcolor = BLANK,
        .position = {50, 120},
        .textcolor = BLACK,
        .fontindex = 0,
    },
    {
        // 2
        .type = INPUT,
        .position = {220, 50},
        .text = XSTR(DFT_WEIGHT),
        .textmaxcount = 3,
        .fontindex = 0,
        .textcolor = BLACK,
        .allowed = "0123456789",
        .check_event_fn = check,
    },
    {
        // 3
        .type = INPUT,
        .position = {220, 120},
        .text = XSTR(DFT_HEIGHT),
        .textmaxcount = 3,
        .fontindex = 0,
        .textcolor = BLACK,
        .allowed = "0123456789",
        .check_event_fn = check,
    },
    {
        // 4
        .type = BUTTON,
        .position = {300, 42},
        .size = {25, 25},
        .text = "▲",
        .textcolor = BLACK,
        .fontindex = 0,
        .click_event_fn = click,
    },
    {
        // 5
        .type = BUTTON,
        .position = {300, 71},
        .size = {25, 25},
        .text = "▼",
        .textcolor = BLACK,
        .fontindex = 0,
        .click_event_fn = click,
    },
    {
        // 6
        .type = BUTTON,
        .position = {300, 112},
        .size = {25, 25},
        .text = "▲",
        .textcolor = BLACK,
        .fontindex = 0,
        .click_event_fn = click,
    },
    {
        // 7
        .type = BUTTON,
        .position = {300, 141},
        .size = {25, 25},
        .text = "▼",
        .textcolor = BLACK,
        .fontindex = 0,
        .click_event_fn = click,
    },
    {
        // 8
        .type = LABEL,
        .text = "",
        .backgroundcolor = BLANK,
        .position = {85, 200},
        .textcolor = BLACK,
        .fontindex = 1,
        .print_event_fn = print,
    },
};

// Application Data

int weight = DFT_WEIGHT;
int height = DFT_HEIGHT;
char BMI[25] = "";

// Callback functions

static void click(size_t id) {
  switch (id) {
    case 4:
      weight++;
      if (weight > MAX_WEIGHT)
        weight = MAX_WEIGHT;
      int2string(weight, widgets[2].text);
      break;
    case 5:
      weight--;
      if (weight < MIN_WEIGHT)
        weight = MIN_WEIGHT;
      int2string(weight, widgets[2].text);
      break;
    case 6:
      height++;
      if (height > MAX_HEIGHT)
        height = MAX_HEIGHT;
      int2string(height, widgets[3].text);
      break;
    case 7:
      height--;
      if (height < MIN_HEIGHT)
        height = MIN_HEIGHT;
      int2string(height, widgets[3].text);
      break;
    default:
      break;
  }
}

static void print(size_t id) {
  if (id == 8)
    calcBMI();
}

static void check(size_t id) {
  if (id == 2) {
    string2int(widgets[id].text, &weight);
    if (weight == 0)
      weight = DFT_WEIGHT;
    if (weight > MAX_WEIGHT)
      weight = MAX_WEIGHT;
    if (weight < MIN_WEIGHT)
      weight = MIN_WEIGHT;
    int2string(weight, widgets[id].text);
  } else if (id == 3) {
    string2int(widgets[id].text, &height);
    if (height == 0)
      height = DFT_HEIGHT;
    if (height > MAX_HEIGHT)
      height = MAX_HEIGHT;
    if (height < MIN_HEIGHT)
      height = MIN_HEIGHT;
    int2string(height, widgets[id].text);
  }
}

// Utilities

static void calcBMI(void) {
  sprintf(BMI, "BMI = %.1f kg/m²",
          (double)weight * 1e4 / ((double)height * (double)height));
  strcpy(widgets[8].text, BMI);
}

static void int2string(const int i, char* str) {
  sprintf(str, "%d", i);
}

static void string2int(const char* str, int* i) {
  *i = atoi(str);
}

/* ----------------------------------------------------------------*/

static void Startup(void) {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, APP_TITLE);
  // Not resizable
  ClearWindowState(FLAG_WINDOW_RESIZABLE);
  SetTargetFPS(TARGET_FPS);
  // Icon
  Image img = LoadImage(APP_ICON);
  SetWindowIcon(img);
  // Center Window
  int monitor = GetCurrentMonitor();
  int monitorWidth = GetMonitorWidth(monitor);
  int monitorHeight = GetMonitorHeight(monitor);
  SetWindowPosition(monitorWidth / 2 - SCREEN_WIDTH / 2,
                    monitorHeight / 2 - SCREEN_HEIGHT / 2);
  // Fonts
  int fontsizes[FONT_COUNT] = FONT_SIZES;
  char* fontpaths[FONT_COUNT] = FONT_PATHS;
  for (size_t i = 0; i < FONT_COUNT; i++) {
    font[i] = LoadFontEx(fontpaths[i], fontsizes[i], NULL, 0);
    AddCodepointRange(&font[i], fontpaths[i], 0x00b2, 0x00b2);  // = ²
    AddCodepointRange(&font[i], fontpaths[i], 0x25b2, 0x25b2);  // = ▲
    AddCodepointRange(&font[i], fontpaths[i], 0x25bc, 0x25bc);  // = ▼
    SetTextureFilter(font[i].texture, TEXTURE_FILTER_BILINEAR);
  }
  SetTextLineSpacing(0);
  // Widgets
  for (size_t i = 0; i < MAX_WIDGETS; i++) {
    switch (widgets[i].type) {
      case IMAGE:
        widgets[i].texture = LoadTexture(widgets[i].filename);
        widgets[i].size =
            (Vector2){widgets[i].texture.width, widgets[i].texture.height};
        break;
      case LABEL:
        widgets[i].size =
            MeasureTextEx(font[widgets[i].fontindex], widgets[i].text,
                          font[widgets[i].fontindex].baseSize,
                          font[widgets[i].fontindex].baseSize / 24);
        break;
      case CROSSBUTTON:
        widgets[i].size =
            MeasureTextEx(font[widgets[i].fontindex], " x ",
                          font[widgets[i].fontindex].baseSize,
                          font[widgets[i].fontindex].baseSize / 24);
        widgets[i].size =
            (Vector2){widgets[i].size.x + 1, widgets[i].size.y + 1};
        break;
      case INPUT:
        widgets[i].size =
            MeasureTextEx(font[widgets[i].fontindex], "M",
                          font[widgets[i].fontindex].baseSize,
                          font[widgets[i].fontindex].baseSize / 24);
        widgets[i].size =
            (Vector2){widgets[i].size.x * widgets[i].textmaxcount + 5,
                      widgets[i].size.y + 5};
        break;
      default:
        break;
    }
  }
}

static void Update(void) {
  mousepos = GetMousePosition();
  bool mousereleased = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
  mousepressed = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
  if (mousereleased)
    stoptyping();
  if (typing_widget >= 0)
    process_keys();
  for (size_t i = 0; i < MAX_WIDGETS; i++)
    widgets[i].mouse_on_widget = false;
  bool notfound = true;
  for (size_t i = 0; i < MAX_WIDGETS && notfound; i++) {
    switch (widgets[i].type) {
      case LABEL:
      case CROSSBUTTON:
      case IMAGE:
      case BUTTON:
        if (CheckCollisionPointRec(
                mousepos,
                (Rectangle){widgets[i].position.x, widgets[i].position.y,
                            widgets[i].size.x, widgets[i].size.y})) {
          if (mousereleased && widgets[i].click_event_fn)
            widgets[i].click_event_fn(i);
          widgets[i].mouse_on_widget = true;
          notfound = false;
        }
        break;
      case INPUT:
        if (CheckCollisionPointRec(
                mousepos,
                (Rectangle){widgets[i].position.x, widgets[i].position.y,
                            widgets[i].size.x, widgets[i].size.y})) {
          if (mousereleased) {
            typing_widget = i;
            widgets[i].typing = true;
            set_input_cursor(true, widgets[i].text);
          }
        }
      default:
        break;
    }
  }
}

static void Render(void) {
  ClearBackground(APP_BACKGROUND_COLOR);
  // DrawFPS(1, 1);
  for (size_t i = 0; i < MAX_WIDGETS; i++) {
    switch (widgets[i].type) {
      case PANEL:
        paintRectangle(widgets[i].position, widgets[i].size,
                       widgets[i].backgroundcolor, false, widgets[i].sunken,
                       false);
        break;
      case IMAGE:
        DrawTexture(widgets[i].texture, widgets[i].position.x,
                    widgets[i].position.y, WHITE);
        break;
      case BUTTON:
        if (widgets[i].print_event_fn)
          widgets[i].print_event_fn(i);
        paintTextButton(widgets[i].text, widgets[i].position, widgets[i].size,
                        font[widgets[i].fontindex], widgets[i].textcolor,
                        widgets[i].mouse_on_widget);
        break;
      case CROSSBUTTON:
        paintTextButton(" x ", widgets[i].position, widgets[i].size,
                        font[widgets[i].fontindex], widgets[i].textcolor,
                        widgets[i].mouse_on_widget);
        break;
      case LABEL:
        if (widgets[i].print_event_fn)
          widgets[i].print_event_fn(i);
        DrawRectangleV(widgets[i].position, widgets[i].size,
                       widgets[i].backgroundcolor);
        DrawTextEx(font[widgets[i].fontindex], widgets[i].text,
                   widgets[i].position, font[widgets[i].fontindex].baseSize,
                   font[widgets[i].fontindex].baseSize / 24,
                   widgets[i].textcolor);
        break;
      case INPUT:
        paintRectangle(widgets[i].position, widgets[i].size, BLANK, false, true,
                       widgets[i].typing);
        DrawTextEx(
            font[widgets[i].fontindex], widgets[i].text,
            (Vector2){widgets[i].position.x + 3, widgets[i].position.y + 3},
            font[widgets[i].fontindex].baseSize,
            font[widgets[i].fontindex].baseSize / 24, widgets[i].textcolor);
      default:
        break;
    }
  }
}

static void Shutdown(void) {
  for (size_t i = 0; i < MAX_WIDGETS; i++) {
    if (widgets[i].type == IMAGE) {
      UnloadTexture(widgets[i].texture);
    }
  }
  for (size_t i = 0; i < FONT_COUNT; i++)
    UnloadFont(font[i]);
  CloseWindow();
}

static void AddCodepointRange(Font* font,
                              const char* fontPath,
                              int start,
                              int stop) {
  int rangeSize = stop - start + 1;
  int currentRangeSize = font->glyphCount;
  int updatedCodepointCount = currentRangeSize + rangeSize;
  int* updatedCodepoints = (int*)calloc(updatedCodepointCount, sizeof(int));
  for (int i = 0; i < currentRangeSize; i++)
    updatedCodepoints[i] = font->glyphs[i].value;
  for (int i = currentRangeSize; i < updatedCodepointCount; i++)
    updatedCodepoints[i] = start + (i - currentRangeSize);
  UnloadFont(*font);
  *font = LoadFontEx(fontPath, font->baseSize, updatedCodepoints,
                     updatedCodepointCount);
  free(updatedCodepoints);
}

static void paintRectangle(Vector2 pos,
                           Vector2 dim,
                           Color bgcolor,
                           bool hover,
                           bool sunken,
                           bool active) {
  Color color_shadow = (Color){0, 0, 0, 255};
  Color color_light = (Color){255, 255, 255, 255};
  Color color_shadow_active = DARKBLUE;  // (Color){20, 20, 20, 255};
  Color color_light_active = BLUE;       // (Color){253, 249, 0, 255};
  Color color_normal = (Color){200, 200, 200, 255};
  Color color_hover = (Color){190, 190, 190, 255};
  Color color_sunken = (Color){175, 175, 175, 255};
  Color c1, c2, c3, csave;
  if (bgcolor.r == 0 && bgcolor.g == 0 && bgcolor.b == 0 && bgcolor.a == 0) {
    c1 = color_normal;
    if (hover)
      c1 = color_hover;
    if (sunken)
      c1 = color_sunken;
  } else
    c1 = bgcolor;
  c2 = color_light;
  c3 = color_shadow;
  if (active) {
    c2 = color_light_active;
    c3 = color_shadow_active;
  }
  if (sunken) {
    csave = c2;
    c2 = c3;
    c3 = csave;
  }
  DrawRectangleV(pos, dim, c1);
  DrawLineEx(pos, (Vector2){pos.x + dim.x + 1, pos.y}, 2, c2);
  DrawLineEx(pos, (Vector2){pos.x, pos.y + dim.y + 1}, 2, c2);
  DrawLineEx((Vector2){pos.x + 1, pos.y + dim.y},
             (Vector2){pos.x + dim.x + 1, pos.y + dim.y}, 2, c3);
  DrawLineEx((Vector2){pos.x + dim.x, pos.y + 1},
             (Vector2){pos.x + dim.x, pos.y + dim.y + 1}, 2, c3);
}

static void paintTextButton(const char* text,
                            Vector2 pos,
                            Vector2 dim,
                            Font font,
                            Color textcolor,
                            bool hover) {
  float delta = 0.0f;
  Vector2 textdim =
      MeasureTextEx(font, text, font.baseSize, font.baseSize / 24);
  if (hover && mousepressed)
    delta = -2.0f;
  paintRectangle(pos, dim, BLANK, hover, hover && mousepressed, false);
  DrawTextEx(font, text,
             (Vector2){pos.x + dim.x / 2 - textdim.x / 2 + delta,
                       pos.y + dim.y / 2 - textdim.y / 2 + delta},
             font.baseSize, font.baseSize / 24, textcolor);
}

static size_t u32_to_utf8(const uint32_t cp, char out[5]) {
  for (int i = 0; i < 5; i++)
    out[i] = '\0';
  if (cp <= 0x7F) {
    out[0] = (unsigned char)cp;
    return 1;
  }
  if (cp <= 0x7FF) {
    out[0] = 0xC0 | (cp >> 6);
    out[1] = 0x80 | (cp & 0x3F);
    return 2;
  }
  if (cp >= 0xD800 && cp <= 0xDFFF) {
    return 0;  // invalid (surrogate)
  }
  if (cp <= 0xFFFF) {
    out[0] = 0xE0 | (cp >> 12);
    out[1] = 0x80 | ((cp >> 6) & 0x3F);
    out[2] = 0x80 | (cp & 0x3F);
    return 3;
  }
  if (cp <= 0x10FFFF) {
    out[0] = 0xF0 | (cp >> 18);
    out[1] = 0x80 | ((cp >> 12) & 0x3F);
    out[2] = 0x80 | ((cp >> 6) & 0x3F);
    out[3] = 0x80 | (cp & 0x3F);
    return 4;
  }
  return 0;  // invalid (out of Unicode range)
}

static void remove_last_utf8_char(char* str) {
  if (!str || *str == '\0')
    return;
  char* p = str + strlen(str) - 1;
  while (p > str && ((*p & 0xC0) == 0x80)) {
    p--;
  }
  *p = '\0';
}

static size_t utf8_strlen(const char* s) {
  size_t len = 0;
  while (*s) {
    if ((*s & 0xC0) != 0x80) {
      len++;
    }
    s++;
  }
  return len;
}

static void set_input_cursor(bool yes, char* text) {
  int len = strlen(text);
  if (yes)
    text[len] = '|';
  else
    len = len - 2;
  text[len + 1] = '\0';
}

static void process_keys(void) {
  int key;
  char buf[5];
  while ((key = GetKeyPressed()) != 0) {
    if (KEY_BACKSPACE == key) {
      // printf("BACKSPACE %s\n",widgets[typing_widget].text);
      set_input_cursor(false, widgets[typing_widget].text);
      remove_last_utf8_char(widgets[typing_widget].text);
      set_input_cursor(true, widgets[typing_widget].text);
    }
  }
  while ((key = GetCharPressed()) != 0) {
    int l = u32_to_utf8(key, buf);
    if (l > 0 && (strlen(widgets[typing_widget].allowed) == 0 ||
                  strstr(widgets[typing_widget].allowed, buf) != NULL)) {
      set_input_cursor(false, widgets[typing_widget].text);
      if (utf8_strlen(widgets[typing_widget].text) <
          widgets[typing_widget].textmaxcount) {
        strcat(widgets[typing_widget].text, buf);
      }
      set_input_cursor(true, widgets[typing_widget].text);
    }
  }
}

static void stoptyping(void) {
  if (typing_widget >= 0) {
    set_input_cursor(false, widgets[typing_widget].text);
    if (widgets[typing_widget].check_event_fn) {
      widgets[typing_widget].check_event_fn(typing_widget);
    }
  }
  typing_widget = -1;
  for (size_t j = 0; j < MAX_WIDGETS; j++)
    widgets[j].typing = false;
}

/* ----------------------------------------------------------------*/

int main(void) {
  Startup();
  while (!WindowShouldClose() && !exitstate) {
    Update();
    BeginDrawing();
    Render();
    EndDrawing();
  }
  Shutdown();
  return 0;
}
