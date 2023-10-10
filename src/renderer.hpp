#include <cstdint>
#include <cstring>
#include <cstdio>
#include "fixedpoint.h"

#define maxNumberOfPolygons 1000
#define zCullingDistance 2000
#define maxNumberOfObjects 800
#define showDraw false
#define outlineColor 0
#define diagnostics false

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
    int16_t z = 0;
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
    Fixed24 length;
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
    int16_t z;
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
    object(Fixed24 initX, Fixed24 initY, Fixed24 initZ, Fixed24 initSize, const uint8_t** initTexture, bool initOutline) {
        x = initX;
        y = initY;
        z = initZ;
        size = initSize;
        texture = initTexture;
        outline = initOutline;
        visible = true;
    };

    void deleteObject();

    // Offset the cube
    void moveBy(Fixed24 x, Fixed24 y, Fixed24 z);

    // Move the cube
    void moveTo(Fixed24 x, Fixed24 y, Fixed24 z);

    // Prepare the cube's polygons for rendering
    void generatePolygons(bool clip);

    void generatePoints();
    
    // Get the distance from the cube to the camera
    int getDistance();

    // The position of each of the cubes points (verticies) in screen space, once rendered
    screenPoint renderedPoints[8];

    // A pointer to an array of uint8_t arrays each representing a 16x16 texture for one face of the polygon
    const uint8_t** texture;

    // X position of the cube
    Fixed24 x;

    // Y position of the cube
    Fixed24 y;

    // Z position of the cube
    Fixed24 z;

    // Size of the cube
    Fixed24 size;

    // Whether the cube is currently visible
    // Unused right now; might get used in the future
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
    int16_t z;

    // The original number of the polygon (in the unwrapping order)
    uint8_t polygonNum;
};

// Convert a point from 3D space to screen space
screenPoint transformPoint(point point);

// Get the distance from a point to the camera
int getPointDistance(point point);

// Used for sorting of polygons
int zCompare(const void *arg1, const void *arg2);

// Used for sorting of transformedPolygons
int renderedZCompare(const void *arg1, const void *arg2);

// Used for sorting of objects
int distanceCompare(const void *arg1, const void *arg2);

int xCompare(const void *arg1, const void *arg2);

void deletePolygons();
void deleteEverything();

// Render the 3D world
void drawScreen(uint8_t mode);

// Render a single transformed polygon
void renderPolygon(transformedPolygon* polygon);
object** xSearch(object* key);

// An array of all the objects in the world
extern object* objects[maxNumberOfObjects];

// An array of all the polygons that are ready to be rendered
extern transformedPolygon* preparedPolygons[maxNumberOfPolygons];

// The number of polygons ready to be rendered
extern unsigned int numberOfPreparedPolygons;

// The number of objects in the world
extern unsigned int numberOfObjects;

// The number of polygons that are outside the visible area (should be zero with recent changes)
extern unsigned int outOfBoundsPolygons;

// The number of polygons that have been determined to be obscured by other polygons
extern unsigned int obscuredPolygons;

extern Fixed24 cameraXYZ[3];

void xSort();

unsigned int_sqrt(const unsigned n);