#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ti/getcsc.h>
#include <graphx.h>
#include "renderer.hpp"

#define focalDistance (Fixed24)400
/*
feature add ideas:
Lighting:
still thinking about how to do it
It's probably time to move on to making a game with this
*/
/*
optimization ideas:
Add support for different sized textures, depending on whats most apropriate, say 16x16, 8x8, or 4x4. That way, for small polygons we can waste less time drawing.
Or, undersampling the textures at small sizes
Add a buffer to cache transformed polygon values if they atch -- Is it possible to do this without taking longer than rendering?
My brain is devoid of ideas at this time for optimization
Using ints for distance from camera (z) now... faster and doesn't encounter range issues like Fixed24 does... but has FAR less precision... maybe some multiplication is in order to increase the precision (virtual fixed point)?
Make rendered points back into a pointer that is deleted and reinitalized after each frame -- that way memory isn't wasted storing points of objects that are off screen
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
transformedPolygon* preparedPolygons[maxNumberOfPolygons];

// Buffer for creating diagnostic strings
char buffer[200];

unsigned int numberOfPreparedPolygons = 0;

unsigned int numberOfObjects = 0;

unsigned int outOfBoundsPolygons = 0;

unsigned int obscuredPolygons = 0;

// The number of frames since the objects in the world were sorted
int8_t lastResort = -1;

// Faces of a cube
uint8_t face1Points[] = {0, 1, 2, 3};
polygon face1 = {face1Points, 5};
uint8_t face5Points[] = {5, 1, 2, 6};
polygon face5 = {face5Points, 3};
uint8_t face4Points[] = {4, 0, 3, 7};
polygon face4 = {face4Points, 1};
uint8_t face3Points[] = {4, 5, 6, 7};
polygon face3 = {face3Points, 4};
uint8_t face2Points[] = {3, 2, 6, 7};
polygon face2 = {face2Points, 2};
uint8_t face0Points[] = {4, 0, 1, 5};
polygon face0 = {face0Points, 0};

/*
Array of the polygons that make up a cube
Unwrapping order:
top, front, bottom, back, left, right
*/ 
polygon cubePolygons[] = {face0, face1, face2, face3, face4, face5};

// Position of the camera
Fixed24 cameraXYZ[3] = {-100, 200, -100};

// Angle of the camera
const float cameraAngle[3] = {0.7853982, 0.7853982, 0};

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
    // The verticies of the cube
    point points[] = {{x, y, z}, {x+size, y, z}, {x+size, y-size, z}, {x, y-size, z}, {x, y, z+size}, {x+size, y, z+size}, {x+size, y-size, z+size}, {x, y-size, z+size}};
    
    // A local copy of the polygons of a cube (Not sure this is necessary)
    polygon polygons[6];

    // Will we render this object (is it on screen)?
    bool objectRender = true;

    // rough but hopefully effective optimization
    {
        int z = getPointDistance(points[0]);

        // If the object is outside the culling distance, don't render it
        if (z < zCullingDistance) {
            renderedPoints[0] = transformPoint(points[0]);
            renderedPoints[0].z = z;
        } else {
            objectRender = false;
            outOfBoundsPolygons += 6;
        }

        // If any part of the object is off screen, don't render it
        // Not ideal, but makes a lot of other things easier (and faster)
        if (renderedPoints[0].x < 0 || renderedPoints[0].x > 320 || renderedPoints[0].y < 0 || renderedPoints[0].y > 240) {
            objectRender = false;
            outOfBoundsPolygons += 6;
        }
    }

    // Check all the other points if the origin of the cube is on screen
    if (objectRender) {
        for (uint8_t i = 1; i < 8; i++) {
            int z = getPointDistance(points[i]);
            if (z < zCullingDistance) {
                renderedPoints[i] = transformPoint(points[i]);
                renderedPoints[i].z = z;
            } else {
                objectRender = false;
                outOfBoundsPolygons += 6;
                break;
            }
            if (renderedPoints[i].x < 0 || renderedPoints[i].x > 320 || renderedPoints[i].y < 0 || renderedPoints[i].y > 240) {
                objectRender = false;
                outOfBoundsPolygons += 6;
                break;
            }
        }
    }

    // Rendering the object (Preparing polygons for rendering)
    if (objectRender) {
        visible = true;
        // Init local copy of polygons
        memcpy(polygons, cubePolygons, sizeof(polygon)*6);
        for (uint8_t polygonNum = 0; polygonNum < 6; polygonNum++) {
            // Set the polygon's distance from the camera
            int z = 0;
            polygon* polygon = &polygons[polygonNum];
            for (unsigned int i = 0; i < 4; i++) {
                z += renderedPoints[polygon->points[i]].z;
            }
            z = z >> 2;
            polygon->z = z;
        }

        // Sort the polygons by distance from the camera, front to back
        qsort((void *) polygons, 6, sizeof(polygon), zCompare);

        // Prepare the polygons for rendering
        for (uint8_t polygonNum = 0; polygonNum < 6; polygonNum++) {
            // Protection against polygon buffer overflow
            if (numberOfPreparedPolygons < maxNumberOfPolygons) {
                // The polygon we are rendering
                polygon* polygon = &polygons[polygonNum];

                // Are we going to render the polygon?
                bool render = true;
                Fixed24 z = polygon->z;
                
                // Normalized z (0-255)
                unsigned int normalizedZ = ::abs(polygon->z) >> 2;

                // Clamp the Z to the range of the uint8_t
                if (normalizedZ > 255) {
                    normalizedZ = 255;
                }

                // If there are any other polygons this one could be overlapping with, check the z buffer
                // The z culling still has problems
                if (numberOfPreparedPolygons > 0 && clip == true) {
                    // Get the average x & y of the polygon
                    int totalX = 0;
                    int totalY = 0;
                    for (uint8_t i = 0; i < 4; i++) {
                        totalX += renderedPoints[polygon->points[i]].x;
                        totalY += renderedPoints[polygon->points[i]].y;
                    }
                    int x = totalX >> 2;
                    int y = totalY >> 2;

                    // If this polygon is behind whatever is at (x,y), don't render it
                    if (normalizedZ >= gfx_GetPixel(x, y)) {
                        obscuredPolygons++;
                        render = false;
                    }
                    /*uint8_t obscuredPoints = 0;
                    for (uint8_t i = 0; i < 4; i++) {
                        screenPoint* point = &renderedPoints[polygon->points[i]];
                        if (normalizedZ >= gfx_GetPixel(point->x, point->y)) {
                            obscuredPoints++;
                        }
                    }
                    if (obscuredPoints == 4) {
                        obscuredPolygons++;
                        render = false;
                    }*/
                }

                // Prepare this polygon for rendering
                if (render) {
                    // Draw the polygon to the z buffer (just the screen)
                    if (clip == true) {
                        gfx_SetColor(normalizedZ);
                        int x1 = renderedPoints[polygon->points[0]].x;
                        int x2 = renderedPoints[polygon->points[1]].x;
                        int x3 = renderedPoints[polygon->points[2]].x;
                        int x4 = renderedPoints[polygon->points[3]].x;
                        int y1 = renderedPoints[polygon->points[0]].y;
                        int y2 = renderedPoints[polygon->points[1]].y;
                        int y3 = renderedPoints[polygon->points[2]].y;
                        int y4 = renderedPoints[polygon->points[3]].y;
                        gfx_FillTriangle_NoClip(x1, y1, x2, y2, x3, y3);
                        gfx_FillTriangle_NoClip(x1, y1, x4, y4, x3, y3);
                    }

                    // Create a new transformed (prepared) polygon and set initialize it;
                    preparedPolygons[numberOfPreparedPolygons] = new transformedPolygon{polygon->points, this, texture[polygon->polygonNum], z};
                    
                    // Increment the number of prepared polygons
                    numberOfPreparedPolygons++;
                }
            }
        }
    } else {
        visible = false;
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
    return getPointDistance({x, y, z});
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
    // Result we will return
    screenPoint result;
    
    // Convert from global space to relative to the camera
    const Fixed24 x = point.x - cameraXYZ[0];
    const Fixed24 y = point.y - cameraXYZ[1];
    const Fixed24 z = point.z - cameraXYZ[2];
    
    // Components of the transform equation that get reused
    const Fixed24 sum1 = (sz*y+cz*x);
    const Fixed24 sum2 = (cy*z+sy*sum1);
    const Fixed24 sum3 = (cz*y - sz*x);
    // This is mostly magic I found on Wikipedia that I don't really understand
    const Fixed24 dx = cy*sum1-sy*z;
    const Fixed24 dy = sx*sum2 + cx*sum3;
    const Fixed24 dz = cx*sum2 - sx*sum3;
    const Fixed24 sum4 = (focalDistance/dz);
    result.x = ((Fixed24)160 + sum4*dx).floor();
    result.y = ((Fixed24)120 - sum4*dy).floor();
    // Is calling two functions but only if you need to slower than always calling just one function? IDK!
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

void drawScreen(bool redraw) {
    if (!redraw) {
        // Move the camera (for the demo that is the state of the program)
        cameraXYZ[2] += 5;
        gfx_SetTextXY(0, 0);
        // Clear the list of polygons
        deletePolygons();
        outOfBoundsPolygons = 0;
        obscuredPolygons = 0;
        // Clear the screen
        gfx_FillScreen(255);
        // Sort all objects front to back
        // improves polygon culling but sorting can be slow
        if (lastResort == -1 || lastResort > 9) {
            qsort(objects, numberOfObjects, sizeof(object*), distanceCompare);
            lastResort = 0;
        } else {
            lastResort++;
        }
        for (unsigned int i = 0; i < numberOfObjects; i++) {
            objects[i]->generatePolygons(true);
        }
    }
    // Sort all polygons back to front
    qsort(preparedPolygons, numberOfPreparedPolygons, sizeof(transformedPolygon *), renderedZCompare);
    
    // I think this is slower but there will be no leftovers from polygon generation
    #if showDraw == false
    // Clear the screen again
    if (!redraw) {
        gfx_FillScreen(255);
    }
    #endif
    for (unsigned int i = 0; i < numberOfPreparedPolygons; i++) {
        renderPolygon(preparedPolygons[i]);
    }

    // Print some diagnostic information
    if (!redraw) {
        snprintf(buffer, 200, "Out of bounds polygons: %u", outOfBoundsPolygons);
        gfx_PrintString(buffer);
        gfx_SetTextXY(0, gfx_GetTextY() + 10);
        snprintf(buffer, 200, "Obscured Polygons: %u", obscuredPolygons);
        gfx_PrintString(buffer);
        gfx_SetTextXY(0, gfx_GetTextY() + 10);
        snprintf(buffer, 200, "Total Polygons: %u", numberOfPreparedPolygons);
        gfx_PrintString(buffer);
    }

    /*gfx_SetTextXY(0, gfx_GetTextY() + 10);
    snprintf(buffer, 200, "Object Size: %u", sizeof(object));
    gfx_PrintString(buffer);
    gfx_SetTextXY(0, gfx_GetTextY() + 10);
    snprintf(buffer, 200, "Polygon Size: %u", sizeof(transformedPolygon));
    gfx_PrintString(buffer);
    gfx_SetTextXY(0, gfx_GetTextY() + 10);*/
    #if showDraw == false
    if (!redraw) {
        gfx_SwapDraw();
    }
    #endif
}

// implementation of affine texture mapping (i think thats what you'd call this anyway)
// as noted in The Science Elf's original video, affine texture mapping can look very weird on triangles
// but on quads, it looks pretty good at a fraction of the cost
void renderPolygon(transformedPolygon* polygon) {
    // Quick shorthand
    // Another useful shortand
    #define points polygon->points
    #define sourceObject polygon->object

    if (sourceObject->outline) {
        gfx_SetColor(0);
        for (uint8_t i = 0; i < 4; i++) {
            uint8_t nextPoint = i + 1;
            if (nextPoint > 3) {
                nextPoint = 0;
            }
            gfx_Line_NoClip(sourceObject->renderedPoints[points[i]].x, sourceObject->renderedPoints[points[i]].y, sourceObject->renderedPoints[points[nextPoint]].x, sourceObject->renderedPoints[points[nextPoint]].y);
        }
    } else {
        point sourcePoints[] = {{sourceObject->x, sourceObject->y, sourceObject->z}, {sourceObject->x+sourceObject->size, sourceObject->y, sourceObject->z}, {sourceObject->x+sourceObject->size, sourceObject->y-sourceObject->size, sourceObject->z}, {sourceObject->x, sourceObject->y-sourceObject->size, sourceObject->z}, {sourceObject->x, sourceObject->y, sourceObject->z+sourceObject->size}, {sourceObject->x+sourceObject->size, sourceObject->y, sourceObject->z+sourceObject->size}, {sourceObject->x+sourceObject->size, sourceObject->y-sourceObject->size, sourceObject->z+sourceObject->size}, {sourceObject->x, sourceObject->y-sourceObject->size, sourceObject->z+sourceObject->size}};

        // Array of the 2 lines of the polygon we will need for this
        lineEquation lineEquations[2];
        // uint8_t* points = polygon->points;
        
        // Generate the lines from the points of the polygon
        for (unsigned int i = 0; i < 2; i++) {
            unsigned int nextPoint = (2*i) + 2;
            if (nextPoint > 3) {
                nextPoint = 0;
            }
            //lines[i] = {points[i], points[nextPoint]};
            Fixed24 dx = sourceObject->renderedPoints[points[nextPoint]].x - sourceObject->renderedPoints[points[(2*i)+1]].x;
            Fixed24 dy = sourceObject->renderedPoints[points[nextPoint]].y - sourceObject->renderedPoints[points[(2*i)+1]].y;
            Fixed24 length;

            // Find the length of the line
            // Use faster Fixed24 sqr/sqrt if the numbers are not too big
            // Leading to infinite loops caused by wrong length values
            /*if ((int)dx < 45 && (int)dy < 45) {
                dx = sqr(dx);
                dy = sqr(dy);
                if ((int)dx + (int)dy < 2047) {
                    Fixed24 value = dx + dy;
                    length = sqrt(value);
                } else {
                    length = sqrtf(powf((float)dx, 2) + powf((float)dy, 2));
                } 
            } else {
                length = sqrtf(powf((float)dx, 2) + powf((float)dy, 2));
            }*/
            length = sqrtf(powf((float)dx, 2) + powf((float)dy, 2));
            lineEquations[i] = {sourceObject->renderedPoints[points[(2*i)+1]].x, sourceObject->renderedPoints[points[nextPoint]].x, sourceObject->renderedPoints[points[(2*i)+1]].y, sourceObject->renderedPoints[points[nextPoint]].y, length};
        }
        Fixed24 length;
        if (lineEquations[1].length > lineEquations[0].length) {
            length = lineEquations[1].length;
        } else {
            length = lineEquations[0].length;
        }

        // Ratios that make the math faster (means we can multiply instead of divide)
        const Fixed24 textureRatio = length*reciprocalOf16;
        const Fixed24 reciprocalOfTextureRatio = (Fixed24)1/textureRatio;
        const Fixed24 reciprocalOfLength = (Fixed24)1/length;
        for (uint8_t i = 0; i < (int)length; i++) {
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
            const Fixed24 xDiff = ((((Fixed24)1-reciprocalOf16)*textureLine.cx) + (reciprocalOf16*textureLine.px)) - textureLine.cx;
            const Fixed24 yDiff = ((((Fixed24)1-reciprocalOf16)*textureLine.cy) + (reciprocalOf16*textureLine.py)) - textureLine.cy;
            
            // Draw each pixel of the texture row
            for (uint8_t a = 0; a < 16; a++) {
                x1 = x2;
                y1 = y2;

                // Texture undersampling for small polygons
                if (length < (Fixed24)16) {
                    a++;
                    x2 += xDiff;
                    y2 += yDiff;
                    if (length < (Fixed24)8) {
                        a++;
                        x2 += xDiff;
                        y2 += yDiff;
                    }
                }

                // Set the line color to the color of the pixel from the texture
                if (polygon->texture[row + a] != 255) {
                    gfx_SetColor(polygon->texture[row + a]);

                    // Move along the line 
                    x2 += xDiff;
                    y2 += yDiff;

                    // Convert the Fixed24's to ints
                    int lineX1 = x1;
                    int lineX2 = x2;
                    int lineY1 = y1;
                    int lineY2 = y2;
                    // this'll do, but I'd like a better way of fixing the white pixels than *blindly* overdrawing
                    if (lineY2 < 1) {
                        gfx_Line(lineX1, lineY1, lineX2, lineY2-1);
                    } else {
                        gfx_Line_NoClip(lineX1, lineY1, lineX2, lineY2-1);
                    }
                    gfx_Line_NoClip(lineX1, lineY1, lineX2, lineY2);
                }
            }
        }
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
}

int getPointDistance(point point) {
    Fixed24 x = point.x - cameraXYZ[0];
    Fixed24 y = point.y - cameraXYZ[1];
    Fixed24 z = point.z - cameraXYZ[2];
    if ((int)x < 46 && (int)y < 46 && (int)z < 46) {
        x = sqr(x);
        y = sqr(y);
        z = sqr(z);
        if ((int)x + (int)y + (int)z < 2048) {
            Fixed24 value = x + y + z;
            return sqrt(value);
        }
        return sqrtf((float)x + (float)y + (float)z);
    }
    return sqrtf(powf(x, 2) + powf(y, 2) + powf(z, 2));
}