#ifndef MYRAYGUI_H
#define MYRAYGUI_H

/* MyRayGUI Version 2026-02-18 */

#include <inttypes.h>
#include <raylib.h>
#include <stdlib.h>
#include <string.h>

/* ----------------------------------------------------------------*/

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
                            bool hover,
                            float spacing);
static void paintScrollbar(int id);
static int u32_to_utf8(const uint32_t cp, char out[5]);
static void remove_last_utf8_char(char* str);
static int utf8_strlen(const char* s);
static void set_input_cursor(bool yes, char* text);
static void process_keys(void);
static void stoptyping(void);
static void init_longtext(int id, char* text);
static char* wrap_text_words(const char* text, int width);

/* ----------------------------------------------------------------*/

static Font font[FONT_COUNT];
static float fontspacings[FONT_COUNT];
static Vector2 mousepos;
static bool mousepressed;
static bool exitstate = false;
static int typing_widget = -1;
static int mouse_cursor = MOUSE_CURSOR_DEFAULT;

typedef enum {
  NONE = 0,
  LABEL,
  BUTTON,
  IMAGE,
  CROSSBUTTON,
  PANEL,
  INPUT,
  TEXTSCROLLAREA,
  CLICKAREA,
} WidgetType;

typedef void (*callback_init_func)(int widget_index);
typedef void (*callback_print_func)(int widget_index);
typedef void (*callback_click_func)(int widget_index);
typedef void (*callback_check_func)(int widget_index);

typedef struct WidgetData {
  WidgetType type;
  char filename[128];     // IMAGE
  Texture2D texture;      // IMAGE
  char text[256];         // LABEL, BUTTON, INPUT
  int textmaxcount;       // INPUT
  Vector2 position;       // all
  Color backgroundcolor;  // LABEL, PANEL, BUTTON
  Color textcolor;        // LABEL, BUTTON, CROSSBUTTON, INPUT
  Vector2 size;           // BUTTON, PANEL, TEXTSCROLLAREA, CLICKAREA
  int fontindex;          // LABEL, BUTTON, CROSSBUTTON, TEXT
  bool sunken;            // PANEL
  bool mouse_on_widget;   // all
  bool typing;            // INPUT
  char allowed[256];      // INPUT
  char* longtext;         // TEXTSCROLLAREA (via init / init_longtext)
  float yscrollpos;       // TEXTSCROLLAREA
  float ytextmax;         // TEXTSCROLLAREA
  callback_init_func init_event_fn;    // TEXTSCROLLAREA
  callback_print_func print_event_fn;  // LABEL, BUTTON
  callback_click_func
      click_event_fn;  // LABEL, BUTTON, IMAGE, CROSSBUTTON, CLICKAREA
  callback_check_func check_event_fn;  // INPUT
} WidgetData;

#endif  // MYRAYGUI_H

#if defined(MYRAYGUI_IMPLEMENTATION)

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
  float fontspacings[FONT_COUNT] = FONT_SPACINGS;
  char* fontpaths[FONT_COUNT] = FONT_PATHS;
  int fontaddcpranges[FONT_ADD_CP_RANGES_COUNT][2] = FONT_ADD_CP_RANGES;
  for (int i = 0; i < FONT_COUNT; i++) {
    font[i] = LoadFontEx(fontpaths[i], fontsizes[i], NULL, 0);
    for (int j = 0; j < FONT_ADD_CP_RANGES_COUNT; j++) {
      AddCodepointRange(&font[i], fontpaths[i], fontaddcpranges[j][0],
                        fontaddcpranges[j][1]);
    }
    SetTextureFilter(font[i].texture, TEXTURE_FILTER_BILINEAR);
  }
  SetTextLineSpacing(TEXT_LINE_SPACING);
  // Widgets
  for (int i = 0; i < MAX_WIDGETS; i++) {
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
                          fontspacings[widgets[i].fontindex]);
        break;
      case CROSSBUTTON:
        widgets[i].size = MeasureTextEx(font[widgets[i].fontindex], " x ",
                                        font[widgets[i].fontindex].baseSize,
                                        fontspacings[widgets[i].fontindex]);
        widgets[i].size =
            (Vector2){widgets[i].size.x + 1, widgets[i].size.y + 1};
        break;
      case TEXTSCROLLAREA:
        if (widgets[i].init_event_fn)
          widgets[i].init_event_fn(i);
        else {
          init_longtext(i, "  ");
        }
        break;
      case INPUT:
        widgets[i].size = MeasureTextEx(font[widgets[i].fontindex], "M",
                                        font[widgets[i].fontindex].baseSize,
                                        fontspacings[widgets[i].fontindex]);
        widgets[i].size =
            (Vector2){widgets[i].size.x * widgets[i].textmaxcount + 5,
                      widgets[i].size.y + 5};
        break;
      default:
        if (widgets[i].init_event_fn)
          widgets[i].init_event_fn(i);
        break;
    }
  }
}

static void Update(void) {
  mousepos = GetMousePosition();
  bool mouseleftreleased = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
  bool mouserightreleased = IsMouseButtonReleased(MOUSE_BUTTON_RIGHT);
  mousepressed = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
  if (mouseleftreleased)
    stoptyping();
  if (typing_widget >= 0)
    process_keys();
  for (int i = 0; i < MAX_WIDGETS; i++)
    widgets[i].mouse_on_widget = false;
  mouse_cursor = MOUSE_CURSOR_DEFAULT;
  bool notfound = true;
  for (int i = 0; i < MAX_WIDGETS && notfound; i++) {
    switch (widgets[i].type) {
      case LABEL:
      case CROSSBUTTON:
      case IMAGE:
      case BUTTON:
      case CLICKAREA:
        if (CheckCollisionPointRec(
                mousepos,
                (Rectangle){widgets[i].position.x, widgets[i].position.y,
                            widgets[i].size.x, widgets[i].size.y})) {
          if (widgets[i].click_event_fn)
            mouse_cursor = MOUSE_CURSOR_POINTING_HAND;
          if (mouseleftreleased && widgets[i].click_event_fn)
            widgets[i].click_event_fn(i);
          widgets[i].mouse_on_widget = true;
          notfound = false;
        }
        break;
      case TEXTSCROLLAREA:
        if (widgets[i].ytextmax >= widgets[i].size.y) {
          if (CheckCollisionPointRec(
                  mousepos,
                  (Rectangle){widgets[i].position.x, widgets[i].position.y,
                              widgets[i].size.x, widgets[i].size.y})) {
            if (mouseleftreleased)
              widgets[i].yscrollpos -= widgets[i].size.y;
            if (mouserightreleased)
              widgets[i].yscrollpos += widgets[i].size.y;
            widgets[i].yscrollpos += GetMouseWheelMove() * 20;
            if (widgets[i].yscrollpos > 0.f)
              widgets[i].yscrollpos = 0.f;
            if (widgets[i].yscrollpos <
                -widgets[i].ytextmax + widgets[i].size.y * .95)
              widgets[i].yscrollpos =
                  -widgets[i].ytextmax + widgets[i].size.y * .95;
          }
        }
        break;
      case INPUT:
        if (CheckCollisionPointRec(
                mousepos,
                (Rectangle){widgets[i].position.x, widgets[i].position.y,
                            widgets[i].size.x, widgets[i].size.y})) {
          if (mouseleftreleased) {
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
  SetMouseCursor(mouse_cursor);
  // DrawFPS(1, 1);
  for (int i = 0; i < MAX_WIDGETS; i++) {
    switch (widgets[i].type) {
      case PANEL:
        paintRectangle(widgets[i].position, widgets[i].size,
                       widgets[i].backgroundcolor, false, widgets[i].sunken,
                       false);
        break;
      case TEXTSCROLLAREA:
        paintRectangle(widgets[i].position, widgets[i].size,
                       widgets[i].backgroundcolor, false, widgets[i].sunken,
                       false);
        BeginScissorMode(widgets[i].position.x + 5, widgets[i].position.y + 5,
                         widgets[i].size.x - 10, widgets[i].size.y - 10);
        DrawTextEx(font[widgets[i].fontindex], widgets[i].longtext,
                   (Vector2){widgets[i].position.x + 5,
                             widgets[i].position.y + 5 + widgets[i].yscrollpos},
                   font[widgets[i].fontindex].baseSize,
                   fontspacings[widgets[i].fontindex], widgets[i].textcolor);
        EndScissorMode();
        if (widgets[i].ytextmax >= widgets[i].size.y)
          paintScrollbar(i);
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
                        widgets[i].mouse_on_widget,
                        fontspacings[widgets[i].fontindex]);
        break;
      case CROSSBUTTON:
        paintTextButton(" x ", widgets[i].position, widgets[i].size,
                        font[widgets[i].fontindex], widgets[i].textcolor,
                        widgets[i].mouse_on_widget,
                        fontspacings[widgets[i].fontindex]);
        break;
      case LABEL:
        if (widgets[i].print_event_fn)
          widgets[i].print_event_fn(i);
        DrawRectangleV(widgets[i].position, widgets[i].size,
                       widgets[i].backgroundcolor);
        DrawTextEx(font[widgets[i].fontindex], widgets[i].text,
                   widgets[i].position, font[widgets[i].fontindex].baseSize,
                   fontspacings[widgets[i].fontindex], widgets[i].textcolor);
        break;
      case INPUT:
        paintRectangle(widgets[i].position, widgets[i].size, BLANK, false, true,
                       widgets[i].typing);
        DrawTextEx(
            font[widgets[i].fontindex], widgets[i].text,
            (Vector2){widgets[i].position.x + 3, widgets[i].position.y + 3},
            font[widgets[i].fontindex].baseSize,
            fontspacings[widgets[i].fontindex], widgets[i].textcolor);
      default:
        break;
    }
  }
}

static void Shutdown(void) {
  for (int i = 0; i < MAX_WIDGETS; i++) {
    switch (widgets[i].type) {
      case IMAGE:
        UnloadTexture(widgets[i].texture);
        break;
      case TEXTSCROLLAREA:
        free(widgets[i].longtext);
        break;
      default:
        break;
    }
  }
  for (int i = 0; i < FONT_COUNT; i++)
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
                            bool hover,
                            float spacing) {
  float delta = 0.0f;
  Vector2 textdim = MeasureTextEx(font, text, font.baseSize, spacing);
  if (hover && mousepressed)
    delta = -2.0f;
  paintRectangle(pos, dim, BLANK, hover, hover && mousepressed, false);
  DrawTextEx(font, text,
             (Vector2){pos.x + dim.x / 2 - textdim.x / 2 + delta,
                       pos.y + dim.y / 2 - textdim.y / 2 + delta},
             font.baseSize, spacing, textcolor);
}

static void paintScrollbar(int id) {
  float x = widgets[id].position.x + widgets[id].size.x + 2;
  float y = widgets[id].position.y + widgets[id].size.y *
                                         widgets[id].yscrollpos /
                                         (-widgets[id].ytextmax);
  float h = widgets[id].size.y * widgets[id].size.y / widgets[id].ytextmax;
  if (h > widgets[id].size.y)
    h = widgets[id].size.y;
  if (y + h > widgets[id].size.y + widgets[id].position.y)
    h = widgets[id].size.y + widgets[id].position.y - y;
  DrawRectangleV((Vector2){x, y}, (Vector2){5, h}, DARKGRAY);
}

static int u32_to_utf8(const uint32_t cp, char out[5]) {
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

static int utf8_strlen(const char* s) {
  int len = 0;
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
    text[len] = INPUT_CURSOR;
  else
    len = len - 2;
  text[len + 1] = '\0';
}

static void process_keys(void) {
  int key;
  char buf[5];
  while ((key = GetKeyPressed()) != 0) {
    if (KEY_BACKSPACE == key) {
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
  for (int j = 0; j < MAX_WIDGETS; j++)
    widgets[j].typing = false;
}

static void init_longtext(int id, char* text) {
  int width = widgets[id].size.x;
  Vector2 charsize = MeasureTextEx(font[widgets[id].fontindex], "b",
                                   font[widgets[id].fontindex].baseSize,
                                   fontspacings[widgets[id].fontindex]);
  int chwidth = charsize.x;
  if (widgets[id].longtext != NULL) {
    free(widgets[id].longtext);
  }
  widgets[id].longtext = wrap_text_words(text, width / chwidth);
  Vector2 size =
      MeasureTextEx(font[widgets[id].fontindex], widgets[id].longtext,
                    font[widgets[id].fontindex].baseSize,
                    fontspacings[widgets[id].fontindex]);
  widgets[id].ytextmax = size.y;
}

static char* wrap_text_words(const char* text, int width) {
  int len = strlen(text);
  int cap = len * 2 + 2;
  char* out = malloc(cap);
  int j = 0;
  int curw = 0;
  for (int i = 0; i < len;) {
    if (text[i] == '\n') {
      out[j++] = '\n';
      curw = 0;
      i++;
      continue;
    }
    if (text[i] == ' ') {
      i++;
      continue;
    }
    int start = i;
    while (i < len && text[i] != ' ' && text[i] != '\n')
      i++;
    int wlen = i - start;
    if (curw && curw + 1 + wlen > width) {
      out[j++] = '\n';
      curw = 0;
    }
    if (wlen > width) {
      for (int k = 0; k < wlen; k++) {
        if (curw + 1 > width) {
          out[j++] = '\n';
          curw = 0;
        }
        out[j++] = text[start + k];
        curw += 1;
      }
      continue;
    }
    if (curw) {
      out[j++] = ' ';
      curw += 1;
    }
    memcpy(out + j, text + start, wlen);
    j += wlen;
    curw += wlen;
  }
  out[j] = '\0';
  return realloc(out, j + 1);
}

#endif  // defined(MYRAYGUI_IMPLEMENTATION)