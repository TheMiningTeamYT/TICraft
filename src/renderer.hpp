#pragma once
#include <cstdint>
#include <cstring>
#include <cstdio>
#include "fixedpoint.h"

#define maxNumberOfPolygons 2000
#define zCullingDistance 2000
#define maxNumberOfObjects 1000
#define showDraw false
#define outlineColor 0
#define diagnostics false
#define cubeSize 20

/*
A point in 3d space
*/
struct point {
    Fixed24 x;
    Fixed24 y;
    Fixed24 z;
};

struct squaredReturn {
    int n1squared;
    int n2squared;
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
(Unused) A line between 2 points (represented as indexes into an array of screenPoints)
*/
struct line {
    uint8_t point1;
    uint8_t point2;
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
    uint8_t* points;

    // The number of the polygon in the shape (cube) it is a part of
    uint8_t polygonNum;

    // The distance from the polygon to the camera
    uint8_t z;
};

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
        outline = initOutline;
    };

    object() {};

    void deleteObject();

    // Offset the cube
    void moveBy(int newX, int newY, int newZ);

    // Move the cube
    void moveTo(int newX, int newY, int newZ);

    // Prepare the cube's polygons for rendering
    void generatePolygons();

    void generatePoints();
    
    // Get the distance from the cube to the camera
    int getDistance();
    
    // The position of each of the cubes points (verticies) in screen space, once rendered
    screenPoint renderedPoints[8];

    // An index into an array of pointers representing the texture of the cube
    uint8_t texture;

    // X position of the cube
    int16_t x;

    // Y position of the cube
    int16_t y;

    // Z position of the cube
    int16_t z;

    // Whether the cube is currently visible
    bool visible = false;

    // draw as outline or texture
    bool outline;
};

// this can (and should) be replaced by a more efficient cubeSave
struct cubeSave {
    // X position of the cube
    Fixed24 x;

    // Y position of the cube
    Fixed24 y;

    // Z position of the cube
    Fixed24 z;

    // Size of the cube
    Fixed24 legacy_size;

    // An index into an array of pointers representing the texture of the cube
    uint8_t texture;
};

struct cubeSave_v2 {
    // X position of the cube
    int16_t x;

    // Y position of the cube
    int16_t y;

    // Z position of the cube
    int16_t z;

    // An index into an array of pointers representing the texture of the cube
    uint8_t texture;
};

/*
Save file format:
struct saveFile {
    char magic[7]; // BLOCKS
    unsigned int version = 1; // Version
    unsigned int numberOfObjects;
    cubeSave objects[numberOfObjects];
    uint8_t selectedObject;
    Fixed24[3] cameraPos;
    Fixed24[3] cursorPos;
    uint32_t checksum; // CRC32
};
*/

// Get the distance from a point to the camera
int getPointDistance(point point);

// Used for sorting of polygons
int zCompare(const void *arg1, const void *arg2);

// Used for sorting of objects
int distanceCompare(const void *arg1, const void *arg2);

int xCompare(const void *arg1, const void *arg2);

void deleteEverything();

// Render the 3D world
void drawScreen(bool fullRedraw);

// Render a single transformed polygon
void renderPolygon(object* sourceObject, polygon* preparedPolygon);

// An array of all the objects in the world
extern object* objects[maxNumberOfObjects];
extern object* zSortedObjects[maxNumberOfObjects];

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
extern Fixed24 cxdy;
extern Fixed24 nsxdy;
extern Fixed24 ncyd;
extern Fixed24 syd;
extern float angleX;
extern float angleY;
extern float degRadRatio;
void drawCursor();
void getBuffer();

#define xSort() qsort(objects, numberOfObjects, sizeof(object*), xCompare); qsort(zSortedObjects, numberOfObjects, sizeof(object*), distanceCompare);

void rotateCamera(float x, float y);

void resetCamera();

void drawImage(int x, int y, int width, int height, uint16_t* dataPointer);
void drawRectangle(int x, int y, int width, int height, uint16_t color);
void redrawScreen();

extern "C" {
    void drawTextureLineNewA(int startingX, int endingX, int startingY, int endingY, const uint8_t* texture, uint8_t colorOffset, uint8_t polygonZ);
    void drawTextureLineNewA_NoClip(int startingX, int endingX, int startingY, int endingY, const uint8_t* texture, uint8_t colorOffset, uint8_t polygonZ);
    uint16_t approx_sqrt_a(unsigned int n);
    uint8_t polygonZShift(unsigned int x);
    struct squaredReturn findXZSquared(Fixed24 n);
    struct squaredReturn findYSquared(Fixed24 n);
    void shadeScreen();
}