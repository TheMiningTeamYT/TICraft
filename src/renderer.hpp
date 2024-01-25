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
    uint16_t z;

    int16_t x;
    int16_t y;
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
    object(Fixed24 initX, Fixed24 initY, Fixed24 initZ, uint8_t initTexture, bool initOutline) {
        x = initX;
        y = initY;
        z = initZ;
        texture = initTexture;
        outline = initOutline;
        visible = true;
    };

    object() {};

    void deleteObject();

    // Offset the cube
    void moveBy(Fixed24 newX, Fixed24 newY, Fixed24 newZ);

    // Move the cube
    void moveTo(Fixed24 newX, Fixed24 newY, Fixed24 newZ);

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
    Fixed24 x;

    // Y position of the cube
    Fixed24 y;

    // Z position of the cube
    Fixed24 z;

    // Whether the cube is currently visible
    bool visible;

    // draw as outline or texture
    bool outline;
};

/*
A polygon that has been prepared for rendering
*/
struct transformedPolygon {
    // A pointer to the parent object of this polygon
    object* object;

    // The distance from the camera
    uint16_t z;

    // The original number of the polygon (in the unwrapping order)
    uint8_t polygonNum;
};

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
void renderPolygon(transformedPolygon polygon);

// An array of all the objects in the world
extern object** objects;
extern object** zSortedObjects;

// An array of all the polygons that are ready to be rendered
extern transformedPolygon* preparedPolygons;

// The number of polygons ready to be rendered
extern unsigned int numberOfPreparedPolygons;

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

extern "C" {
    // Convert a point from 3D space to screen space
    screenPoint transformPointNewA(point x);
    void drawTextureLineNewA(int startingX, int endingX, int startingY, int endingY, const uint8_t* texture, uint8_t colorOffset, uint8_t z);
    void drawTextureLineNewA_NoClip(int startingX, int endingX, int startingY, int endingY, const uint8_t* texture, uint8_t colorOffset, uint8_t z);
    uint16_t approx_sqrt_a(unsigned int n);
    void shadeScreen();
}