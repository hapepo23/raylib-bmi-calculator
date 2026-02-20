#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* RayLib BMI Calculator, Version: 2025-02-18 */

/* ----------------------------------------------------------------*/

// Definitions

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
#define FONT_PATHS \
  { "LiberationSans-Regular.ttf", "LiberationSans-Bold.ttf" }
#define FONT_SIZES \
  { 32, 32 }
#define FONT_SPACINGS \
  { 0.f, 0.f }
#define FONT_ADD_CP_RANGES_COUNT 3
#define FONT_ADD_CP_RANGES                \
  {                                       \
    {0x00b2, 0x00b2}, {0x25b2, 0x25b2}, { \
      0x25bc, 0x25bc                      \
    }                                     \
  }
#define TEXT_LINE_SPACING 0
#define INPUT_CURSOR '_'

#define MAX_WIDGETS 9

#define MAX_WEIGHT 200
#define MIN_WEIGHT 30
#define DFT_WEIGHT 70
#define MAX_HEIGHT 220
#define MIN_HEIGHT 120
#define DFT_HEIGHT 170

/* ----------------------------------------------------------------*/

#include "myraygui.h"

/* ----------------------------------------------------------------*/

int main(void);
static void click(int id);
static void print(int id);
static void check(int id);
static void calcBMI(void);
static void int2string(const int i, char* str);
static void string2int(const char* str, int* i);

/* ----------------------------------------------------------------*/

// Widget Definitions

static WidgetData widgets[MAX_WIDGETS] = {
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

/* ----------------------------------------------------------------*/

// Application Data

static int weight = DFT_WEIGHT;
static int height = DFT_HEIGHT;
static char BMI[25] = "";

/* ----------------------------------------------------------------*/

// Main

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

/* ----------------------------------------------------------------*/

// Callback functions

static void click(int id) {
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

static void print(int id) {
  if (id == 8)
    calcBMI();
}

static void check(int id) {
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

/* ----------------------------------------------------------------*/

// Utilities

static void calcBMI(void) {
  if (typing_widget == 2 || typing_widget == 3)
    sprintf(BMI, "BMI = ...");
  else
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

#define MYRAYGUI_IMPLEMENTATION
#include "myraygui.h"

/* ----------------------------------------------------------------*/
