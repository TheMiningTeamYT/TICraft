#include <cstdint>
#include <cstring>
#include <cstdio>
#include "fixedpoint.h"

#define maxNumberOfPolygons 2400
#define zCullingDistance (Fixed24) 1500
#define maxNumberOfObjects 400

struct point {
    Fixed24 x;
    Fixed24 y;
    Fixed24 z;
};

struct screenPoint {
    int x;
    int y;
    int derotatedX;
    int derotatedY;
    Fixed24 z;
};

struct line {
    unsigned int point1;
    unsigned int point2;
};

struct lineEquation {
    Fixed24 cx;
    Fixed24 px;
    Fixed24 cy;
    Fixed24 py;
    Fixed24 length;
};

struct polygon {
    uint8_t* points;
    uint8_t numberOfPoints;
    uint8_t polygonNum;
    Fixed24 z;
};

class object {
    public:
    object(Fixed24 initX, Fixed24 initY, Fixed24 initZ, Fixed24 initSize, const uint8_t** initTexture) {
        x = initX;
        y = initY;
        z = initZ;
        size = initSize;
        texture = initTexture;
    };
    void moveBy(Fixed24 x, Fixed24 y, Fixed24 z);
    void moveTo(Fixed24 x, Fixed24 y, Fixed24 z);
    void generatePolygons();
    Fixed24 getDistance();
    screenPoint renderedPoints[8];
    const uint8_t** texture;
    Fixed24 x;
    Fixed24 y;
    Fixed24 z;
    Fixed24 size;
};

struct transformedPolygon {
    uint8_t* points;
    object* object;
    const uint8_t* texture;
    int minX;
    int minY;
    int maxX;
    int maxY;
    Fixed24 z;
};

screenPoint transformPoint(point point);
Fixed24 getPointDistance(point point);
int zCompare(const void *arg1, const void *arg2);
int renderedZCompare(const void *arg1, const void *arg2);
int distanceCompare(const void *arg1, const void *arg2);
void deletePolygons();
void deleteEverything();
void drawScreen();
void renderPolygon(transformedPolygon* polygon);

extern object* objects[400];
extern transformedPolygon* preparedPolygons[maxNumberOfPolygons];
extern unsigned int numberOfPreparedPolygons;
extern unsigned int numberOfObjects;
extern unsigned int outOfBoundsPolygons;
extern unsigned int obscuredPolygons;