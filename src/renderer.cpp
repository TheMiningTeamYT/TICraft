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
feature add ideas:
Lighting:
still thinking about how to do it
It's probably time to move on to making a game with this
*/
/*
optimization ideas:
Appearently generating the points takes way longer than I realized.
Add support for different sized textures, depending on whats most apropriate, say 16x16, 8x8, or 4x4. That way, for small polygons we can waste less time drawing.
Or, undersampling the textures at small sizes
Add a buffer to cache transformed polygon values if they atch -- Is it possible to do this without taking longer than rendering?
My brain is devoid of ideas at this time for optimization
Using ints for distance from camera (z) now... faster and doesn't encounter range issues like Fixed24 does... but has FAR less precision... maybe some multiplication is in order to increase the precision (virtual fixed point)?
Make rendered points back into a pointer that is deleted and reinitalized after each frame -- that way memory isn't wasted storing points of objects that are off screen
Don't re-render the points if told not to
*/
/*
Speed observations:
It appears that drawing the polygons is a farely slow process (if you disable double buffering you can see it happen --- it should be too fast to see)
It seems that generating the row is the longest part of the polygon drawing process. -- maybe can be optimized?
Undersampling the texture by 2x (16->8px) led to over a 1/3 improvement in render time
It also appears that generating the polygons takes a considerable amount of time as well (with double buffering disabled you can see it sit as it generates them all)
For large numbers of cubes, polygon generation takes up most of the render time -- what can be done to optimize this?
A faster method of drawing the pixels to the screen (rather than drawing tons of little lines) would be preferred
It would also likely be faster if we didn't have to overdraw the lines to avoid blank pixels
Also, the fixed point library I'm using does not have a very big range, so overflow issues are common. Maybe a better fixed point library is in order?
*/

object* objects[maxNumberOfObjects];
object** zSortedObjects;
transformedPolygon* preparedPolygons[maxNumberOfPolygons];

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

void object::generatePolygons(bool clip) {
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
                polygon* polygon = &polygons[polygonNum];

                // Are we going to render the polygon?
                bool render = true;
                
                // Normalized z (0-255)
                unsigned int normalizedZ = ::abs(polygon->z) >> 3;

                // Clamp the Z to the range of the uint8_t
                if (normalizedZ > 255) {
                    normalizedZ = 255;
                }

                // If there are any other polygons this one could be overlapping with, check the z buffer
                // The z culling still has problems
                if (clip == true) {
                    // Get the average x & y of the polygon
                    int totalX = 0;
                    int totalY = 0;
                    for (uint8_t i = 0; i < 4; i++) {
                        totalX += renderedPoints[polygon->points[i]].x;
                        totalY += renderedPoints[polygon->points[i]].y;
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
                        screenPoint* point = &renderedPoints[polygon->points[i]];
                        if (normalizedZ >= gfx_GetPixel((point->x + x)>>1, (point->y + y)>>1)) {
                            obscuredPoints++;
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
                    if (clip == true) {
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
                    preparedPolygons[numberOfPreparedPolygons] = new transformedPolygon{this, polygon->z, polygon->polygonNum};
                    
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
        if (renderedPoints[0].x < 0 || renderedPoints[0].x > 320 || renderedPoints[0].y < 0 || renderedPoints[0].y > 240) {
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
            if (renderedPoints[i].x < 0 || renderedPoints[i].x > 320 || renderedPoints[i].y < 0 || renderedPoints[i].y > 240) {
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
    transformedPolygon* polygon1 = *((transformedPolygon **) arg1);
    transformedPolygon* polygon2 = *((transformedPolygon **) arg2);
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
    result.x = ((Fixed24)160 + sum4*dx).floor();
    result.y = ((Fixed24)120 - sum4*dy).floor();
    // Is calling two functions but only if you need to slower than always calling just one function? IDK!
    result.z = int_sqrt(((int)x*(int)x)+((int)y*(int)y)+((int)z*(int)z));
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
*/
void drawScreen(uint8_t mode) {
    if (mode == 0 || mode == 2) {
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
        for (unsigned int i = 0; i < numberOfObjects; i++) {
            zSortedObjects[i]->generatePolygons(true);
        }
        gfx_SetDrawScreen();
    }
    // Sort all polygons back to front
    qsort(preparedPolygons, numberOfPreparedPolygons, sizeof(transformedPolygon *), renderedZCompare);
    
    // I think this is slower but there will be no leftovers from polygon generation
    #if showDraw == false
    // Clear the screen again
    if (mode == 0) {
        gfx_FillScreen(255);
    }
    #endif
    for (unsigned int i = 0; i < numberOfPreparedPolygons; i++) {
        renderPolygon(preparedPolygons[i]);
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
            colorOffset = 64;
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
            // Use faster Fixed24 sqr/sqrt if the numbers are not too big
            /*if ((int)dx < 46 && (int)dy < 46) {
                dx = sqr(dx);
                dy = sqr(dy);
                if ((int)dx + (int)dy < 2048) {
                    Fixed24 value = dx + dy;
                    length = sqrt(value);
                } else {
                    length = sqrtf((float)dx + (float)dy);
                } 
            } else {
                length = sqrtf(powf((float)dx, 2) + powf((float)dy, 2));
            }*/
            int length = ((int)dx*(int)dx)+((int)dy*(int)dy);
            lineEquations[i] = {sourceObject->renderedPoints[points[currentPoint]].x, sourceObject->renderedPoints[points[nextPoint]].x, sourceObject->renderedPoints[points[currentPoint]].y, sourceObject->renderedPoints[points[nextPoint]].y, length};
        }
        int length;
        if (lineEquations[1].length > lineEquations[0].length) {
            length = int_sqrt(lineEquations[1].length);
        } else {
            length = int_sqrt(lineEquations[0].length);
        }

        // Ratios that make the math faster (means we can multiply instead of divide)
        const Fixed24 reciprocalOfLength = (Fixed24)1/(Fixed24)length;
        Fixed24 reciprocalOfTextureRatio;
        reciprocalOfTextureRatio.n = reciprocalOfLength.n<<4;
        for (int i = 0; i < length; i++) {
            /*gfx_SetTextXY(0, 0);
            snprintf(buffer, 200, "i: %u", i);
            gfx_SetColor(255);
            gfx_FillRectangle(0, 0, 32, 32);
            gfx_PrintString(buffer);*/

            // Another ratio that makes the math faster
            const Fixed24 divI = ((Fixed24)i*reciprocalOfLength);

            // The line we will draw a row of the texture along
            lineEquation textureLine = {(((Fixed24)1-divI)*lineEquations[1].px)+(divI*lineEquations[1].cx), (((Fixed24)1-divI)*lineEquations[0].cx)+(divI*lineEquations[0].px), (((Fixed24)1-divI)*lineEquations[1].py)+(divI*lineEquations[1].cy), (((Fixed24)1-divI)*lineEquations[0].cy)+(divI*lineEquations[0].py)};
            int row = ((Fixed24)i*reciprocalOfTextureRatio).floor()*16;

            // Declare/Init the x/y of the lines we will need
            Fixed24 x1;
            Fixed24 x2 = textureLine.cx;
            Fixed24 y1;
            Fixed24 y2 = textureLine.cy;
            // Numbers we can recursively add instead of needing to multiply on each loop (less precise but gets the job done)
            Fixed24 xDiff = ((((Fixed24)1-reciprocalOf16)*textureLine.cx) + (reciprocalOf16*textureLine.px)) - textureLine.cx;
            Fixed24 yDiff = ((((Fixed24)1-reciprocalOf16)*textureLine.cy) + (reciprocalOf16*textureLine.py)) - textureLine.cy;
            bool longDistance = xDiff.abs() + yDiff.abs() > (Fixed24)1;
            /*uint8_t aDiff = 1;
            if (length < (Fixed24)4) {
                xDiff.n = xDiff.n << 2;
                yDiff.n = yDiff.n << 2;
                aDiff = 3;
            } else if (length < (Fixed24)8) {
                xDiff.n = xDiff.n << 1;
                yDiff.n = yDiff.n << 1;
                aDiff = 2;
            }*/

            // Draw each pixel of the texture row
            for (int a = 0; a < 16; a++) {
                x1 = x2;
                y1 = y2;

                // Set the line color to the color of the pixel from the texture
                // 255 is reserved for transparency
                uint8_t color = texture[row + a] + colorOffset;
                //gfx_SetColor(color);

                // Move along the line 
                x2 += xDiff;
                y2 += yDiff;

                // Convert the Fixed24's to ints
                int lineX1 = x1;
                int lineY1 = y1;
                // this'll do, but I'd like a better way of fixing the white pixels than *blindly* overdrawing
                // this is leaving a few blank pixels -- not ideal
                if (longDistance) {
                    // faster but a bunch of unfilled pixels
                    // too many unfilled pixels
                    while (texture[row + a] == texture[row + a + 1] && a < 15) {
                        x2 += xDiff;
                        y2 += yDiff;
                        a++;
                    }
                    int lineX2 = x2;
                    int lineY2 = y2;
                    color = texture[row + a] + colorOffset;
                    gfx_SetColor(color);
                    if (lineY1 > 0 && lineY2 > 0) {
                        gfx_Line_NoClip(lineX1, lineY1 - 1, lineX2, lineY2 - 1);
                    } else {
                        gfx_Line(lineX1, lineY1 - 1, lineX2, lineY2 - 1);
                    }
                    gfx_Line_NoClip(lineX1, lineY1, lineX2, lineY2);
                } else {
                    gfx_vram[(lineY1*320)+lineX1] = color;
                }
            }
        }
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
    delete[] zSortedObjects;
    zSortedObjects = new object*[numberOfObjects];
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
// implementation for all unsigned integer types
unsigned int_sqrt(unsigned n) {
    unsigned char shift = bit_width(n);
    shift += shift & 1; // round up to next multiple of 2

    unsigned result = 0;

    do {
        shift -= 2;
        result <<= 1; // leftshift the result to make the next guess
        result |= 1;  // guess that the next bit is 1
        result ^= result * result > (n >> shift); // revert if guess too high
    } while (shift != 0);

    return result;
}