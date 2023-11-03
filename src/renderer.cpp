#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ti/getcsc.h>
#include <graphx.h>
#include "renderer.hpp"
#include "textures.hpp"

#define focalDistance (Fixed24)300
/*
todo:
handle bad alloc

feature add ideas:
Lighting:
still thinking about how to do it
It's probably time to move on to making a game with this
*/
/*
Optimization ideas:
With the latest advancements in polygon drawing optimizations, generating the z buffer now takes about as long as filling the polygons themselves
This implies to me that generating the zBuffer could benefit from assembly optimization
The only problem is --- how the heck do I (a first time eZ80 assembly developer) do that?
Based on prior experience whatever I cobble together will probably be faster than what the compiler spits out, but I still have to get something working first.
*/

object** objects = (object**) 0xD3A000;
object** zSortedObjects = objects + maxNumberOfObjects;
transformedPolygon* preparedPolygons = (transformedPolygon*) (zSortedObjects + maxNumberOfObjects);

// Buffer for creating diagnostic strings
char buffer[200];

unsigned int numberOfPreparedPolygons = 0;

unsigned int numberOfObjects = 0;

unsigned int outOfBoundsPolygons = 0;

unsigned int obscuredPolygons = 0;

// Faces of a cube
uint8_t face1Points[] = {0, 1, 2, 3};
const polygon face1 = {face1Points, 2};
uint8_t face4Points[] = {4, 0, 3, 7};
const polygon face4 = {face4Points, 1};
uint8_t face0Points[] = {4, 0, 1, 5};
const polygon face0 = {face0Points, 0};

/*
Array of the polygons that make up a cube
Unwrapping order:
top, front, bottom, back, left, right
Unwrapping order with the optimized version:
top, front, right
*/ 
const polygon cubePolygons[] = {face0, face4, face1};

// Position of the camera
Fixed24 cameraXYZ[3] = {-100, 150, -100};

// Angle of the camera
const float cameraAngle[3] = {0.5235988, 0.7853982, 0};

// Constant ratios needed for projection
const Fixed24 cx = cosf(cameraAngle[0]);
const Fixed24 sx = sinf(cameraAngle[0]);
const Fixed24 cy = cosf(cameraAngle[1]);
const Fixed24 sy = sinf(cameraAngle[1]);
const Fixed24 cz = cosf(cameraAngle[2]);
const Fixed24 sz = sinf(cameraAngle[2]);

// Reciprocal needed for texture rendering
const Fixed24 reciprocalOf16 = (Fixed24)1/(Fixed24)16;

/*
clip:
clip = 0 // not affected by clipping
clip = 1 // affected by clipping, no z buffer
clip = 2 // affected by clipping, write to z buffer
*/
void object::generatePolygons(uint8_t clip) {
    // A local copy of the polygons of a cube (Not sure this is necessary)
    polygon polygons[3];

    // Rendering the object (Preparing polygons for rendering)
    if (visible) {
        // Init local copy of polygons
        memcpy(polygons, cubePolygons, sizeof(polygon)*3);
        for (uint8_t polygonNum = 0; polygonNum < 3; polygonNum++) {
            // Set the polygon's distance from the camera
            int z = 0;
            polygon* polygon = &polygons[polygonNum];
            for (unsigned int i = 0; i < 4; i++) {
                z += renderedPoints[polygon->points[i]].z;
            }
            z >>= 2;
            polygon->z = z;
        }

        // Sort the polygons by distance from the camera, front to back
        qsort((void *) polygons, 3, sizeof(polygon), zCompare);

        // Prepare the polygons for rendering
        for (uint8_t polygonNum = 0; polygonNum < 3; polygonNum++) {
            // Protection against polygon buffer overflow
            if (numberOfPreparedPolygons < maxNumberOfPolygons) {
                // The polygon we are rendering
                polygon polygon = polygons[polygonNum];

                // Are we going to render the polygon?
                bool render = true;

                bool zBufferEmpty = false;
                
                // Normalized z (0-255)
                unsigned int normalizedZ = ::abs(polygon.z) >> 3;

                // Clamp the Z to the range of the uint8_t
                if (normalizedZ > 255) {
                    normalizedZ = 255;
                }

                // If there are any other polygons this one could be overlapping with, check the z buffer
                // The z culling still has problems
                if (clip > 0) {
                    // Get the average x & y of the polygon
                    int totalX = 0;
                    int totalY = 0;
                    for (uint8_t i = 0; i < 4; i++) {
                        totalX += renderedPoints[polygon.points[i]].x;
                        totalY += renderedPoints[polygon.points[i]].y;
                    }
                    int x = totalX >> 2;
                    int y = totalY >> 2;
                    /*
                    // If this polygon is behind whatever is at (x,y), don't render it
                    if (normalizedZ >= gfx_GetPixel(x, y)) {
                        obscuredPolygons++;
                        render = false;
                    }*/
                    uint8_t obscuredPoints = 0;
                    for (uint8_t i = 0; i < 4; i++) {
                        screenPoint* point = &renderedPoints[polygon.points[i]];
                        uint8_t bufferZ = gfx_GetPixel((point->x + x)>>1, (point->y + y)>>1);
                        if (normalizedZ >= bufferZ) {
                            obscuredPoints++;
                        }
                        if (bufferZ == 255) {
                            zBufferEmpty = true;
                        }
                    }
                    if (obscuredPoints == 4) {
                        render = false;
                        obscuredPolygons++;
                    }
                }

                // Prepare this polygon for rendering
                if (render) {
                    // Draw the polygon to the z buffer (just the screen)
                    // For the transparent textures (glass & leaves), if the zBuffer has been cleared out (set to 255) around the glass, DON't draw to the zBuffer.
                    // Otherwise, do
                    // This is to avoid situations where either the glass doesn't draw to the zBuffer in a partial redraw, breaking the partial redraw engine,
                    // or cases where it draws to the zBuffer in a partial redraw with it shouldn't, resulting in blank space behind the glass.
                    if (clip > 1) {
                        gfx_SetColor(normalizedZ);
                        int x1 = renderedPoints[polygons[polygonNum].points[0]].x;
                        int x2 = renderedPoints[polygons[polygonNum].points[1]].x;
                        int x3 = renderedPoints[polygons[polygonNum].points[2]].x;
                        int x4 = renderedPoints[polygons[polygonNum].points[3]].x;
                        int y1 = renderedPoints[polygons[polygonNum].points[0]].y;
                        int y2 = renderedPoints[polygons[polygonNum].points[1]].y;
                        int y3 = renderedPoints[polygons[polygonNum].points[2]].y;
                        int y4 = renderedPoints[polygons[polygonNum].points[3]].y;
                        gfx_FillTriangle_NoClip(x1, y1, x2, y2, x3, y3);
                        gfx_FillTriangle_NoClip(x1, y1, x4, y4, x3, y3);
                    }

                    // Create a new transformed (prepared) polygon and set initialize it;
                    preparedPolygons[numberOfPreparedPolygons] = transformedPolygon{this, polygon.z, polygon.polygonNum};
                    
                    // Increment the number of prepared polygons
                    numberOfPreparedPolygons++;
                }
            }
        }
    }
}

void object::deleteObject() {
    if (visible) {
        gfx_SetDrawBuffer();
        // A local copy of the polygons of a cube (Not sure this is necessary)
        polygon polygons[3];
        memcpy(polygons, cubePolygons, sizeof(polygon)*3);
        for (uint8_t polygonNum = 0; polygonNum < 3; polygonNum++) {
            // Set the polygon's distance from the camera
            int z = 0;
            polygon* polygon = &polygons[polygonNum];
            for (unsigned int i = 0; i < 4; i++) {
                z += renderedPoints[polygon->points[i]].z;
            }
            z >>= 2;
            polygon->z = z;
        }

        // Sort the polygons by distance from the camera, front to back
        qsort((void *) polygons, 3, sizeof(polygon), zCompare);

        // Prepare the polygons for rendering
        if (visible) {
            for (uint8_t polygonNum = 0; polygonNum < 3; polygonNum++) {
                // The polygon we are rendering
                gfx_SetColor(255);
                /*int x1 = renderedPoints[polygons[polygonNum].points[0]].x;
                int x2 = renderedPoints[polygons[polygonNum].points[1]].x;
                int x3 = renderedPoints[polygons[polygonNum].points[2]].x;
                int x4 = renderedPoints[polygons[polygonNum].points[3]].x;
                int y1 = renderedPoints[polygons[polygonNum].points[0]].y;
                int y2 = renderedPoints[polygons[polygonNum].points[1]].y;
                int y3 = renderedPoints[polygons[polygonNum].points[2]].y;
                int y4 = renderedPoints[polygons[polygonNum].points[3]].y;
                gfx_FillTriangle_NoClip(x1, y1, x2, y2, x3, y3);
                gfx_FillTriangle_NoClip(x1, y1, x4, y4, x3, y3);
                gfx_SetDrawScreen();
                gfx_FillTriangle_NoClip(x1, y1, x2, y2, x3, y3);
                gfx_FillTriangle_NoClip(x1, y1, x4, y4, x3, y3);
                gfx_SetDrawBuffer();*/
                int16_t minX = renderedPoints[polygons[polygonNum].points[0]].x;
                int16_t minY = renderedPoints[polygons[polygonNum].points[0]].y;
                int16_t maxX = renderedPoints[polygons[polygonNum].points[0]].x;
                int16_t maxY = renderedPoints[polygons[polygonNum].points[0]].y;
                for (uint8_t i = 1; i < 4; i++) {
                    if (renderedPoints[polygons[polygonNum].points[i]].x < minX) {
                        minX = renderedPoints[polygons[polygonNum].points[i]].x;
                    } else if (renderedPoints[polygons[polygonNum].points[i]].x > maxX) {
                        maxX = renderedPoints[polygons[polygonNum].points[i]].x;
                    }
                    if (renderedPoints[polygons[polygonNum].points[i]].y < minY) {
                        minY = renderedPoints[polygons[polygonNum].points[i]].y;
                    } else if (renderedPoints[polygons[polygonNum].points[i]].y > maxY) {
                        maxY = renderedPoints[polygons[polygonNum].points[i]].y;
                    }
                }
                if (minX > 1) {
                    minX -= 2;
                }
                if (maxX < 318) {
                    maxX += 2;
                }
                if (minY > 1) {
                    minY -= 2;
                }
                if (maxY < 238) {
                    maxY += 2;
                }
                gfx_FillRectangle_NoClip(minX, minY, (maxX-minX), (maxY-minY));
                gfx_SetDrawScreen();
                gfx_FillRectangle_NoClip(minX, minY, (maxX-minX), (maxY-minY));
                gfx_SetDrawBuffer();
            }
        }
    }
    gfx_SetDrawScreen();
}

void object::generatePoints() {
    visible = true;
    // The verticies of the cube
    point points[] = {{x, y, z}, {x+size, y, z}, {x+size, y-size, z}, {x, y-size, z}, {x, y, z+size}, {x+size, y, z+size}, {x+size, y-size, z+size}, {x, y-size, z+size}};
    // rough but hopefully effective optimization
    {
        renderedPoints[0] = transformPoint(points[0]);

        // If the object is outside the culling distance, don't render it
        if (renderedPoints[0].z >= zCullingDistance) {
            visible = false;
        }

        // If any part of the object is off screen, don't render it
        // Not ideal, but makes a lot of other things easier (and faster)
        if (renderedPoints[0].x < 0 || renderedPoints[0].x > 319 || renderedPoints[0].y < 0 || renderedPoints[0].y > 239) {
            visible = false;
        }
    }

    // Check all the other points if the origin of the cube is on screen
    if (visible) {
        for (uint8_t i = 1; i < 8; i++) {
            renderedPoints[i] = transformPoint(points[i]);
            if (renderedPoints[i].z >= zCullingDistance) {
                visible = false;
                break;
            }
            if (renderedPoints[i].x < 0 || renderedPoints[i].x > 319 || renderedPoints[i].y < 0 || renderedPoints[i].y > 239) {
                visible = false;
                break;
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

int renderedZCompare(const void *arg1, const void *arg2) {
    transformedPolygon* polygon1 = ((transformedPolygon *) arg1);
    transformedPolygon* polygon2 = ((transformedPolygon *) arg2);
    return polygon2->z - polygon1->z;
}

int distanceCompare(const void *arg1, const void *arg2) {
    object* object1 = *((object**) arg1);
    object* object2 = *((object**) arg2);
    return object1->getDistance() - object2->getDistance();
}

// why don't you work???
int xCompare(const void *arg1, const void *arg2) {
    object* object1 = *((object**) arg1);
    object* object2 = *((object**) arg2);
    /*snprintf(buffer, 200, "XComp X1: %f, X2: %f", (float)object1->x, (float)object2->x);
    gfx_PrintString(buffer);*/
    return object1->x - object2->x;
}

// possible target for optimization -- as soon as I figure out how
screenPoint transformPoint(point point) {
    // Result we will return
    screenPoint result;
    
    // Convert from global space to relative to the camera
    Fixed24 x = point.x - cameraXYZ[0];
    Fixed24 y = point.y - cameraXYZ[1];
    Fixed24 z = point.z - cameraXYZ[2];
    
    // Components of the transform equation that get reused
    const Fixed24 sum1 = (sz*y+cz*x);
    const Fixed24 sum2 = (cy*z+sy*sum1);
    const Fixed24 sum3 = (cz*y - sz*x);
    // This is mostly magic I found on Wikipedia that I don't really understand
    const Fixed24 dx = cy*sum1 - sy*z;
    const Fixed24 dy = sx*sum2 + cx*sum3;
    const Fixed24 dz = cx*sum2 - sx*sum3;
    const Fixed24 sum4 = (focalDistance/dz);
    result.x = (int)((Fixed24)160 + sum4*dx);
    result.y = (int)((Fixed24)120 - sum4*dy);
    result.z = int_sqrt_a(((int)x*(int)x)+((int)y*(int)y)+((int)z*(int)z));
    return result;
}

void deletePolygons() {
    numberOfPreparedPolygons = 0;
}

void deleteEverything() {
    deletePolygons();
    for (unsigned int i = 0; i < numberOfObjects; i++) {
        if (objects[i]) {
            delete objects[i];
        }
    }
    numberOfObjects = 0;
}

/*
0: Normal full render
1: Redraw without regenerating polygons
2: Redraw with regenerating polygons (but not z buffer)
3: 1 with special handling for transparent objects
*/
void drawScreen(uint8_t mode) {
    if (mode == 0 || mode == 2 || mode == 3) {
        // Move the camera (for the demo that is the state of the program)
        //cameraXYZ[2] += 5;
        gfx_SetDrawBuffer();
        if (mode == 0) {
            gfx_SetTextXY(0, 0);
            // Clear the screen
            gfx_FillScreen(255);
        }
        // Clear the list of polygons
        deletePolygons();
        outOfBoundsPolygons = 0;
        obscuredPolygons = 0;
        if (mode == 3) {
            for (unsigned int i = 0; i < numberOfObjects; i++) {
                if (zSortedObjects[i]->texture != 15 && zSortedObjects[i]->texture != 23) {
                    zSortedObjects[i]->generatePolygons(2);
                }
            }
            for (unsigned int i = 0; i < numberOfObjects; i++) {
                if (zSortedObjects[i]->texture == 15 || zSortedObjects[i]->texture == 23) {
                    zSortedObjects[i]->generatePolygons(1);
                }
            }
        } else if (mode == 0) {
            for (unsigned int i = 0; i < numberOfObjects; i++) {
                if (zSortedObjects[i]->texture == 15 || zSortedObjects[i]->texture == 23) {
                    zSortedObjects[i]->generatePolygons(1);
                } else {
                    zSortedObjects[i]->generatePolygons(2);
                }
            }
        } else {
            for (unsigned int i = 0; i < numberOfObjects; i++) {
                zSortedObjects[i]->generatePolygons(2);
            }
        }
        gfx_SetDrawScreen();
    }
    // Sort all polygons back to front
    qsort(preparedPolygons, numberOfPreparedPolygons, sizeof(transformedPolygon), renderedZCompare);
    
    // I think this is slower but there will be no leftovers from polygon generation
    #if showDraw == false
    // Clear the screen again
    if (mode == 0) {
        gfx_FillScreen(255);
    }
    #endif
    for (unsigned int i = 0; i < numberOfPreparedPolygons; i++) {
        renderPolygon(&preparedPolygons[i]);
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
void renderPolygon(transformedPolygon* polygon) {
    // Quick shorthand
    uint8_t* points = cubePolygons[polygon->polygonNum].points;
    // Another useful shortand
    object* sourceObject = polygon->object;
    // result of memory optimization
    const uint8_t* texture = textures[polygon->object->texture][polygon->polygonNum];

    point sourcePoints[] = {{sourceObject->x, sourceObject->y, sourceObject->z}, {sourceObject->x+sourceObject->size, sourceObject->y, sourceObject->z}, {sourceObject->x+sourceObject->size, sourceObject->y-sourceObject->size, sourceObject->z}, {sourceObject->x, sourceObject->y-sourceObject->size, sourceObject->z}, {sourceObject->x, sourceObject->y, sourceObject->z+sourceObject->size}, {sourceObject->x+sourceObject->size, sourceObject->y, sourceObject->z+sourceObject->size}, {sourceObject->x+sourceObject->size, sourceObject->y-sourceObject->size, sourceObject->z+sourceObject->size}, {sourceObject->x, sourceObject->y-sourceObject->size, sourceObject->z+sourceObject->size}};

    // Array of the 2 lines of the polygon we will need for this
    lineEquation lineEquations[2];
    // uint8_t* points = polygon->points;
    if (sourceObject->outline) {
        gfx_SetColor(outlineColor);
        for (uint8_t i = 0; i < 4; i++) {
            uint8_t nextPoint = i + 1;
            if (nextPoint > 3) {
                nextPoint = 0;
            }
            gfx_Line_NoClip(sourceObject->renderedPoints[points[i]].x, sourceObject->renderedPoints[points[i]].y, sourceObject->renderedPoints[points[nextPoint]].x, sourceObject->renderedPoints[points[nextPoint]].y);
        }
    } else {
        uint8_t colorOffset = 0;
        if (polygon->polygonNum == 2) {
            colorOffset = 126;
        }
        // Generate the lines from the points of the polygon
        for (unsigned int i = 0; i < 2; i++) {
            unsigned int nextPoint = (2*i) + 2;
            unsigned int currentPoint = (2*i) + 1;
            if (nextPoint > 3) {
                nextPoint = 0;
            }
            //lines[i] = {points[i], points[nextPoint]};
            Fixed24 dx = sourceObject->renderedPoints[points[nextPoint]].x - sourceObject->renderedPoints[points[currentPoint]].x;
            Fixed24 dy = sourceObject->renderedPoints[points[nextPoint]].y - sourceObject->renderedPoints[points[currentPoint]].y;

            // Find the length of the line
            //int length = ((int)dx*(int)dx)+((int)dy*(int)dy);
            int length;
            if (abs(dx) > abs(dy)) {
                length = abs(dx);
            } else {
                length = abs(dy);
            }
            lineEquations[i] = {sourceObject->renderedPoints[points[currentPoint]].x, sourceObject->renderedPoints[points[nextPoint]].x, sourceObject->renderedPoints[points[currentPoint]].y, sourceObject->renderedPoints[points[nextPoint]].y, length, dx, dy};
        }
        int length;
        if (lineEquations[1].length > lineEquations[0].length) {
            //length = int_sqrt(lineEquations[1].length);
            length = lineEquations[1].length;
        } else {
            // length = int_sqrt(lineEquations[0].length);
            length = lineEquations[0].length;
        }

        // Ratios that make the math faster (means we can multiply instead of divide)
        const Fixed24 reciprocalOfLength = (Fixed24)1/(Fixed24)length;

        // Another ratio that makes the math faster
        Fixed24 divI = 0;
        Fixed24 lineCX = lineEquations[1].px;
        Fixed24 linePX = lineEquations[0].cx;
        Fixed24 lineCY = lineEquations[1].py;
        Fixed24 linePY = lineEquations[0].cy;
        Fixed24 CXdiff = (-lineEquations[1].dx)*reciprocalOfLength;
        Fixed24 PXdiff = (lineEquations[0].dx)*reciprocalOfLength;
        Fixed24 CYdiff = (-lineEquations[1].dy)*reciprocalOfLength;
        Fixed24 PYdiff = (lineEquations[0].dy)*reciprocalOfLength;
        
        // main body
        for (int i = 0; i < length; i++) {
            int row = (divI.n >> 4) & 0xFFFFF0;
            drawTextureLineA(lineCX, linePX, lineCY, linePY, &texture[row], colorOffset);
            divI += reciprocalOfLength;
            lineCX += CXdiff;
            linePX += PXdiff;
            lineCY += CYdiff;
            linePY += PYdiff;
        }
        // last line
        drawTextureLineA(lineCX, linePX, lineCY, linePY, &texture[240], colorOffset);
    }
}

int getPointDistance(point point) {
    int x = point.x - cameraXYZ[0];
    int y = point.y - cameraXYZ[1];
    int z = point.z - cameraXYZ[2];
    return int_sqrt((x*x)+(y*y)+(z*z));
}

object** xSearch(object* key) {
    return (object**) bsearch((void*) key, objects, numberOfObjects, sizeof(object *), xCompare);
}

void xSort() {
    qsort(objects, numberOfObjects, sizeof(object *), xCompare);
    memcpy(zSortedObjects, objects, sizeof(object*) * numberOfObjects);
    // Sort all objects front to back
    // improves polygon culling but sorting can be slow
    qsort(zSortedObjects, numberOfObjects, sizeof(object*), distanceCompare);
}

unsigned char bit_width(unsigned x) {
    return x == 0 ? 1 : 24 - __builtin_clz(x);
}

// Thank you Jan Schultke of Stack Overflow! (seriously it's ludicrious how much faster this is)
// https://stackoverflow.com/questions/34187171/fast-integer-square-root-approximation
// probably could be optimized in assembly -- just a matter of how
// I know that previously square root was one of the longest parts of the math, and I suspect it still is
unsigned int_sqrt(unsigned n) {
    unsigned char shift = bit_width(n);
    shift += shift & 1; // round up to next multiple of 2

    unsigned result = 0;

    do {
        // decrement shift
        shift -= 2;
        // shift result left 1
        result <<= 1; // leftshift the result to make the next guess
        // or result with 1
        result |= 1;  // guess that the next bit is 1
        // xor(?) result with bool (result * result) > (n >> shift)
        result ^= result * result > (n >> shift); // revert if guess too high
    } while (shift != 0);

    return result;
}

// Implemented in assembly now
/*void drawTextureLine(Fixed24 startingX, Fixed24 startingY, Fixed24 endingX, Fixed24 endingY, const uint8_t* texture, uint8_t colorOffset) {
    Fixed24 x1 = startingX;
    Fixed24 y1 = startingY;
    Fixed24 dx = (endingX) - startingX;
    Fixed24 dy = (endingY) - startingY;
    int textureLineLength;
    if (abs(dx) > abs(dy)) {
        textureLineLength = abs(dx) + (Fixed24)1;
    } else {
        textureLineLength = abs(dy) + (Fixed24)1;
    }
    Fixed24 reciprocalOfTextureLineLength = (Fixed24)1/(Fixed24)textureLineLength;
    Fixed24 textureLineRatio;
    textureLineRatio.n = reciprocalOfTextureLineLength.n << 4;
    // Numbers we can recursively add instead of needing to multiply on each loop (less precise but gets the job done)
    Fixed24 xDiff = (Fixed24)dx * reciprocalOfTextureLineLength;
    Fixed24 yDiff = (Fixed24)dy * reciprocalOfTextureLineLength;
    // call out to assembly
    fillLine(x1, y1, xDiff, yDiff, textureLineLength, textureLineRatio, texture, colorOffset);
    //drawTextureLineA(startingX, endingX, startingY, endingY, texture, colorOffset);
}*/