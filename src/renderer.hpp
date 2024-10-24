#pragma once
#include <cstdint>
#include <cstring>
#include <cstdio>
#include "fixedpoint.h"

// Might be a good idea to put these in a namespace. Maybe. :/

// might be too close
#define zCullingDistance 400
#define maxNumberOfObjects 3500
#define showDraw false
#define diagnostics false
#define cubeSize 20
#define visible 64
#define outline 128
#define is_visible(x) ((x)->properties & visible)
#define is_outline(x) ((x)->properties & outline)
#define outlineColor 252

/*
A point in 3d space
*/
struct point {
    Fixed24 x;
    Fixed24 y;
    Fixed24 z;
};

struct squaredPair {
    int a;
    int b;
};

/*
A point in screen space (with distance to the camera)
*/
struct screenPoint {
    int16_t x;
    int16_t y;

    //Distance to the camera
    uint16_t z = 0;
};

/*
A parametric line equation between two points
(ie: (1-t)C + tP = y)
*/
struct lineEquation {
    // C for the x dimension
    Fixed24 cx;

    // P for the x dimension
    Fixed24 px;

    // C for the y dimension
    Fixed24 cy;

    // P for the y dimension
    Fixed24 py;

    // The length of the line
    int length;

    Fixed24 dx;
    Fixed24 dy;
};

/*
A raw polygon (really quadrilateral)
*/
struct polygon {
    // The points of the polygon, represented as a list of indexes into an array of screenPoints
    unsigned int points[4];

    // The number of the polygon in the shape (cube) it is a part of
    unsigned int polygonNum;
};

// Get the distance from a point to the camera
int getPointDistance(point point);

/*
3D Object (really a cube)
*/
class object {
    public:
    // initX: starting X position for the cube
    // initY: starting Y position for the cube
    // initZ: starting Z position for the cube
    // initSize: starting size for the cube
    // initTexture: pointer to an array of uint8_t arrays each representing a 16x16 texture for one face of the polygon
    object(int initX, int initY, int initZ, uint8_t initTexture, bool initOutline) {
        x = initX;
        y = initY;
        z = initZ;
        texture = initTexture;
        if (initOutline) {
            properties |= outline;
        }
    }

    object() {};

    void deleteObject();

    // Offset the cube
    void moveBy(int newX, int newY, int newZ);

    // Prepare the cube's polygons for rendering
    void generatePolygons();

    void generatePoints();

    void findDistance();

    // An index into an array of pointers representing the texture of the cube
    uint8_t texture;

    // X position of the cube
    int16_t x;

    // Y position of the cube
    int16_t y;

    // Z position of the cube
    int16_t z;

    // The distance of the cube from the camera
    uint16_t distance = 0;

    // Whether the cube is currently visible and whether to draw it as an outline or textured
    uint8_t properties = 0;
};

// Used for sorting of objects
int distanceCompare(const void *arg1, const void *arg2);

int xCompare(const void *arg1, const void *arg2);

void deleteEverything();

// Render the 3D world
void drawScreen();

// Render a single transformed polygon
void renderPolygon(object* sourceObject, screenPoint* polygonRenderedPoints, uint8_t polygonNum, uint8_t normalizedZ);

// An array of all the objects in the world
extern object* objects[maxNumberOfObjects];
extern screenPoint renderedPoints[8];

// The number of objects in the world
extern unsigned int numberOfObjects;

// The number of polygons that are outside the visible area (should be zero with recent changes)
extern unsigned int outOfBoundsPolygons;

// The number of polygons that have been determined to be obscured by other polygons
extern unsigned int obscuredPolygons;

extern Fixed24 cameraXYZ[3];

extern Fixed24 cx;
extern Fixed24 sx;
extern Fixed24 cy;
extern Fixed24 sy;
extern Fixed24 nsxsy;
extern Fixed24 nsxcy;
extern Fixed24 cxsy;
extern Fixed24 cxcy;
// d means delta
extern Fixed24 cxd;
extern Fixed24 nsxd;
extern Fixed24 cyd;
extern Fixed24 nsyd;
extern Fixed24 nsxsyd;
extern Fixed24 nsxcyd;
extern Fixed24 cxsyd;
extern Fixed24 cxcyd;
extern Fixed24 angleX;
extern Fixed24 angleY;
#define xSort() qsort(objects, numberOfObjects, sizeof(object*), xCompare); zSort();
void rotateCamera(Fixed24 x, Fixed24 y);
void resetCamera();
void drawImage(int x, int y, int width, int height, uint16_t* dataPointer);
void drawRectangle(int x, int y, int width, int height, uint16_t color);
void redrawScreen();
void zSort();

extern "C" {
    void drawTextureLineNewA(int startingX, int endingX, int startingY, int endingY, const uint8_t* texture, uint8_t colorOffset, uint8_t polygonZ);
    void drawTextureLineNewA_NoClip(int startingX, int endingX, int startingY, int endingY, const uint8_t* texture, uint8_t colorOffset, uint8_t polygonZ);
    uint16_t approx_sqrt_a(unsigned int n);
    uint8_t polygonZShift(unsigned int x);
    struct squaredPair findXYZSquared(Fixed24 n);
    int polygonPointMultiply(int n);
    void shadeScreen();
}