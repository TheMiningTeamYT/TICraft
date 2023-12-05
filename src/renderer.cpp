#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ti/getcsc.h>
#include <graphx.h>
#include "renderer.hpp"
#include "textures.hpp"
#include "saves.hpp"
/*
feature add ideas:
Lighting:
still thinking about how to do it
*/

object** objects = (object**) 0xD3E000;
object** zSortedObjects = objects + maxNumberOfObjects;

// Buffer for creating diagnostic strings
char buffer[200];

unsigned int numberOfObjects = 0;

unsigned int outOfBoundsPolygons = 0;

unsigned int obscuredPolygons = 0;

// Faces of a cube
uint8_t face5points[] = {7, 6, 2, 3};
const polygon face5 = {face5points, 5};
uint8_t face4points[] = {5, 4, 7, 6};
const polygon face4 = {face4points, 4};
uint8_t face3points[] = {1, 5, 6, 2};
const polygon face3 = {face3points, 3};
uint8_t face2Points[] = {0, 1, 2, 3};
const polygon face2 = {face2Points, 2};
uint8_t face1Points[] = {4, 0, 3, 7};
const polygon face1 = {face1Points, 1};
uint8_t face0Points[] = {4, 5, 1, 0};
const polygon face0 = {face0Points, 0};

/*
Array of the polygons that make up a cube
Unwrapping order:
top, front, left, right, back, bottom
*/ 
const polygon cubePolygons[] = {face0, face1, face2, face3, face4, face5};

// Position of the camera
Fixed24 cameraXYZ[3] = {-100, 150, -100};

float angleX = 30;
float angleY = 45;
float degRadRatio = M_PI/180;

Fixed24 cx = 0.8660254037844387f;
Fixed24 sx = 0.49999999999999994f;
Fixed24 cy = 0.7071067811865476f;
Fixed24 sy = 0.7071067811865476f;

/*
clip:
clip = 0 // not affected by clipping
clip = 1 // affected by clipping, no z buffer
clip = 2 // affected by clipping, write to z buffer
*/
void object::generatePolygons(bool clip) {
    // A local copy of the polygons of a cube (Not sure this is necessary)
    polygon polygons[6];

    // Rendering the object (Preparing polygons for rendering)
    if (visible) {
        // Init local copy of polygons
        memcpy(polygons, cubePolygons, sizeof(polygon)*6);
        for (uint8_t polygonNum = 0; polygonNum < 6; polygonNum++) {
            // Set the polygon's distance from the camera
            polygon* polygon = &polygons[polygonNum];
            int z = 0;
            int totalX = 0;
            int totalY = 0;
            for (uint8_t i = 0; i < 4; i++) {
                totalX += renderedPoints[polygon->points[i]].x;
                totalY += renderedPoints[polygon->points[i]].y;
                z += renderedPoints[polygon->points[i]].z;
            }
            totalX /= 4;
            totalY /= 4;
            z /= 4;
            polygon->x = totalX;
            polygon->y = totalY;
            polygon->z = z;
        }
        // Sort the polygons by distance from the camera, front to back
        qsort((void *) polygons, 6, sizeof(polygon), zCompare);

        // Prepare the polygons for rendering
        for (uint8_t polygonNum = 0; polygonNum < 6; polygonNum++) {
            // The polygon we are rendering
            polygon polygon = polygons[polygonNum];

            // Are we going to render the polygon?
            bool render = true;
            
            // Normalized z (0-255)
            uint16_t normalizedZ = (polygon.z / 8);
            if (polygon.z < 12) {
                render = false;
            } else {
                // If there are any other polygons this one could be overlapping with, check the z buffer
                // The z culling still has problems
                if (clip == true) {
                    // Get the average x & y of the polygon
                    int x = polygon.x;
                    int y = polygon.y;

                    uint8_t obscuredPoints = 0;
                    for (uint8_t i = 0; i < 4; i++) {
                        screenPoint* point = &renderedPoints[polygon.points[i]];
                        if (point->x < 0 || point->x > 319 || point->y < 0 || point->y > 239) {
                            obscuredPoints++;
                        } else {
                            int pointX = (point->x + x)>>1;
                            int pointY = (point->y + y)>>1;
                            if (!(pointX < 0 || pointX > 319 || pointY < 0 || pointY > 239)) {
                                uint8_t bufferZ = gfx_GetPixel(pointX, pointY);
                                if (normalizedZ >= bufferZ) {
                                    obscuredPoints++;
                                }
                            }
                        }
                    }
                    if (obscuredPoints == 4) {
                        render = false;
                        #if diagnostics == true
                        obscuredPolygons++;
                        #endif
                    }
                }
            }

            // Prepare this polygon for rendering
            if (render) {
                // Draw the polygon to the z buffer (just the screen)
                // For the transparent textures (glass & leaves), if the zBuffer has been cleared out (set to 255) around the glass, DON't draw to the zBuffer.
                // Otherwise, do
                // This is to avoid situations where either the glass doesn't draw to the zBuffer in a partial redraw, breaking the partial redraw engine,
                // or cases where it draws to the zBuffer in a partial redraw with it shouldn't, resulting in blank space behind the glass.

                // Create a new transformed (prepared) polygon and set initialize it;
                transformedPolygon preparedPolygon = {this, normalizedZ, polygon.polygonNum};
                renderPolygon(preparedPolygon);
            }
        }
    }
}

void object::deleteObject() {
    if (visible) {
        for (uint8_t polygonNum = 0; polygonNum < 6; polygonNum++) {
            gfx_SetDrawBuffer();
            gfx_SetColor(255);
            /*int x1 = renderedPoints[cubePolygons[polygonNum].points[0]].x;
            int x2 = renderedPoints[cubePolygons[polygonNum].points[1]].x;
            int x3 = renderedPoints[cubePolygons[polygonNum].points[2]].x;
            int x4 = renderedPoints[cubePolygons[polygonNum].points[3]].x;
            int y1 = renderedPoints[cubePolygons[polygonNum].points[0]].y;
            int y2 = renderedPoints[cubePolygons[polygonNum].points[1]].y;
            int y3 = renderedPoints[cubePolygons[polygonNum].points[2]].y;
            int y4 = renderedPoints[cubePolygons[polygonNum].points[3]].y;
            gfx_FillTriangle_NoClip(x1, y1, x2, y2, x3, y3);
            gfx_FillTriangle_NoClip(x1, y1, x4, y4, x3, y3);
            gfx_SetDrawScreen();
            gfx_FillTriangle_NoClip(x1, y1, x2, y2, x3, y3);
            gfx_FillTriangle_NoClip(x1, y1, x4, y4, x3, y3);
            gfx_SetDrawBuffer();*/
            int16_t minX = renderedPoints[cubePolygons[polygonNum].points[0]].x;
            int16_t minY = renderedPoints[cubePolygons[polygonNum].points[0]].y;
            int16_t maxX = renderedPoints[cubePolygons[polygonNum].points[0]].x;
            int16_t maxY = renderedPoints[cubePolygons[polygonNum].points[0]].y;
            for (uint8_t i = 1; i < 4; i++) {
                if (renderedPoints[cubePolygons[polygonNum].points[i]].x < minX) {
                    minX = renderedPoints[cubePolygons[polygonNum].points[i]].x;
                } else if (renderedPoints[cubePolygons[polygonNum].points[i]].x > maxX) {
                    maxX = renderedPoints[cubePolygons[polygonNum].points[i]].x;
                }
                if (renderedPoints[cubePolygons[polygonNum].points[i]].y < minY) {
                    minY = renderedPoints[cubePolygons[polygonNum].points[i]].y;
                } else if (renderedPoints[cubePolygons[polygonNum].points[i]].y > maxY) {
                    maxY = renderedPoints[cubePolygons[polygonNum].points[i]].y;
                }
            }
            minX -= 1;
            maxX += 1;
            minY -= 1;
            maxY += 1;
            gfx_FillRectangle(minX, minY, (maxX-minX), (maxY-minY));
            gfx_SetDrawScreen();
            gfx_FillRectangle(minX, minY, (maxX-minX), (maxY-minY));
        }
    }
}

void object::generatePoints() {
    visible = true;
    uint8_t nonVisiblePoints = 0;
    // The verticies of the cube
    point points[] = {{x, y, z}, {x+size, y, z}, {x+size, y-size, z}, {x, y-size, z}, {x, y, z+size}, {x+size, y, z+size}, {x+size, y-size, z+size}, {x, y-size, z+size}};;
    for (unsigned int i = 0; i < 8; i++) {
        Fixed24 offset = 0;
        if (cameraXYZ[1] < (Fixed24)40) {
            offset = (Fixed24)20;
        }
        if ((angleY >= -22.5 && angleY <= 22.5)) {
            if (z + offset < cameraXYZ[2]) {
                visible = false;
                renderedPoints[0].z = getPointDistance(points[0]);
                break;
            }
        } else if (angleY >= 67.5) {
            if (x + offset < cameraXYZ[0]) {
                visible = false;
                renderedPoints[0].z = getPointDistance(points[0]);
                break;
            }
        } else if (angleY > 22.5 && angleY < 67.5) {
            if ((x + z) + offset < (cameraXYZ[0] + cameraXYZ[2])) {
                visible = false;
                renderedPoints[0].z = getPointDistance(points[0]);
                break;
            }
        } else if (angleY <= -67.5) {
            if (x > cameraXYZ[0] + offset) {
                visible = false;
                renderedPoints[0].z = getPointDistance(points[0]);
                break;
            }
        } else if (angleY < -22.5 && angleY > -67.5) {
            if ((x - z) > (cameraXYZ[0] - cameraXYZ[2]) + offset) {
                visible = false;
                renderedPoints[0].z = getPointDistance(points[0]);
                break;
            }
        }
    }

    if (visible) {
        for (uint8_t i = 0; i < 8; i++) {
            renderedPoints[i] = transformPointNewA(points[i]);
            if (renderedPoints[i].z > zCullingDistance || renderedPoints[i].x < -200 || renderedPoints[i].x > 519 || renderedPoints[i].y < -200 || renderedPoints[i].y > 439) {
                visible = false;
                break;
            }
            if (renderedPoints[i].x < 0 || renderedPoints[i].x > 319 || renderedPoints[i].y < 0 || renderedPoints[i].y > 239) {
                nonVisiblePoints++;
            }
        }
        if (nonVisiblePoints == 8) {
            visible = false;
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

int object::getDistance() {
    if (renderedPoints[0].z != 0) {
        return renderedPoints[0].z;
    }
    renderedPoints[0].z = getPointDistance({x, y, z});
    return renderedPoints[0].z;
}

int zCompare(const void *arg1, const void *arg2) {
    polygon* polygon1 = (polygon *) arg1;
    polygon* polygon2 = (polygon *) arg2;
    return polygon1->z - polygon2->z;
}

int distanceCompare(const void *arg1, const void *arg2) {
    object* object1 = *((object**) arg1);
    object* object2 = *((object**) arg2);
    return object1->getDistance() - object2->getDistance();
}

int xCompare(const void *arg1, const void *arg2) {
    object* object1 = *((object**) arg1);
    object* object2 = *((object**) arg2);
    /*snprintf(buffer, 200, "XComp X1: %f, X2: %f", (float)object1->x, (float)object2->x);
    gfx_PrintString(buffer);*/
    return object1->x - object2->x;
}

void deleteEverything() {
    // deletePolygons();
    for (unsigned int i = 0; i < numberOfObjects; i++) {
        if (objects[i]) {
            delete objects[i];
        }
    }
    numberOfObjects = 0;
}

void drawScreen(bool fullRedraw) {
    if (fullRedraw) {
        gfx_FillScreen(255);
    }
    gfx_SetDrawBuffer();
    if (fullRedraw) {
        gfx_SetTextXY(0, 0);
        // Clear the screen
        gfx_FillScreen(255);
    }
    # if diagnostics == true
    outOfBoundsPolygons = 0;
    obscuredPolygons = 0;
    #endif
    for (unsigned int i = 0; i < numberOfObjects; i++) {
        if (zSortedObjects[i]->texture != 15 && zSortedObjects[i]->texture != 23) {
            zSortedObjects[i]->generatePolygons(true);
        }
    }
    for (unsigned int i = 0; i < numberOfObjects; i++) {
        if (zSortedObjects[i]->texture == 15 || zSortedObjects[i]->texture == 23) {
            zSortedObjects[i]->generatePolygons(true);
        }
    }
    gfx_SetDrawScreen();

    // Print some diagnostic information
    #if diagnostics == true
    if (mode == 0) {
        snprintf(buffer, 200, "Out of bounds polygons: %u", outOfBoundsPolygons);
        gfx_PrintString(buffer);
        gfx_SetTextXY(0, gfx_GetTextY() + 10);
        snprintf(buffer, 200, "Obscured Polygons: %u", obscuredPolygons);
        gfx_PrintString(buffer);
        gfx_SetTextXY(0, gfx_GetTextY() + 10);
        snprintf(buffer, 200, "Total Polygons: %u", numberOfPreparedPolygons);
        gfx_PrintString(buffer);
        gfx_SetTextXY(0, gfx_GetTextY() + 10);
        snprintf(buffer, 200, "Object Size: %u", sizeof(object));
        gfx_PrintString(buffer);
        gfx_SetTextXY(0, gfx_GetTextY() + 10);
        snprintf(buffer, 200, "Polygon Size: %u", sizeof(transformedPolygon));
        gfx_PrintString(buffer);
        gfx_SetTextXY(0, gfx_GetTextY() + 10);
    }
    #endif
}

// implementation of affine texture mapping (i think thats what you'd call this anyway)
// as noted in The Science Elf's original video, affine texture mapping can look very weird on triangles
// but on quads, it looks pretty good at a fraction of the cost
void renderPolygon(transformedPolygon polygon) {
    // Quick shorthand
    uint8_t* points = cubePolygons[polygon.polygonNum].points;
    // Another useful shortand
    object* sourceObject = polygon.object;
    // Another useful shorthand
    screenPoint renderedPoints[] = {sourceObject->renderedPoints[points[0]], sourceObject->renderedPoints[points[1]], sourceObject->renderedPoints[points[2]], sourceObject->renderedPoints[points[3]]};
    // result of memory optimization
    const uint8_t* texture = textures[sourceObject->texture][polygon.polygonNum];

    // uint8_t* points = polygon.points;
    if (sourceObject->outline) {
        gfx_SetColor(outlineColor);
        for (uint8_t i = 0; i < 4; i++) {
            uint8_t nextPoint = i + 1;
            if (nextPoint > 3) {
                nextPoint = 0;
            }
            gfx_Line(renderedPoints[i].x, renderedPoints[i].y, renderedPoints[nextPoint].x, renderedPoints[nextPoint].y);
        }
    } else {
        bool clipLines = false;
        for (unsigned int i = 0; i < 4; i++) {
            if (renderedPoints[i].x < 1 || renderedPoints[i].x > 318 || renderedPoints[i].y < 1 || renderedPoints[i].y > 238) {
                clipLines = true;
                break;
            }
        }
        uint8_t colorOffset = 0;
        if (polygon.polygonNum == 2 || polygon.polygonNum == 5) {
            colorOffset = 126;
        }
        // Generate the lines from the points of the polygon
        Fixed24 dx1 = renderedPoints[3].x - renderedPoints[0].x;
        Fixed24 dy1 = renderedPoints[3].y - renderedPoints[0].y;
        Fixed24 dx2 = renderedPoints[2].x - renderedPoints[1].x;
        Fixed24 dy2 = renderedPoints[2].y - renderedPoints[1].y;
        int length1 = (abs(dx1) > abs(dy1)) ? abs(dx1) : abs(dy1);
        int length2 = (abs(dx2) > abs(dy2)) ? abs(dx2) : abs(dy2);
        int length = (length1 > length2) ? length1 : length2;

        // Ratios that make the math faster (means we can multiply instead of divide)
        const Fixed24 reciprocalOfLength = (Fixed24)1/(Fixed24)length;
        
        Fixed24 textureRatio = reciprocalOfLength;
        textureRatio.n <<=4;
        Fixed24 row = 0;
        Fixed24 lineCX = renderedPoints[0].x;
        Fixed24 linePX = renderedPoints[1].x;
        Fixed24 lineCY = renderedPoints[0].y;
        Fixed24 linePY = renderedPoints[1].y;
        Fixed24 CXdiff = dx1*reciprocalOfLength;
        Fixed24 CYdiff = dy1*reciprocalOfLength;
        Fixed24 PXdiff = dx2*reciprocalOfLength;
        Fixed24 PYdiff = dy2*reciprocalOfLength;

        // main body
        if (clipLines) {
            for (int i = 0; i < length; i++) {
                if (row > (Fixed24)15) {
                    row = 15;
                    textureRatio = 0;
                }
                if (!((lineCX < (Fixed24)0 && linePX < (Fixed24)0) || (lineCX > (Fixed24) 319 && linePX > (Fixed24)319)) && !((lineCY < (Fixed24)0 && linePY < (Fixed24)0) || (lineCY > (Fixed24) 239 && linePY > (Fixed24)239)))
                    drawTextureLineNewA(lineCX, linePX, lineCY, linePY, &texture[((int)row)*16], colorOffset, polygon.z);
                lineCX += CXdiff;
                linePX += PXdiff;
                lineCY += CYdiff;
                linePY += PYdiff;
                row += textureRatio;
            }
        } else {
            for (int i = 0; i < length; i++) {
                if (row > (Fixed24)15) {
                    row = 15;
                    textureRatio = 0;
                }
                drawTextureLineNewA_NoClip(lineCX, linePX, lineCY, linePY, &texture[((int)row)*16], colorOffset, polygon.z);
                lineCX += CXdiff;
                linePX += PXdiff;
                lineCY += CYdiff;
                linePY += PYdiff;
                row += textureRatio;
            }
        }
        // last line
        if (clipLines) {
            drawTextureLineNewA(lineCX, linePX, lineCY, linePY, &texture[240], colorOffset, polygon.z);
        } else {
            drawTextureLineNewA_NoClip(lineCX, linePX, lineCY, linePY, &texture[240], colorOffset, polygon.z);
        }
    }
}

int getPointDistance(point point) {
    int x = point.x - cameraXYZ[0];
    int y = point.y - cameraXYZ[1];
    int z = point.z - cameraXYZ[2];
    return approx_sqrt_a((x*x)+(y*y)+(z*z));
}

object** xSearch(object* key) {
    return (object**) bsearch((void*) key, objects, numberOfObjects, sizeof(object *), xCompare);
}

void xSort() {
    qsort(objects, numberOfObjects, sizeof(object*), xCompare);
    memcpy(zSortedObjects, objects, sizeof(object*) * numberOfObjects);
    // Sort all objects front to back
    // improves polygon culling but sorting can be slow
    qsort(zSortedObjects, numberOfObjects, sizeof(object*), distanceCompare);
}

unsigned char bit_width(unsigned x) {
    return x == 0 ? 1 : 24 - __builtin_clz(x);
}

void rotateCamera(float x, float y) {
    bool cameraRotated = false;
    if ((x != 0.0f) && (angleX + x >= -85.0f && angleX + x <= 85.0f)) {
        angleX += x;
        cx = cosf(angleX*degRadRatio);
        sx = sinf(angleX*degRadRatio);
        cameraRotated = true;
    }
    if ((y != 0.0f) && (angleY + y >= -90.0f && angleY + y <= 90.0f)) {
        angleY += y;
        cy = cosf(angleY*degRadRatio);
        sy = sinf(angleY*degRadRatio);
        cameraRotated = true;
    }
    if (cameraRotated) {
        for (unsigned int i = 0; i < numberOfObjects; i++) {
            objects[i]->generatePoints();
        }
        drawScreen(true);
        getBuffer();
        drawCursor();
    }
}

void resetCamera() {
    angleX = 30;
    angleY = 45;
    cx = 0.8660254037844387f;
    sx = 0.49999999999999994f;
    cy = 0.7071067811865476f;
    sy = 0.7071067811865476f;
    cameraXYZ[0] = -100;
    cameraXYZ[1] = 150;
    cameraXYZ[2] = -100;
}