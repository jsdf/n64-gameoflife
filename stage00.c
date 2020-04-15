
#include <assert.h>
#include <math.h>
#include <nusys.h>
#include <string.h>

#include "graphic.h"
#include "main.h"
#include "stage00.h"

#define DEBUG_CONSOLE 0
#define TICK_SKIP_FRAME 8

#define CELLS_X 50
#define CELLS_Y 40
#define CELL_WIDTH_X 5
#define CELL_WIDTH_Y 5
// #define DEAD 0
// #define ALIVE 1
#define START_ZOOM 10.0f
#define END_ZOOM 1.0f
#define ZOOM_TIME 10.0f

void tick();
float cubicOut(float t);
float lerp(float v0, float v1, float t);
void drawSquares(GraphicsTask* gfxTask);
void initGrid();

typedef enum CellState {
  DEAD,
  ALIVE,
} CellState;

typedef struct Cell {
  CellState state;
  int lastChanged;
} Cell;

Cell grid[2][CELLS_Y][CELLS_X];
int curGrid;
float zoom;
int ticks;
int frame;
int colormode;

// the 'setup' function
void initStage00() {
  // the advantage of initializing these values here, rather than statically, is
  // that if you switch stages/levels, and later return to this stage, you can
  // call this function to reset these values.
  curGrid = 0;
  // zoom = START_ZOOM;
  zoom = END_ZOOM;
  ticks = 0;
  frame = 0;
  colormode = 0;

  initGrid();
}

// the 'update' function
void updateGame00() {
  // float t = MIN(nuScRetraceCounter / (60.0f * ZOOM_TIME), 1.0f);
  // zoom = lerp(START_ZOOM, END_ZOOM, cubicOut(t));
  if (frame % TICK_SKIP_FRAME == 0) {
    tick();
    ticks++;
  }

  // read controller input from controller 1 (index 0)
  nuContDataGetEx(contdata, 0);
  // We check if the 'A' Button was pressed using a bitwise AND with
  // contdata[0].trigger and the A_BUTTON constant.
  // The contdata[0].trigger property is set only for the frame that the button
  // is initially pressed. The contdata[0].button property is similar, but stays
  // on for the duration of the button press.
  if (contdata[0].trigger & A_BUTTON) {
    initGrid();
  }
  if (contdata[0].trigger & B_BUTTON) {
    colormode = !colormode;
  }

  frame++;
}

// the 'draw' function
void makeDL00() {
  GraphicsTask* gfxTask;

  // switch the current graphics task
  // also frame the displayListPtr global variable
  gfxTask = gfxSwitchTask();

  // prepare the RCP for rendering a graphics task
  gfxRCPInit();

  // clear the color framebuffer and Z-buffer, similar to glClear()
  gfxClearCfb(colormode ? GPACK_RGBA5551(0, 0, 0, 1)
                        : GPACK_RGBA5551(255, 255, 255, 1));

  guOrtho(&gfxTask->projection, -(float)SCREEN_WD / 2.0F,
          (float)SCREEN_WD / 2.0F, -(float)SCREEN_HT / 2.0F,
          (float)SCREEN_HT / 2.0F, -10.0F, 10.0F, 10.0F);

  // load the projection matrix into the matrix stack.
  // given the combination of G_MTX_flags we provide, effectively this means
  // "replace the projection matrix with this new matrix"
  gSPMatrix(displayListPtr++,
            // we use the OS_K0_TO_PHYSICAL macro to convert the pointer to this
            // matrix into a 'physical' address as required by the RCP
            OS_K0_TO_PHYSICAL(&(gfxTask->projection)),
            // these flags tell the graphics microcode what to do with this
            // matrix documented here:
            // http://n64devkit.square7.ch/tutorial/graphics/1/1_3.htm
            G_MTX_PROJECTION |  // using the projection matrix stack...
                G_MTX_LOAD |  // don't multiply matrix by previously-top matrix
                              // in stack
                G_MTX_NOPUSH  // don't push another matrix onto the stack before
                              // operation
  );

  guScale(&gfxTask->modelview, zoom, zoom, 1.0f);
  // guMtxIdent(&gfxTask->modelview);

  gSPMatrix(displayListPtr++, OS_K0_TO_PHYSICAL(&(gfxTask->modelview)),
            // similarly this combination means "replace the modelview matrix
            // with this new matrix"
            G_MTX_MODELVIEW | G_MTX_NOPUSH | G_MTX_LOAD);

  drawSquares(gfxTask);

  // mark the end of the display list
  gDPFullSync(displayListPtr++);
  gSPEndDisplayList(displayListPtr++);

  // assert that the display list isn't longer than the memory allocated for it,
  // otherwise we would have corrupted memory when writing it.
  // isn't unsafe memory access fun?
  // this could be made safer by instead asserting on the displaylist length
  // every time the pointer is advanced, but that would add some overhead.
  assert(displayListPtr - gfxTask->displayList < MAX_DISPLAY_LIST_COMMANDS);

  // create a graphics task to render this displaylist and send it to the RCP
  nuGfxTaskStart(
      gfxTask->displayList,
      (int)(displayListPtr - gfxTask->displayList) * sizeof(Gfx),
      NU_GFX_UCODE_F3DEX,  // load the 'F3DEX' version graphics microcode, which
                           // runs on the RCP to process this display list
      NU_SC_NOSWAPBUFFER   // tells NuSystem to immediately display the frame on
                           // screen after the RCP finishes rendering it
  );

  if (DEBUG_CONSOLE) {
    char conbuf[100];
    nuDebConTextPos(0, 12, 23);
    sprintf(conbuf, "retrace=%d, %d", (int)nuScRetraceCounter,
            ((int)nuScRetraceCounter % 2));
    nuDebConCPuts(0, conbuf);
    nuDebConTextPos(0, 12, 24);
    sprintf(conbuf, "ticks=%d", ticks);
    nuDebConCPuts(0, conbuf);
    nuDebConTextPos(0, 12, 25);
    sprintf(conbuf, "frame=%d", frame);
    nuDebConCPuts(0, conbuf);
  }

  /* Draw characters on the frame buffer */
  nuDebConDisp(NU_SC_SWAPBUFFER);
}

typedef struct RGBColor {
  int r;
  int g;
  int b;
} RGBColor;

RGBColor hsvToRGB(float h, float s, float v) {
  float chroma = v * s;
  float hprime = h / 60.0f;
  float x = chroma * (1.0f - fabsf(fmodf(hprime, 2.0f) - 1.0f));

  float r = 0.0f;
  float g = 0.0f;
  float b = 0.0f;
  if (hprime <= 1) {
    r = chroma;
    g = x;
    b = 0;
  } else if (hprime <= 2) {
    r = x;
    g = chroma;
    b = 0;
  } else if (hprime <= 3) {
    r = 0;
    g = chroma;
    b = x;
  } else if (hprime <= 4) {
    r = 0;
    g = x;
    b = chroma;
  } else if (hprime <= 5) {
    r = x;
    g = 0;
    b = chroma;
  } else if (hprime <= 6) {
    r = chroma;
    g = 0;
    b = x;
  }
  {
    float m = v - chroma;
    return (RGBColor){(r + m) * 255, (g + m) * 255, (b + m) * 255};
  }
}

#define COLOR_R 0xff
#define COLOR_G 0x00
#define COLOR_B 0x00
#define MAX_AGE 0.8

#define PACK_RGBA4444(r, g, b, a) (r << 24) + (g << 16) + (b << 8) + a

// A static array of model vertex data.
// This include the position (x,y,z), texture U,V coords (called S,T in the SDK)
// and vertex color values in r,g,b,a form.
// As this data will be read by the RCP via direct memory access, which is
// required to be 16-byte aligned, it's a good idea to annotate it with the GCC
// attribute `__attribute__((aligned (16)))`, to force it to be 16-byte aligned.
static Vtx squareVerts[] = {
    //  x,   y,  z, flag, S, T,    r,    g,    b,    a
    {-10, 10, -5, 0, 0, 0, COLOR_R, COLOR_G, COLOR_B, 0xff},
    {10, 10, -5, 0, 0, 0, COLOR_R, COLOR_G, COLOR_B, 0xff},
    {10, -10, -5, 0, 0, 0, COLOR_R, COLOR_G, COLOR_B, 0xff},
    {-10, -10, -5, 0, 0, 0, COLOR_R, COLOR_G, COLOR_B, 0xff},
};

void drawSquares(GraphicsTask* gfxTask) {
  int row, col, i, color;
  float age, x_norm, y_norm, theta;
  RGBColor rgbColor;
  // depending on which graphical features, the RDP might need to spend 1 or 2
  // cycles to render a primitive, and we need to tell it which to do
  gDPSetCycleType(displayListPtr++, G_CYC_1CYCLE);
  // use antialiasing, rendering an opaque surface
  gDPSetRenderMode(displayListPtr++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
  // reset any rendering flags set when drawing the previous primitive
  gSPClearGeometryMode(displayListPtr++, 0xFFFFFFFF);

  gSPSetGeometryMode(displayListPtr++, G_SHADE);

  for (row = 0; row < CELLS_Y; ++row) {
    for (col = 0; col < CELLS_X; ++col) {
      i = row * CELLS_X + col;
      if (grid[curGrid][row][col].state != ALIVE)
        continue;
      // create a transformation matrix representing the position of the
      // square
      guPosition(&gfxTask->objectTransforms[i],
                 // rotation
                 0.0f,          // roll
                 0.0f,          // pitch
                 0.0f,          // heading
                 2.0f / 10.0f,  // scale
                                // position
                 (col - CELLS_X / 2) * CELL_WIDTH_X,
                 (row - CELLS_Y / 2) * CELL_WIDTH_Y,
                 0.0  // between near and far plane
      );

      // push relative transformation matrix
      gSPMatrix(displayListPtr++,
                OS_K0_TO_PHYSICAL(&(gfxTask->objectTransforms[i])),
                G_MTX_MODELVIEW |  // operating on the modelview matrix stack...
                    G_MTX_PUSH |   // ...push another matrix onto the stack...
                    G_MTX_MUL  // ...which is multipled by previously-top matrix
                               // (eg. a relative transformation)
      );

      // load vertex data for the triangles
      gSPVertex(displayListPtr++, &(squareVerts[0]), 4, 0);
      age =
          (MAX_AGE -
           MIN(MAX_AGE, (frame - grid[curGrid][row][col].lastChanged) / 60.0) +
           (1.0 - MAX_AGE));
      y_norm = (row + 1) / (float)CELLS_Y;
      x_norm = (col + 1) / (float)CELLS_X;
      theta = fmodf(x_norm * 0.01 * y_norm * 360.0 + frame, 360);
      if (colormode) {
        rgbColor = hsvToRGB(theta,  // hue
                            0.7,    // sat
                            age     // value
        );
      } else {
        rgbColor = hsvToRGB(theta,     // hue
                            0.3 * 0,   // sat
                            1.0 - age  // value
        );
      }
      color = PACK_RGBA4444(rgbColor.r, rgbColor.g, rgbColor.b, 255);

      // color =
      //    ((int)(((col + 1) / (float)CELLS_X) * age * 255) << 24) +
      //    ((int)((col + 1) / (float)CELLS_X * age * 255) << 16) +
      //    ((int)((row + 1) / (float)CELLS_Y * age * 255) << 8) + 255;
      gSPModifyVertex(displayListPtr++, 0, G_MWO_POINT_RGBA, color);

      gSP2Triangles(displayListPtr++, 0, 1, 2, 0, 0, 2, 3, 0);

      // pop the matrix that we added back off the stack, to move the drawing
      // position back to where it was before we rendered this object
      gSPPopMatrix(displayListPtr++, G_MTX_MODELVIEW);
    }
  }

  // Mark that we've finished sending commands for this particular
  // primitive. This is needed to prevent race conditions inside the
  // rendering hardware in the case that subsequent commands change
  // rendering settings.
  gDPPipeSync(displayListPtr++);
}

void makeCopy(int from, int to) {
  memcpy(grid[to], grid[from], sizeof(Cell) * CELLS_X * CELLS_Y);
}

int neighboursAlive(int gridN, int cellY, int cellX) {
  int y;
  int x;
  CellState neighborState;
  int aliveNeighbors = 0;
  int startX = CLAMP((cellX - 1), 0, CELLS_X - 1);
  int startY = CLAMP((cellY - 1), 0, CELLS_Y - 1);
  int endX = CLAMP((cellX + 1), 0, CELLS_X - 1);
  int endY = CLAMP((cellY + 1), 0, CELLS_Y - 1);

  for (y = startY; y <= endY; y++) {
    for (x = startX; x <= endX; x++) {
      if (!(x == cellX && y == cellY)) {
        neighborState = grid[gridN][y][x].state;
        if (neighborState == ALIVE) {
          aliveNeighbors++;
        }
      }
    }
  }

  return aliveNeighbors;
}

void initGrid() {
  int row, col;
  for (row = 0; row < CELLS_Y; ++row) {
    for (col = 0; col < CELLS_X; ++col) {
      grid[curGrid][row][col].state = RAND(10) > 5;
      grid[curGrid][row][col].lastChanged = frame;
    }
  }
}

void tick() {
  int y, x;
  CellState cellState;
  int aliveNeighbors;
  int nextGrid = curGrid ^ 1;
  makeCopy(curGrid, nextGrid);

  for (y = 0; y < CELLS_Y; ++y) {
    for (x = 0; x < CELLS_X; ++x) {
      cellState = grid[curGrid][y][x].state;

      aliveNeighbors = neighboursAlive(curGrid, y, x);

      if (cellState) {
        if (aliveNeighbors > 3 || aliveNeighbors < 2) {
          cellState = DEAD;
        }
      } else {
        if (aliveNeighbors == 3) {
          cellState = ALIVE;
        }
      }
      if (cellState != grid[nextGrid][y][x].state) {
        grid[nextGrid][y][x].state = cellState;
        grid[nextGrid][y][x].lastChanged = frame;
      }
    }
  }
  // flip grids
  curGrid = nextGrid;
}

float cubicOut(float t) {
  float invT = 1.0f - t;
  return 1.0f - (invT * invT * invT);
}

float lerp(float v0, float v1, float t) {
  return (1 - t) * v0 + t * v1;
}

// the nusystem callback for the stage, called once per frame
void stage00(int pendingGfx) {
  // produce a new displaylist (unless we're running behind, meaning we already
  // have the maximum queued up)
  if (pendingGfx < 2)
    makeDL00();

  // update the state of the world for the next frame
  updateGame00();
}
