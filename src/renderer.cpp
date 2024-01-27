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
optimization ideas:
I suspect the qsort when generating polygons is taking a while, or at least is taking a not insignificant amount of time.
What I could do is go back to having a list of every polygon to render, but instead of culling the polygons *before* adding them to the list, do so after.
That way we can just do 1 qsort on a larger list
but actually it seems that'd be slower
Get rid of the x sorted objects and replace it with z sorted objects
*/
/*
feature add ideas:
Lighting:
still thinking about how to do it
*/
object** objects;
object** zSortedObjects;

// Buffer for creating diagnostic strings
char buffer[200];

unsigned int numberOfObjects = 0;

unsigned int outOfBoundsPolygons = 0;

unsigned int obscuredPolygons = 0;

// Faces of a cube
uint8_t face5points[] = {7, 6, 2, 3};
polygon face5 = {face5points, 5};
uint8_t face4points[] = {5, 4, 7, 6};
polygon face4 = {face4points, 4};
uint8_t face3points[] = {1, 5, 6, 2};
polygon face3 = {face3points, 3};
uint8_t face2Points[] = {0, 1, 2, 3};
polygon face2 = {face2Points, 2};
uint8_t face1Points[] = {4, 0, 3, 7};
polygon face1 = {face1Points, 1};
uint8_t face0Points[] = {4, 5, 1, 0};
polygon face0 = {face0Points, 0};

/*
Array of the polygons that make up a cube
Unwrapping order:
top, front, left, right, back, bottom
*/ 
polygon cubePolygons[] = {face0, face1, face2, face3, face4, face5};

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
void object::generatePolygons() {
    // Rendering the object (Preparing polygons for rendering)
    if (visible) {
        for (uint8_t polygonNum = 0; polygonNum < 6; polygonNum++) {
            // Set the polygon's distance from the camera
            polygon* polygon = &cubePolygons[polygonNum];
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
        qsort(cubePolygons, 6, sizeof(polygon), zCompare);

        // Prepare the polygons for rendering
        for (uint8_t polygonNum = 0; polygonNum < 6; polygonNum++) {
            gfx_SetDrawBuffer();
            // The polygon we are rendering
            polygon* polygon = &cubePolygons[polygonNum];
            // Normalized z (0-255)
            unsigned int normalizedZ = (polygon->z >> 3);

            // Are we going to render the polygon?
            bool render = false;
            if (outline) {
                render = true;
                goto renderThePolygon;
            }
            if (polygon->z >= 20) {
                // If there are any other polygons this one could be overlapping with, check the z buffer
                // The z culling still has problems
                if (!outline) {
                    // Get the average x & y of the polygon
                    int x = polygon->x;
                    int y = polygon->y;

                    if (x >= 0 && x < GFX_LCD_WIDTH && y >= 0 && y < GFX_LCD_HEIGHT) {
                        if (normalizedZ < gfx_GetPixel(x, y)) {
                            render = true;
                            goto renderThePolygon;
                        }
                    }
                    for (uint8_t i = 0; i < 4; i++) {
                        screenPoint* point = &renderedPoints[polygon->points[i]];
                        int pointX = (point->x + point->x + point->x + point->x + point->x + point->x + point->x + x)>>3;
                        int pointY = (point->y + point->y + point->y + point->y + point->y + point->y + point->y + y)>>3;
                        if (pointX >= 0 && pointX < GFX_LCD_WIDTH && pointY >= 0 && pointY < GFX_LCD_HEIGHT) {
                            if (normalizedZ < gfx_GetPixel(pointX, pointY)) {
                                render = true;
                                break;
                            }
                        }
                    }
                }
            }
            renderThePolygon:
            // Prepare this polygon for rendering
            if (render) {
                // Draw the polygon to the z buffer (just the screen)
                // For the transparent textures (glass & leaves), if the zBuffer has been cleared out (set to 255) around the glass, DON't draw to the zBuffer.
                // Otherwise, do
                // This is to avoid situations where either the glass doesn't draw to the zBuffer in a partial redraw, breaking the partial redraw engine,
                // or cases where it draws to the zBuffer in a partial redraw with it shouldn't, resulting in blank space behind the glass.

                renderPolygon(this, polygon, normalizedZ);
            }
            #if diagnostics == true
            else {
                obscuredPolygons++;
            }
            #endif
        }
        gfx_SetDrawScreen();
    }
}

void object::deleteObject() {
    if (visible) {
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
        int minX = renderedPoints[0].x;
        int minY = renderedPoints[0].y;
        int maxX = renderedPoints[0].x;
        int maxY = renderedPoints[0].y;
        for (uint8_t i = 1; i < 8; i++) {
            if (renderedPoints[i].x < minX) {
                minX = renderedPoints[i].x;
            } else if (renderedPoints[i].x > maxX) {
                maxX = renderedPoints[i].x;
            }
            if (renderedPoints[i].y < minY) {
                minY = renderedPoints[i].y;
            } else if (renderedPoints[i].y > maxY) {
                maxY = renderedPoints[i].y;
            }
        }
        minX -= 1;
        maxX += 1;
        minY -= 1;
        maxY += 1;
        gfx_FillRectangle(minX, minY, (maxX-minX), (maxY-minY));
        gfx_SetDrawBuffer();
        gfx_FillRectangle(minX, minY, (maxX-minX), (maxY-minY));
        for (unsigned int i = 0; i < numberOfObjects; i++) {
            object* otherObject = zSortedObjects[i];
            if (otherObject != this) {
                for (uint8_t pointNum = 0; pointNum < 8; pointNum++) {
                    screenPoint point = otherObject->renderedPoints[pointNum];
                    if (point.x >= minX && point.x <= maxX && point.y >= minY && point.y <= maxY) {
                        otherObject->generatePolygons();
                        break;
                    }
                }
            }
        }
        gfx_SetDrawScreen();
        delete this;
    }
}

// bugs with inconsistent behavior be here.
// how... I'm not sure
// sometimes it works fine, sometimes it fails spectacularly.
// and I will never know why.
void object::generatePoints() {
    // i want to rewrite this in assembly
    // i have... hmm... zero faith in the compiler
    visible = false;

    const Fixed24 x1 = x - cameraXYZ[0];
    const Fixed24 x2 = x1 + (Fixed24)cubeSize;
    const Fixed24 y1 = y - cameraXYZ[1];
    const Fixed24 y2 =  y1 - (Fixed24)cubeSize;
    const Fixed24 z1 = z - cameraXYZ[2];
    const Fixed24 z2 = z1 + (Fixed24)cubeSize;

    if (angleX >= 45 && y1 > (Fixed24)0) {
        return;
    }

    const int x1squared = (int)x1*(int)x1;
    const int y1squared = (int)y1*(int)y1;
    const int z1squared = (int)z1*(int)z1;
    renderedPoints[0].z = approx_sqrt_a(x1squared + y1squared + z1squared);
    if (renderedPoints[0].z > zCullingDistance) {
        return;
    }

    const Fixed24 cyz1 = cy*z1;
    const Fixed24 syx1 = sy*x1;
    Fixed24 sum1 = cyz1 + syx1;
    const Fixed24 nsxy1 = -sx*y1;
    Fixed24 dz = (cx*sum1) + nsxy1;
    if (dz <= (Fixed24)20) {
        return;
    }

    const Fixed24 cyx1 = cy*x1;
    const Fixed24 syz1 = sy*z1;
    Fixed24 dx = cyx1 - syz1;
    if (dz + (Fixed24)20 < (dx).abs()) {
        return;
    }

    const int x2squared = (int)x2*(int)x2;
    const int y2squared = (int)y2*(int)y2;
    const int z2squared = (int)z2*(int)z2;

    const Fixed24 cyx2 = cy*x2;
    const Fixed24 syx2 = sy*x2;
    const Fixed24 cxy1 = cx*y1;
    const Fixed24 cxy2 = cx*y2;
    const Fixed24 nsxy2 = -sx*y2;
    const Fixed24 cyz2 = cy*z2;
    const Fixed24 syz2 = sy*z2;

    Fixed24 dy = (sx*sum1) + cxy1;
    if (dz > (Fixed24)40 && dy.abs() <= ((Fixed24)0.7002075382f)*dz) {
        visible = true;
    }
    Fixed24 sum2 = ((Fixed24)171.3777608f)/dz;
    renderedPoints[0] = {(int16_t)((int)(sum2*dx)+160), (int16_t)(120-(int)(sum2*dy))};

    dy = (sx*sum1) + cxy2;
    dz = (cx*sum1) + nsxy2;
    if (!visible) {
        if (dz > (Fixed24)40 && dy.abs() <= ((Fixed24)0.7002075382f)*dz) {
            visible = true;
        }
    }
    sum2 = ((Fixed24)171.3777608f)/dz;
    renderedPoints[3] = {(int16_t)((int)(sum2*dx)+160), (int16_t)(120-(int)(sum2*dy)), approx_sqrt_a(x1squared + y2squared + z1squared)};

    dx = cyx2 - syz1;
    sum1 = cyz1 + syx2;
    dy = (sx*sum1) + cxy1;
    dz = (cx*sum1) + nsxy1;
    if (!visible) {
        if (dz > (Fixed24)40 && dz >= (dx).abs() && dy.abs() <= ((Fixed24)0.7002075382f)*dz) {
            visible = true;
        }
    }
    sum2 = ((Fixed24)171.3777608f)/dz;
    renderedPoints[1] = {(int16_t)((int)(sum2*dx)+160), (int16_t)(120-(int)(sum2*dy)), approx_sqrt_a(x2squared + y1squared + z1squared)};

    dy = (sx*sum1) + cxy2;
    dz = (cx*sum1) + nsxy2;
    if (!visible) {
        if (dz > (Fixed24)40 && dz >= (dx).abs() && dy.abs() <= ((Fixed24)0.7002075382f)*dz) {
            visible = true;
        }
    }
    sum2 = ((Fixed24)171.3777608f)/dz;
    renderedPoints[2] = {(int16_t)((int)(sum2*dx)+160), (int16_t)(120-(int)(sum2*dy)), approx_sqrt_a(x2squared + y2squared + z1squared)};
    
    dx = cyx1 - syz2;
    sum1 = cyz2 + syx1;
    dy = (sx*sum1) + cxy1;
    dz = (cx*sum1) + nsxy1;
    if (!visible) {
        if (dz > (Fixed24)40 && dz >= (dx).abs() && dy.abs() <= ((Fixed24)0.7002075382f)*dz) {
            visible = true;
        }
    }
    sum2 = ((Fixed24)171.3777608f)/dz;
    renderedPoints[4] = {(int16_t)((int)(sum2*dx)+160), (int16_t)(120-(int)(sum2*dy)), approx_sqrt_a(x1squared + y1squared + z2squared)};

    dy = (sx*sum1) + cxy2;
    dz = (cx*sum1) + nsxy2;
    if (!visible) {
        if (dz > (Fixed24)40 && dz >= (dx).abs() && dy.abs() <= ((Fixed24)0.7002075382f)*dz) {
            visible = true;
        }
    }
    sum2 = ((Fixed24)171.3777608f)/dz;
    renderedPoints[7] = {(int16_t)((int)(sum2*dx)+160), (int16_t)(120-(int)(sum2*dy)), approx_sqrt_a(x1squared + y2squared + z2squared)};

    dx = cyx2 - syz2;
    sum1 = cyz2 + syx2;
    dy = (sx*sum1) + cxy1;
    dz = (cx*sum1) + nsxy1;
    if (!visible) {
        if (dz > (Fixed24)40 && dz >= (dx).abs() && dy.abs() <= ((Fixed24)0.7002075382f)*dz) {
            visible = true;
        }
    }
    sum2 = ((Fixed24)171.3777608f)/dz;
    renderedPoints[5] = {(int16_t)((int)(sum2*dx)+160), (int16_t)(120-(int)(sum2*dy)), approx_sqrt_a(x2squared + y1squared + z2squared)};

    dy = (sx*sum1) + cxy2;
    dz = (cx*sum1) + nsxy2;
    if (!visible) {
        if (dz > (Fixed24)40 && dz >= (dx).abs() && dy.abs() <= ((Fixed24)0.7002075382f)*dz) {
            visible = true;
        }
    }
    sum2 = ((Fixed24)171.3777608f)/dz;
    renderedPoints[6] = {(int16_t)((int)(sum2*dx)+160), (int16_t)(120-(int)(sum2*dy)), approx_sqrt_a(x2squared + y2squared + z2squared)};
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
    bool object1Transparent = (object1->texture == 15 || object1->texture == 23);
    bool object2Transparent = (object2->texture == 15 || object2->texture == 23);
    if (object1Transparent && !object2Transparent) {
        return 1;
    }
    if (object2Transparent && !object1Transparent) {
        return -1;
    }
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
    for (unsigned int i = 0; i < numberOfObjects; i++) {
        if (objects[i]) {
            delete objects[i];
        }
    }
    numberOfObjects = 0;
}

void drawScreen(bool fullRedraw) {
    if (fullRedraw) {
        gfx_SetDrawBuffer();
        gfx_SetTextXY(0, 0);
        // Clear the zBuffer
        gfx_FillScreen(255);
        gfx_SetDrawScreen();
        // Clear the screen
        gfx_FillScreen(255);
    }
    #if diagnostics == true
    outOfBoundsPolygons = 0;
    obscuredPolygons = 0;
    #endif
    for (unsigned int i = 0; i < numberOfObjects; i++) {
        zSortedObjects[i]->generatePolygons();
    }

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
// idea: integrate this into generatePolygons instead of having it as a seperate function
/*
void renderPolygon(object* sourceObject, polygon* preparedPolygon, unsigned int normalizedZ) {
    // Quick shorthand
    #define points preparedPolygon->points
    // Another useful shorthand
    screenPoint renderedPoints[] = {sourceObject->renderedPoints[points[0]], sourceObject->renderedPoints[points[1]], sourceObject->renderedPoints[points[2]], sourceObject->renderedPoints[points[3]]};
    // result of memory optimization
    const uint8_t* texture = textures[sourceObject->texture][preparedPolygon->polygonNum];

    // uint8_t* points = polygon.points;
    if (sourceObject->outline) {
        gfx_SetDrawScreen();
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
            if (renderedPoints[i].x < 0 || renderedPoints[i].x > GFX_LCD_WIDTH - 2 || renderedPoints[i].y < 0 || renderedPoints[i].y > GFX_LCD_HEIGHT - 1) {
                clipLines = true;
                break;
            }
        }
        uint8_t colorOffset = 0;
        if (preparedPolygon->polygonNum == 2 || preparedPolygon->polygonNum == 5) {
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
        length++;

        // Ratios that make the math faster (means we can multiply instead of divide)
        const Fixed24 reciprocalOfLength = (Fixed24)1/(Fixed24)length;
        
        Fixed24 textureRatio = reciprocalOfLength;
        textureRatio.n <<=4;
        textureRatio.n--;
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
                if (!((lineCX < (Fixed24)0 && linePX < (Fixed24)0) || (lineCX > (Fixed24)(GFX_LCD_WIDTH - 1) && linePX > (Fixed24)(GFX_LCD_WIDTH - 1))) && !((lineCY < (Fixed24)0 && linePY < (Fixed24)0) || (lineCY > (Fixed24)(GFX_LCD_HEIGHT - 1) && linePY > (Fixed24)(GFX_LCD_HEIGHT - 1))))
                    drawTextureLineNewA(lineCX, linePX, lineCY, linePY, &texture[(row.floor())*16], colorOffset, normalizedZ);
                lineCX += CXdiff;
                linePX += PXdiff;
                lineCY += CYdiff;
                linePY += PYdiff;
                row += textureRatio;
            }
        } else {
            for (int i = 0; i < length; i++) {
                drawTextureLineNewA_NoClip(lineCX, linePX, lineCY, linePY, &texture[(row.floor())*16], colorOffset, normalizedZ);
                lineCX += CXdiff;
                linePX += PXdiff;
                lineCY += CYdiff;
                linePY += PYdiff;
                row += textureRatio;
            }
        }
        if (clipLines) {
            drawTextureLineNewA(lineCX, linePX, lineCY, linePY, &texture[240], colorOffset, normalizedZ);
        } else {
            drawTextureLineNewA_NoClip(lineCX, linePX, lineCY, linePY, &texture[240], colorOffset, normalizedZ);
        }
    }
}
*/

void renderPolygon(object* sourceObject, polygon* preparedPolygon, unsigned int normalizedZ) {
    // Quick shorthand
    #define points preparedPolygon->points
    // Another useful shorthand
    screenPoint renderedPoints[] = {sourceObject->renderedPoints[points[0]], sourceObject->renderedPoints[points[1]], sourceObject->renderedPoints[points[2]], sourceObject->renderedPoints[points[3]]};
    // result of memory optimization
    const uint8_t* texture = textures[sourceObject->texture][preparedPolygon->polygonNum];

    // uint8_t* points = polygon.points;
    if (sourceObject->outline) {
        gfx_SetDrawScreen();
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
            if (renderedPoints[i].x < 0 || renderedPoints[i].x > GFX_LCD_WIDTH - 3 || renderedPoints[i].y < 0 || renderedPoints[i].y > GFX_LCD_HEIGHT - 2) {
                clipLines = true;
                break;
            }
        }
        uint8_t colorOffset = 0;
        if (preparedPolygon->polygonNum == 2 || preparedPolygon->polygonNum == 5) {
            colorOffset = 126;
        }
        // Generate the lines from the points of the polygon
        int x0 = renderedPoints[0].x;
        int y0 = renderedPoints[0].y;
        int x1 = renderedPoints[1].x;
        int y1 = renderedPoints[1].y;
        int dx0 = renderedPoints[3].x - x0;
        int dy0 = renderedPoints[3].y - y0;
        int dx1 = renderedPoints[2].x - x1;
        int dy1 = renderedPoints[2].y - y1;
        int sx0 = 1;
        int sy0 = 1;
        int sx1 = 1;
        int sy1 = 1;
        if (dx0 < 0) {
            sx0 = -1;
        }
        if (dy0 < 0) {
            sy0 = -1;
        }
        if (dx1 < 0) {
            sx1 = -1;
        }
        if (dy1 < 0) {
            sy1 = -1;
        }
        dx0 = -abs(dx0);
        dy0 = -abs(dy0);
        dx1 = -abs(dx1);
        dy1 = -abs(dy1);
        int length0 = (dx0 < dy0) ? -dx0 : -dy0;
        int length1 = (dx1 < dy1) ? -dx1 : -dy1;
        int length = (length0 > length1) ? length0 : length1;
        int tError = length;
        int errorX1 = length;
        int errorY1 = length;
        int errorX0 = length;
        int errorY0 = length;
        int tIndex = 0;
        // main body
        for (int i = -1; i < length; i++) {
            while (errorX0 <= 0) {
                errorX0 += length;
                x0 += sx0;
            }
            errorX0 += dx0;
            while (errorY0 <= 0) {
                errorY0 += length;
                y0 += sy0;
            }
            errorY0 += dy0;
            while (errorX1 <= 0) {
                errorX1 += length;
                x1 += sx1;
            }
            errorX1 += dx1;
            while (errorY1 <= 0) {
                errorY1 += length;
                y1 += sy1;
            }
            errorY1 += dy1;
            if (clipLines) {
                if (!((x0 < 0 && x1 < 0) || (x0 > (GFX_LCD_WIDTH - 1) && x1 > (GFX_LCD_WIDTH - 1))) && !((y0 < 0 && y1 < 0) || (y0 > (GFX_LCD_HEIGHT - 1) && y1 > (GFX_LCD_HEIGHT - 1))))
                    drawTextureLineNewA(x0, x1, y0, y1, texture + tIndex, colorOffset, normalizedZ);
            } else {
                drawTextureLineNewA_NoClip(x0, x1, y0, y1, texture + tIndex, colorOffset, normalizedZ);
            }
            while (tError <= 0 && tIndex < 240) {
                tError += length;
                tIndex += 16;
            }
            tError += -16;
        }
    }
}

int getPointDistance(point point) {
    int x = point.x - cameraXYZ[0];
    int y = point.y - cameraXYZ[1];
    int z = point.z - cameraXYZ[2];
    return approx_sqrt_a((x*x)+(y*y)+(z*z));
}

void rotateCamera(float x, float y) {
    bool cameraRotated = false;
    if ((x != 0) && (angleX + x >= -90 && angleX + x <= 90)) {
        angleX += x;
        cx = cosf(angleX*degRadRatio);
        sx = sinf(angleX*degRadRatio);
        cameraRotated = true;
    }
    if (y != 0) {
        angleY += y;
        while (angleY > 270) {
            angleY -= 360;
        }
        while (angleY < -90) {
            angleY += 360;
        }
        cy = cosf(angleY*degRadRatio);
        sy = sinf(angleY*degRadRatio);
        cameraRotated = true;
    }
    if (cameraRotated) {
        __asm__ ("di");
        for (unsigned int i = 0; i < numberOfObjects; i++) {
            objects[i]->generatePoints();
        }
        __asm__ ("ei");
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

void drawImage(int x, int y, int width, int height, uint16_t* dataPointer) {
    if (x > GFX_LCD_WIDTH - 1 || y > GFX_LCD_HEIGHT- 1) {
        return;
    }
    if (width <= 0 || height <= 0) {
        return;
    }
    if (x + width < 0 || y + height < 0) {
        return;
    }
    int rowSize = width;
    int numberOfRows = height;
    if (x < 0) {
        rowSize += x;
        dataPointer -= x;
        x = 0;
    }
    if (x + rowSize > GFX_LCD_WIDTH - 1) {
        rowSize = GFX_LCD_WIDTH - x;
    }
    if (y < 0) {
        numberOfRows += y;
        dataPointer -= (y*width);
        y = 0;
    }
    if (y + numberOfRows > GFX_LCD_HEIGHT- 1) {
        numberOfRows = 240 - y;
    }
    if (rowSize < 0 || numberOfRows < 0) {
        return;
    }
    rowSize *= sizeof(uint16_t);
    uint16_t* screenPointer = ((uint16_t*)0xD40000) + x + y*GFX_LCD_WIDTH;
    uint8_t numberOfTrueRows = numberOfRows;
    for (uint8_t i = 0; i < numberOfTrueRows && i < 240; i++) {
        memcpy(screenPointer, dataPointer, rowSize);
        screenPointer += GFX_LCD_WIDTH;
        dataPointer += width;
    }
}

void drawRectangle(int x, int y, int width, int height, uint16_t color) {
    if (x > GFX_LCD_WIDTH - 1 || y > GFX_LCD_HEIGHT- 1) {
        return;
    }
    if (width <= 0 || height <= 0) {
        return;
    }
    if (x + width < 0 || y + height < 0) {
        return;
    }
    int rowSize = width;
    int numberOfRows = height;
    if (x < 0) {
        rowSize += x;
        x = 0;
    }
    if (x + rowSize > GFX_LCD_WIDTH - 1) {
        rowSize = GFX_LCD_WIDTH - x;
    }
    if (y < 0) {
        numberOfRows += y;
        y = 0;
    }
    if (y + numberOfRows > GFX_LCD_HEIGHT- 1) {
        numberOfRows = 240 - y;
    }
    if (rowSize < 0 || numberOfRows < 0) {
        return;
    }
    uint16_t* screenPointer = ((uint16_t*)0xD40000) + x + y*GFX_LCD_WIDTH;
    uint8_t numberOfTrueRows = numberOfRows - 1;
    for (unsigned int i = 0; i < rowSize; i++) {
        *(screenPointer + i) = color;
    }
    rowSize *= sizeof(uint16_t);
    screenPointer += GFX_LCD_WIDTH;
    for (uint8_t i = 0; i < numberOfTrueRows && i < 240; i++) {
        memcpy(screenPointer, screenPointer - GFX_LCD_WIDTH, rowSize);
        screenPointer += GFX_LCD_WIDTH;
    }
}