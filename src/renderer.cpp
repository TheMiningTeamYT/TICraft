#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <graphx.h>
#include "renderer.hpp"
#include "textures.hpp"
#include "saves.hpp"
#include "cursor.hpp"
#include "sincos.hpp"
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
object* objects[maxNumberOfObjects];

unsigned int numberOfObjects = 0;

unsigned int outOfBoundsPolygons = 0;

unsigned int obscuredPolygons = 0;
screenPoint renderedPoints[8];

// Faces of a cube
polygon face5 = {7, 3, 2, 6, 5};
polygon face4 = {5, 4, 7, 6, 4};
polygon face3 = {1, 5, 6, 2, 3};
polygon face2 = {0, 1, 2, 3, 2};
polygon face1 = {4, 0, 3, 7, 1};
polygon face0 = {5, 1, 0, 4, 0};

/*
Array of the polygons that make up a cube
Unwrapping order:
top, front, left, right, back, bottom
*/ 
polygon cubePolygons[] = {face0, face5, face1, face2, face3, face4};

// Position of the camera
Fixed24 cameraXYZ[3] = {-100, 150, -100};

Fixed24 angleX = 30*degRadRatio;
Fixed24 angleY = 45*degRadRatio;

Fixed24 cx;
Fixed24 sx;
Fixed24 cy;
Fixed24 sy;
Fixed24 nsxsy;
Fixed24 nsxcy;
Fixed24 cxsy;
Fixed24 cxcy;
// d means delta
Fixed24 cxd;
Fixed24 nsxd;
Fixed24 cyd;
Fixed24 nsyd;
Fixed24 nsxsyd;
Fixed24 nsxcyd;
Fixed24 cxsyd;
Fixed24 cxcyd;

uint8_t outlineColor = 0;

void object::generatePolygons() {
    if (distance < zCullingDistance) {
        generatePoints();
        if (!(properties & visible)) {
            return;
        }
        // Prepare the polygons for rendering
        for (uint8_t polygonNum = 0; polygonNum < 6; polygonNum++) {
            // The polygon we are rendering
            polygon polygon = cubePolygons[polygonNum];
            screenPoint polygonPoints[] = {renderedPoints[polygon.points[0]], renderedPoints[polygon.points[1]], renderedPoints[polygon.points[2]], renderedPoints[polygon.points[3]]};
            // I feel like those multiplications make it slower than it has to be but online resources say this is a good idea and I don't have any of those right now
            // online resources = http://www.faqs.org/faqs/graphics/algorithms-faq/
            // And this is certainly faster than sorting
            if (((polygonPoints[0].x-polygonPoints[1].x)*(polygonPoints[2].y-polygonPoints[1].y)) < ((polygonPoints[0].y-polygonPoints[1].y)*(polygonPoints[2].x-polygonPoints[1].x))) {
                uint8_t normalizedZ;
                // Are we going to render the polygon?
                bool render = false;
                if (properties & outline) {
                    render = true;
                } else {
                    // Get the average x & y of the polygon
                    int x = 0;
                    int y = 0;
                    unsigned int z = 0;
                    for (uint8_t i = 0; i < 4; i++) {
                        x += polygonPoints[i].x;
                        y += polygonPoints[i].y;
                        z += polygonPoints[i].z;
                    }
                    x >>= 2;
                    y >>= 2;
                    // Normalized z (0-255)
                    normalizedZ = polygonZShift(z);

                    if (x >= 0 && x < GFX_LCD_WIDTH && y >= 0 && y < GFX_LCD_HEIGHT) {
                        if (normalizedZ < gfx_GetPixel(x, y)) {
                            render = true;
                            goto renderThePolygon;
                        }
                    }
                    for (uint8_t i = 0; i < 4; i++) {
                        int pointX = (polygonPointMultiply(polygonPoints[i].x) + x)>>3;
                        int pointY = (polygonPointMultiply(polygonPoints[i].y) + y)>>3;
                        if (pointX >= 0 && pointX < GFX_LCD_WIDTH && pointY >= 0 && pointY < GFX_LCD_HEIGHT) {
                            if (normalizedZ < gfx_GetPixel(pointX, pointY)) {
                                render = true;
                                break;
                            }
                        }
                    }
                }
                // Render this polygon
                renderThePolygon:
                if (render) {
                    renderPolygon(this, polygonPoints, polygon.polygonNum, normalizedZ);
                }
                #if diagnostics == true
                else {
                    obscuredPolygons++;
                }
                #endif
            }
        }
    }
}

void object::deleteObject() {
    if (properties & visible) {
        generatePoints();
        gfx_SetColor(255);
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
            object* otherObject = objects[i];
            if (otherObject != this && is_visible(otherObject)) {
                otherObject->generatePoints();
                for (uint8_t pointNum = 0; pointNum < 8; pointNum++) {
                    screenPoint point = renderedPoints[pointNum];
                    if (point.x >= minX && point.x <= maxX && point.y >= minY && point.y <= maxY) {
                        otherObject->generatePolygons();
                        break;
                    }
                }
            }
        }
        gfx_SetDrawScreen();
    }
    delete this;
}

// bugs with inconsistent behavior be here.
// how... I'm not sure
// sometimes it works fine, sometimes it fails spectacularly.
// and I will never know why.
// Could use the z buffer here for some occlusion culling... hmm
void object::generatePoints() {
    // i want to rewrite this in assembly
    // because i have... little faith in the compiler
    properties &= ~visible;
    
    const Fixed24 x1 = (Fixed24)x - cameraXYZ[0];
    const Fixed24 ny1 = cameraXYZ[1] - (Fixed24)y;
    const Fixed24 z1 = (Fixed24)z - cameraXYZ[2];
    if (angleX >= static_cast<Fixed24>(45) && ny1 < (Fixed24)0) {
        return;
    }

    const Fixed24 cxsyx = cxsy*x1;
    const Fixed24 nsxy = sx*ny1;
    const Fixed24 cxcyz = cxcy*z1;
    Fixed24 dz = cxsyx + nsxy + cxcyz;
    if (dz <= (Fixed24)10) {
        return;
    }

    const Fixed24 cyx = cy*x1;
    const Fixed24 nsyz = -sy*z1;
    Fixed24 dx = cyx + nsyz;
    if (dx.abs() >= ((Fixed24)0.933610050995f)*dz + (Fixed24)30) {
        return;
    }

    const Fixed24 cxy = cx*ny1;
    const Fixed24 nsxcyz = nsxcy*z1;
    const Fixed24 nsxsyx = nsxsy*x1;
    Fixed24 dy = nsxsyx + cxy + nsxcyz;
    if (dy.abs() > ((Fixed24)0.7002075382f)*dz + (Fixed24)30) {
        return;
    }
    Fixed24 w = ((Fixed24)171.3777608f)/dz;
    renderedPoints[0] = {static_cast<int16_t>(dx*w + (Fixed24)160), static_cast<int16_t>(dy*w + (Fixed24)120), distance};

    dz += nsxd;
    if (dz <= (Fixed24)10) {
        return;
    }
    const squaredPair xSquared = findXYZSquared(x1);
    const squaredPair ySquared = findXYZSquared(ny1);
    const squaredPair zSquared = findXYZSquared(z1);
    dy += cxd;
    w = ((Fixed24)171.3777608f)/dz;
    renderedPoints[3] = {static_cast<int16_t>(dx*w + (Fixed24)160), static_cast<int16_t>(dy*w + (Fixed24)120), approx_sqrt_a(xSquared.a + ySquared.b + zSquared.a)};

    dz = cxsyx + cxsyd + nsxy + cxcyz;
    if (dz <= (Fixed24)10) {
        return;
    }
    dx += cyd;
    dy = nsxsyx + nsxsyd + cxy + nsxcyz;
    w = ((Fixed24)171.3777608f)/dz;
    renderedPoints[1] = {static_cast<int16_t>(dx*w + (Fixed24)160), static_cast<int16_t>(dy*w + (Fixed24)120), approx_sqrt_a(xSquared.b + ySquared.a + zSquared.a)};

    dz += nsxd;
    if (dz <= (Fixed24)10) {
        return;
    }
    dy += cxd;
    w = ((Fixed24)171.3777608f)/dz;
    renderedPoints[2] = {static_cast<int16_t>(dx*w + (Fixed24)160), static_cast<int16_t>(dy*w + (Fixed24)120), approx_sqrt_a(xSquared.b + ySquared.b + zSquared.a)};

    dz = cxsyx + nsxy + cxcyz + cxcyd;
    if (dz <= (Fixed24)10) {
        return;
    }
    dx = cyx + nsyz + nsyd;
    dy = nsxsyx + cxy + nsxcyz + nsxcyd;
    w = ((Fixed24)171.3777608f)/dz;
    renderedPoints[4] = {static_cast<int16_t>(dx*w + (Fixed24)160), static_cast<int16_t>(dy*w + (Fixed24)120), approx_sqrt_a(xSquared.a + ySquared.a + zSquared.b)};

    dz += nsxd;
    if (dz <= (Fixed24)10) {
        return;
    }
    dy += cxd;
    w = ((Fixed24)171.3777608f)/dz;
    renderedPoints[7] = {static_cast<int16_t>(dx*w + (Fixed24)160), static_cast<int16_t>(dy*w + (Fixed24)120), approx_sqrt_a(xSquared.a + ySquared.b + zSquared.b)};

    dz = cxsyx + cxsyd + nsxy + cxcyz + cxcyd;
    if (dz <= (Fixed24)10) {
        return;
    }
    dx = cyx + cyd + nsyz + nsyd;
    dy = nsxsyx + nsxsyd + cxy + nsxcyz + nsxcyd;
    w = ((Fixed24)171.3777608f)/dz;
    renderedPoints[5] = {static_cast<int16_t>(dx*w + (Fixed24)160), static_cast<int16_t>(dy*w + (Fixed24)120), approx_sqrt_a(xSquared.b + ySquared.a + zSquared.b)};

    dz += nsxd;
    if (dz <= (Fixed24)10) {
        return;
    }
    dy += cxd;
    w = ((Fixed24)171.3777608f)/dz;
    renderedPoints[6] = {static_cast<int16_t>(dx*w + (Fixed24)160), static_cast<int16_t>(dy*w + (Fixed24)120), approx_sqrt_a(xSquared.b + ySquared.b + zSquared.b)};
    properties |= visible;
}

void object::moveBy(int newX, int newY, int newZ) {
    x += newX;
    y += newY;
    z += newZ;
}

void object::findDistance() {
    distance = getPointDistance({x, y, z});
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
    return object1->distance - object2->distance;
}

int xCompare(const void *arg1, const void *arg2) {
    object* object1 = *((object**) arg1);
    object* object2 = *((object**) arg2);
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

void drawScreen() {
    // Disable screen updates
    *((uint8_t*)0xE30018) &= ~1;
    gfx_SetTextXY(0, 0);
    memset(gfx_vram, 255, 153600);
    #if diagnostics == true
    outOfBoundsPolygons = 0;
    obscuredPolygons = 0;
    #endif
    gfx_SetDrawBuffer();
    __asm__ ("di");
    for (unsigned int i = 0; i < numberOfObjects; i++) {
        objects[i]->generatePolygons();
    }
    gfx_SetDrawScreen();

    // Print some diagnostic information
    #if diagnostics == true
    if (mode == 0) {
        // Buffer for creating diagnostic strings
        char buffer[200];
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
    drawCursor(false);
    // Reenable screen updates
    *((uint8_t*)0xE30018) |= 1;
}

// implementation of affine texture mapping (i think thats what you'd call this anyway)
// as noted in The Science Elf's original video, affine texture mapping can look very weird on triangles
// but on quads, it looks pretty good at a fraction of the cost
// idea: integrate this into generatePolygons instead of having it as a seperate function

void renderPolygon(object* sourceObject, screenPoint* polygonRenderedPoints, uint8_t polygonNum, uint8_t normalizedZ) {
    if (is_outline(sourceObject)) {
        gfx_SetColor(outlineColor);
        // Not sure if this is faster or slower than what I was doing before
        // But it does seem to be a little smaller and it really doesn't matter in this instance
        int polygonPoints[] = {polygonRenderedPoints[0].x, polygonRenderedPoints[0].y, polygonRenderedPoints[1].x, polygonRenderedPoints[1].y, polygonRenderedPoints[2].x, polygonRenderedPoints[2].y, polygonRenderedPoints[3].x, polygonRenderedPoints[3].y};
        gfx_Polygon(polygonPoints, 4);
        return;
    }
    // result of memory optimization
    const uint8_t* texture = textures[sourceObject->texture][polygonNum];
    bool clipLines = false;
    for (unsigned int i = 0; i < 4; i++) {
        if (polygonRenderedPoints[i].x < 0 || polygonRenderedPoints[i].x > GFX_LCD_WIDTH - 1 || polygonRenderedPoints[i].y < 0 || polygonRenderedPoints[i].y > GFX_LCD_HEIGHT) {
            clipLines = true;
            break;
        }
    }
    const uint8_t colorOffset = (polygonNum == 2 || polygonNum == 5) ? 126 : 0;
    // Generate the lines from the points of the polygon
    int x0 = polygonRenderedPoints[0].x;
    int y0 = polygonRenderedPoints[0].y;
    int x1 = polygonRenderedPoints[1].x;
    int y1 = polygonRenderedPoints[1].y;
    int dx0 = polygonRenderedPoints[3].x - x0;
    int dy0 = polygonRenderedPoints[3].y - y0;
    int dx1 = polygonRenderedPoints[2].x - x1;
    int dy1 = polygonRenderedPoints[2].y - y1;
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
    for (int i = -2; i < length; i++) {
        if (clipLines) {
            if (!((x0 < 0 && x1 < 0) || (x0 > (GFX_LCD_WIDTH - 1) && x1 > (GFX_LCD_WIDTH - 1))) && !((y0 < 0 && y1 < 0) || (y0 > (GFX_LCD_HEIGHT - 1) && y1 > (GFX_LCD_HEIGHT - 1))))
                drawTextureLineNewA(x0, x1, y0, y1, texture + tIndex, colorOffset, normalizedZ);
        } else {
            drawTextureLineNewA_NoClip(x0, x1, y0, y1, texture + tIndex, colorOffset, normalizedZ);
        }
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
        while (tError <= 0 && tIndex < 240) {
            tError += length;
            tIndex += 16;
        }
        tError += -16;
    }
}

int getPointDistance(point point) {
    int x = point.x - cameraXYZ[0];
    int y = point.y - cameraXYZ[1];
    int z = point.z - cameraXYZ[2];
    return approx_sqrt_a((x*x)+(y*y)+(z*z));
}

void rotateCamera(Fixed24 x, Fixed24 y) {
    bool rotated = false;
    if ((x != static_cast<Fixed24>(0)) && (angleX + x >= static_cast<Fixed24>(-90.0f*degRadRatio) && angleX + x <= static_cast<Fixed24>(90.0f*degRadRatio))) {
        angleX += x;
        cx = fastSinCos::cos(angleX);
        cxd = cx*(Fixed24)cubeSize;
        sx = fastSinCos::sin(angleX);
        nsxd = sx*(Fixed24)cubeSize;
        rotated = true;
    }
    if (y != static_cast<Fixed24>(0)) {
        angleY += y;
        if (angleY > static_cast<Fixed24>(270.0f*degRadRatio)) {
            angleY -= static_cast<Fixed24>(360.0f*degRadRatio);
        }
        if (angleY < static_cast<Fixed24>(-90.0f*degRadRatio)) {
            angleY += static_cast<Fixed24>(360.0f*degRadRatio);
        }
        sy = fastSinCos::sin(angleY);
        nsyd = sy*(Fixed24)-cubeSize;
        cy = fastSinCos::cos(angleY);
        cyd = cy*(Fixed24)cubeSize;
        rotated = true;
    }
    if (rotated) {
        cxsy = cx*sy;
        cxsyd = cxsy*(Fixed24)cubeSize;
        cxcy = cx*cy;
        cxcyd = cxcy*(Fixed24)cubeSize;
        nsxsy = -sx*sy;
        nsxsyd = nsxsy*(Fixed24)cubeSize;
        nsxcy = -sx*cy;
        nsxcyd = nsxcy*(Fixed24)cubeSize;
        drawBuffer();
        drawScreen();
    }
}

void redrawScreen() {
    // Disable screen updates
    *((uint8_t*)0xE30018) &= ~1;
    zSort();
    drawScreen();
}

void zSort() {
    for (unsigned int i = 0; i < numberOfObjects; i++) {
        objects[i]->findDistance();
    }
    // This qsort must go for the sake of performance
    // but how do you replace it?
    qsort(objects, numberOfObjects, sizeof(object*), distanceCompare);
}

void resetCamera() {
    angleX = 30.0f*degRadRatio;
    angleY = 45.0f*degRadRatio;
    cx = 0.8660254037844387f;
    sx = 0.49999999999999994f;
    cy = 0.7071067811865476f;
    sy = 0.7071067811865476f;
    nsxsy = -0.353553391f;
    nsxcy = -0.353553391f;
    cxsy = 0.612372436f;
    cxcy = 0.612372436f;
    // d means delta
    cxd = 0.8660254037844387f*static_cast<float>(cubeSize);
    nsxd = 0.49999999999999994f*static_cast<float>(cubeSize);
    cyd = 0.7071067811865476f*static_cast<float>(cubeSize);
    nsyd = -0.7071067811865476f*static_cast<float>(cubeSize);
    nsxsyd = -0.353553391f*static_cast<float>(cubeSize);
    nsxcyd = -0.353553391f*static_cast<float>(cubeSize);
    cxsyd = 0.612372436f*static_cast<float>(cubeSize);
    cxcyd = 0.612372436f*static_cast<float>(cubeSize);
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