#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ti/getcsc.h>
#include <graphx.h>
#include "renderer.hpp"

#define focalDistance (Fixed24)200

/*
optimization ideas:
Add support for different sized textures, depending on whats most apropriate, say 16x16, 8x8, or 4x4. That way, for small polygons we can waste less time drawing.
Or, undersampling the textures at small sizes
Drawing the polygon is the longest part of the process
*/
/*
Speed observations:
It appears that drawing the polygons is a farely slow process (if you disable double buffering you can see it happen --- it should be too fast to see)
It seems that generating the row is the longest part of the polygon drawing process. -- maybe can be optimized?
Undersampling the texture by 2x (16->8px) led to over a 1/3 improvement in render time
It also appears that generating the polygons takes a considerable amount of time as well (with double buffering disabled you can see it sit as it generates them all)
A faster method of drawing the pixels to the screen (rather than drawing tons of little lines) would be preferred
It would also likely be faster if we didn't have to overdraw the lines to avoid blank pixels
Also, the culling is in *dire* need of fixing --- it needs to be a LOT less agressive
Also, the fixed point library I'm using does not have a very big range, so overflow issues are common. Maybe a better fixed point library is in order?
*/

object* objects[maxNumberOfObjects];
transformedPolygon* preparedPolygons[maxNumberOfPolygons];
char buffer[200];
unsigned int numberOfPreparedPolygons = 0;
unsigned int numberOfObjects = 0;
unsigned int outOfBoundsPolygons = 0;
unsigned int obscuredPolygons = 0;

uint8_t face1Points[] = {0, 1, 2, 3};
polygon face1 = {face1Points, 4, 5};
uint8_t face5Points[] = {5, 1, 2, 6};
polygon face5 = {face5Points, 4, 3};
uint8_t face4Points[] = {4, 0, 3, 7};
polygon face4 = {face4Points, 4, 1};
uint8_t face3Points[] = {4, 5, 6, 7};
polygon face3 = {face3Points, 4, 4};
uint8_t face2Points[] = {3, 2, 6, 7};
polygon face2 = {face2Points, 4, 2};
uint8_t face0Points[] = {4, 0, 1, 5};
polygon face0 = {face0Points, 4, 0};
/*
unwrapping order:
top, front, bottom, back, left, right
*/ 
polygon cubePolygons[] = {face0, face1, face2, face3, face4, face5};

Fixed24 cameraXYZ[3] = {-50, 100, -100};
const float cameraAngle[3] = {0.7853982, 0.7853982, 0};
const Fixed24 cx = cosf(cameraAngle[0]);
const Fixed24 sx = sinf(cameraAngle[0]);
const Fixed24 cy = cosf(cameraAngle[1]);
const Fixed24 sy = sinf(cameraAngle[1]);
const Fixed24 cz = cosf(cameraAngle[2]);
const Fixed24 sz = sinf(cameraAngle[2]);
void object::generatePolygons() {
    point points[] = {{x, y, z}, {x+size, y, z}, {x+size, y-size, z}, {x, y-size, z}, {x, y, z+size}, {x+size, y, z+size}, {x+size, y-size, z+size}, {x, y-size, z+size}};
    polygon polygons[6];
    memcpy(polygons, cubePolygons, sizeof(polygon)*6);
    bool objectRender = true;
    for (uint8_t i = 0; i < 8; i++) {
        Fixed24 z = getPointDistance(points[i]);
        if (z < zCullingDistance) {
            renderedPoints[i] = transformPoint(points[i]);
            renderedPoints[i].z = z;
        } else {
            objectRender = false;
            break;
        }
    }
    if (objectRender == true) {
        for (uint8_t polygonNum = 0; polygonNum < 6; polygonNum++) {
            Fixed24 z = 0;
            polygon* polygon = &polygons[polygonNum];
            for (unsigned int i = 0; i < 4; i++) {
                z += renderedPoints[polygon->points[i]].z;
            }
            z.n = z.n >> 2;
            polygon->z = z;
        }
        qsort((void *) polygons, 6, sizeof(polygon), zCompare);
        for (uint8_t polygonNum = 0; polygonNum < 6; polygonNum++) {
            if (numberOfPreparedPolygons < maxNumberOfPolygons) {
                polygon* polygon = &polygons[polygonNum];
                bool render = true;
                Fixed24 z = polygon->z;
                uint8_t offScreenPoints = 0;
                for (uint8_t i = 0; i < 4; i++) {
                    screenPoint point = renderedPoints[polygon->points[i]];
                    if (point.x < -1000 || point.x > 1320 || point.y < -1000 || point.y > 1240) {
                        render = false;
                        outOfBoundsPolygons++;
                        break;
                    }
                    if (point.x < 0 || point.x > 320 || point.y < 0 || point.y > 240) {
                        offScreenPoints++;
                    }
                    
                }
                if (offScreenPoints == 4) {
                    render = false;
                    outOfBoundsPolygons++;
                }
                if (numberOfPreparedPolygons > 0) {
                    if (render == true) {
                        unsigned int obscuredPoints = 0;
                        bool points[4] = {true, true, true, true};
                        for (unsigned int otherPolygonNum = 0; otherPolygonNum < numberOfPreparedPolygons; otherPolygonNum++) {
                            transformedPolygon* otherPolygon = preparedPolygons[otherPolygonNum];
                            if (z > otherPolygon->z) {
                                for (unsigned int i = 0; i < 4; i++) {
                                    if (points[i] == true) {
                                        screenPoint* point = &renderedPoints[polygon->points[i]];
                                        if ((point->x >= otherPolygon->minX && point->x <= otherPolygon->maxX && point->y >= otherPolygon->minY && point->y <= otherPolygon->maxY)) {
                                            obscuredPoints++;
                                            points[i] = false;
                                        }
                                    }
                                }
                            }
                        }
                        if (obscuredPoints >= 4) {
                            render = false;
                            obscuredPolygons++;
                        }
                    }
                }
                if (render == true) {
                    int minX = renderedPoints[polygon->points[0]].x;
                    int minY = renderedPoints[polygon->points[0]].y;
                    int maxX = renderedPoints[polygon->points[0]].x;
                    int maxY = renderedPoints[polygon->points[0]].y;
                    for (unsigned int i = 0; i < 4; i++) {
                        if (renderedPoints[polygon->points[i]].x < minX) {
                            minX = renderedPoints[polygon->points[i]].x;
                        } else if (renderedPoints[polygon->points[i]].x > maxX) {
                            maxX = renderedPoints[polygon->points[i]].x;
                        }
                        if (renderedPoints[polygon->points[i]].y < minY) {
                            minY = renderedPoints[polygon->points[i]].y;
                        } else if (renderedPoints[polygon->points[i]].y > maxY) {
                            maxY = renderedPoints[polygon->points[i]].y;
                        }
                    }
                    preparedPolygons[numberOfPreparedPolygons] = new transformedPolygon;
                    preparedPolygons[numberOfPreparedPolygons]->minX = minX;
                    preparedPolygons[numberOfPreparedPolygons]->minY = minY;
                    preparedPolygons[numberOfPreparedPolygons]->maxX = maxX;
                    preparedPolygons[numberOfPreparedPolygons]->maxY = maxY;
                    preparedPolygons[numberOfPreparedPolygons]->z = polygon->z;
                    preparedPolygons[numberOfPreparedPolygons]->points = polygon->points;
                    preparedPolygons[numberOfPreparedPolygons]->object = this;
                    preparedPolygons[numberOfPreparedPolygons]->texture = texture[polygon->polygonNum];
                    numberOfPreparedPolygons++;
                }
            }
        }
    }
}

void object::moveBy(Fixed24 newX, Fixed24 newY, Fixed24 newZ) {
    x += newX;
    y += newY;
    z += newZ;
}

void object::moveTo(Fixed24 newX, Fixed24 newY, Fixed24 newZ) {
    x = newX;
    y = newY;
    z = newZ;
}

Fixed24 object::getDistance() {
    return sqrtf(powf(x - cameraXYZ[0], 2) + powf(y - cameraXYZ[1], 2) + powf(z - cameraXYZ[2], 2));
}

int zCompare(const void *arg1, const void *arg2) {
    polygon* polygon1 = (polygon *) arg1;
    polygon* polygon2 = (polygon *) arg2;
    return polygon1->z - polygon2->z;
}

int renderedZCompare(const void *arg1, const void *arg2) {
    transformedPolygon* polygon1 = *((transformedPolygon **) arg1);
    transformedPolygon* polygon2 = *((transformedPolygon **) arg2);
    return polygon2->z - polygon1->z;
}

int distanceCompare(const void *arg1, const void *arg2) {
    object* object1 = *((object**) arg1);
    object* object2 = *((object**) arg2);
    return object1->getDistance() - object2->getDistance();
}

screenPoint transformPoint(point point) {
    screenPoint result;
    const Fixed24 x = point.x - cameraXYZ[0];
    const Fixed24 y = point.y - cameraXYZ[1];
    const Fixed24 z = point.z - cameraXYZ[2];
    const Fixed24 sum1 = (sz*y+cz*x);
    const Fixed24 sum2 = (cy*z+sy*sum1);
    const Fixed24 sum3 = (cz*y - sz*x);
    const Fixed24 dx = cy*sum1-sy*z;
    const Fixed24 dy = sx*sum2 + cx*sum3;
    const Fixed24 dz = cx*sum2 - sx*sum3;
    const Fixed24 sum4 = (focalDistance/dz);
    result.x = ((Fixed24)160 + sum4*dx).floor();
    result.y = ((Fixed24)120 - sum4*dy).floor();
    //result.z = sqrtf(powf(x, 2) + powf(y, 2) + powf(z, 2));
    return result;
}

void deletePolygons() {
    for (unsigned int i = 0; i < numberOfPreparedPolygons; i++) {
        delete preparedPolygons[i];
    }
    numberOfPreparedPolygons = 0;
}

void deleteEverything() {
    deletePolygons();
    for (unsigned int i = 0; i < numberOfObjects; i++) {
        delete objects[numberOfObjects];
    }
}

void drawScreen() {
    deletePolygons();
    outOfBoundsPolygons = 0;
    obscuredPolygons = 0;
    cameraXYZ[2] += 5;
    gfx_SetColor(0);
    gfx_SetTextXY(0, 0);
    //improves polygon culling but sorting can be slow
    qsort(objects, numberOfObjects, sizeof(object*), distanceCompare);
    for (unsigned int i = 0; i < numberOfObjects; i++) {
        objects[i]->generatePolygons();
    }
    qsort(preparedPolygons, numberOfPreparedPolygons, sizeof(transformedPolygon *), renderedZCompare);
    gfx_FillScreen(255);
    for (unsigned int i = 0; i < numberOfPreparedPolygons; i++) {
        renderPolygon(preparedPolygons[i]);
    }
    snprintf(buffer, 200, "Out of bounds polygons: %u", outOfBoundsPolygons);
    gfx_PrintString(buffer);
    gfx_SetTextXY(0, gfx_GetTextY() + 10);
    snprintf(buffer, 200, "Obscured Polygons: %u", obscuredPolygons);
    gfx_PrintString(buffer);
    gfx_SetTextXY(0, gfx_GetTextY() + 10);
    snprintf(buffer, 200, "Total Polygons: %u", numberOfPreparedPolygons);
    gfx_PrintString(buffer);
    //debugging stuff
    /*gfx_SetTextXY(0, gfx_GetTextY() + 10);
    snprintf(buffer, 200, "Object Size: %u", sizeof(object));
    gfx_PrintString(buffer);
    gfx_SetTextXY(0, gfx_GetTextY() + 10);
    snprintf(buffer, 200, "Polygon Size: %u", sizeof(transformedPolygon));
    gfx_PrintString(buffer);
    gfx_SetTextXY(0, gfx_GetTextY() + 10); */
    #if showDraw == false
    gfx_SwapDraw();
    #endif
}

void renderPolygon(transformedPolygon* polygon) {
    // implementation of affine texture mapping (i think thats what you'd call this anyway)
    // as noted in The Science Elf's original video, affine texture mapping can look very weird on triangles
    // but on quads, it looks pretty good at a fraction of the cost
    #define sourceObject polygon->object
    //object* sourceObject = polygon->object;
    point sourcePoints[] = {{sourceObject->x, sourceObject->y, sourceObject->z}, {sourceObject->x+sourceObject->size, sourceObject->y, sourceObject->z}, {sourceObject->x+sourceObject->size, sourceObject->y-sourceObject->size, sourceObject->z}, {sourceObject->x, sourceObject->y-sourceObject->size, sourceObject->z}, {sourceObject->x, sourceObject->y, sourceObject->z+sourceObject->size}, {sourceObject->x+sourceObject->size, sourceObject->y, sourceObject->z+sourceObject->size}, {sourceObject->x+sourceObject->size, sourceObject->y-sourceObject->size, sourceObject->z+sourceObject->size}, {sourceObject->x, sourceObject->y-sourceObject->size, sourceObject->z+sourceObject->size}};
    line lines[4];
    lineEquation lineEquations[4];
    uint8_t* points = polygon->points;
    for (unsigned int i = 0; i < 4; i++) {
        unsigned int nextPoint = i + 1;
        if (nextPoint > 3) {
            nextPoint = 0;
        }
        lines[i] = {points[i], points[nextPoint]};
        Fixed24 dx = sourceObject->renderedPoints[points[nextPoint]].x - sourceObject->renderedPoints[points[i]].x;
        Fixed24 dy = sourceObject->renderedPoints[points[nextPoint]].y - sourceObject->renderedPoints[points[i]].y;
        Fixed24 length = sqrtf(powf(sourceObject->renderedPoints[points[nextPoint]].x - sourceObject->renderedPoints[points[i]].x, 2) + powf(sourceObject->renderedPoints[points[nextPoint]].y - sourceObject->renderedPoints[points[i]].y, 2));
        lineEquations[i] = {sourceObject->renderedPoints[points[i]].x, sourceObject->renderedPoints[points[nextPoint]].x, sourceObject->renderedPoints[points[i]].y, sourceObject->renderedPoints[points[nextPoint]].y, length};
    }
    Fixed24 length;
    if (lineEquations[3].length > lineEquations[1].length) {
        length = lineEquations[3].length;
    } else {
        length = lineEquations[1].length;
    }
    const Fixed24 reciprocalOf16 = (Fixed24)1/(Fixed24)16;
    const Fixed24 textureRatio = length*reciprocalOf16;
    const Fixed24 reciprocalOfTextureRatio = (Fixed24)1/textureRatio;
    const Fixed24 reciprocalOfLength = (Fixed24)1/length;
    for (uint8_t i = 0; i < (int)length; i++) {
        const Fixed24 divI = ((Fixed24)i*reciprocalOfLength);
        lineEquation textureLine = {(((Fixed24)1-divI)*lineEquations[3].px)+(divI*lineEquations[3].cx), (((Fixed24)1-divI)*lineEquations[1].cx)+(divI*lineEquations[1].px), (((Fixed24)1-divI)*lineEquations[3].py)+(divI*lineEquations[3].cy), (((Fixed24)1-divI)*lineEquations[1].cy)+(divI*lineEquations[1].py)};
        Fixed24 dx = (textureLine.cx - textureLine.px).abs();
        Fixed24 dy = (textureLine.cy - textureLine.py).abs();
        int row = ((Fixed24)i*reciprocalOfTextureRatio).floor()*16;
        Fixed24 x1;
        Fixed24 x2 = textureLine.cx;
        Fixed24 y1;
        Fixed24 y2 = textureLine.cy;
        const Fixed24 xDiff = ((((Fixed24)1-reciprocalOf16)*textureLine.cx) + (reciprocalOf16*textureLine.px)) - textureLine.cx;
        const Fixed24 yDiff = ((((Fixed24)1-reciprocalOf16)*textureLine.cy) + (reciprocalOf16*textureLine.py)) - textureLine.cy;
        for (uint8_t a = 0; a < 16; a++) {
            gfx_SetColor(polygon->texture[row + a]);
            x1 = x2;
            y1 = y2;
            x2 += xDiff;
            y2 += yDiff;
            int lineX1 = x1;
            int lineX2 = x2;
            int lineY1 = y1;
            int lineY2 = y2;
            // this'll do, but I'd like a better way of fixing the white pixels than *blindly* overdrawing
            if ((int)length > 32) {
                if (lineX1 >= 0 && lineX1 < 320 && lineX2 >= 0 && lineX2 < 320 && lineY1 >= 0 && lineY1 < 240 && lineY2 > 0 && lineY2 < 240) {
                    gfx_Line_NoClip(lineX1, lineY1, lineX2, lineY2);
                    gfx_Line_NoClip(lineX1, lineY1, lineX2, lineY2-1);
                } else {
                    gfx_Line(lineX1, lineY1, lineX2, lineY2);
                    gfx_Line(lineX1, lineY1, lineX2, lineY2-1);
                }
            } else {
                if (lineX1 >= 0 && lineX1 < 320 && lineX2 >= 0 && lineX2 < 320 && lineY1 >= 0 && lineY1 < 240 && lineY2 > 0 && lineY2 < 240) {
                    gfx_Line_NoClip(lineX1, lineY1, lineX2, lineY2 - 1);
                } else {
                    gfx_Line(lineX1, lineY1, lineX2, lineY2 - 1);
                }
            }
            
        }
    }
    gfx_SetColor(0);
    // drawing polygon outlines -- not necessary for textured polygons
    /*for (unsigned int i = 0; i < 4; i++) {
        screenPoint* point1 = &sourceObject->renderedPoints[lines[i].point1];
        screenPoint* point2 = &sourceObject->renderedPoints[lines[i].point2];
        if (point1->x >= 0 && point1->x <= 319 && point1->y >= 0 && point1->y <= 239 && point2->x >= 0 && point2->x <= 319 && point2->y >= 0 && point2->y <= 239) {
            gfx_Line_NoClip(point1->x, point1->y, point2->x, point2->y);
        } else {
            gfx_Line(point1->x, point1->y, point2->x, point2->y);
        }
        
    } */
}

Fixed24 getPointDistance(point point) {
    const Fixed24 x = point.x - cameraXYZ[0];
    const Fixed24 y = point.y - cameraXYZ[1];
    const Fixed24 z = point.z - cameraXYZ[2];
    return sqrtf(powf(x, 2) + powf(y, 2) + powf(z, 2));
    // doesn't need to be inverse of inverse sqrt, it's just convinent (but slow);
    //return 1/q_rsqrt(powf(x, 2) + powf(y, 2) + powf(z, 2));
}