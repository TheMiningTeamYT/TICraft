const Jimp = require("jimp");
const fs = require('fs');
const { exec } = require("child_process");
Jimp.read("TextureMap.png").then((image) => { 
    let dithering = false;
    let paletteSize = 126;
    let tolerence = 300;
    let imageData = image.bitmap;
    let errorRed = 0;
    let errorGreen = 0;
    let errorBlue = 0;
    for (var i = 0; i < imageData.data.length; i = i) {
        let red = (linearToRGB(rgbToLinear(imageData.data[i]) + errorRed)>>3)<<3;
        let green = (linearToRGB(rgbToLinear(imageData.data[i + 1]) + errorGreen)>>3)<<3;
        let blue = (linearToRGB(rgbToLinear(imageData.data[i + 2]) + errorBlue)>>3)<<3;
        if (dithering == true) {
            errorRed = rgbToLinear(red) - (rgbToLinear(imageData.data[i]) + errorRed);
            errorGreen = rgbToLinear(green) - (rgbToLinear(imageData.data[i + 1]) + errorGreen);
            errorBlue = rgbToLinear(blue) - (rgbToLinear(imageData.data[i + 2]) + errorBlue);
        }
        imageData[i] = red;
        imageData[i + 1] = green;
        imageData[i + 2] = blue;
        i += 4
    }
    let colors = [[0,0,0,1]];
    for (var i = 0; i < imageData.data.length; i = i) {
        let red = imageData.data[i];
        let green = imageData.data[i + 1];
        let blue = imageData.data[i + 2];
        if (colors.length > 0) {
            let diffColors = [];
            for (var a = 0; a < colors.length; a++) {
                diffColors.push(Math.pow(red - colors[a][0], 2) + Math.pow(green - colors[a][1], 2) + Math.pow(blue - colors[a][2], 2));
            }
            let closestColor = colors[diffColors.indexOf(Math.min(...diffColors))];
            if (Math.pow(red - closestColor[0], 2) + Math.pow(green - closestColor[1], 2) + Math.pow(blue - closestColor[2], 2) < tolerence) {
                closestColor[3] += 1;
            } else {
                colors.push([red, green, blue, 1]);
            }
        } else {
            colors.push([red, green, blue, 1]);
        }
        i += 4;
    }
    console.log(colors.length);
    let palette = colors.sort((a, b) => b[3] - a[3]).slice(0, paletteSize).sort((a, b) => ((rgbToLinear(a[0]) + rgbToLinear(a[1]) + rgbToLinear(a[2])) - (rgbToLinear(b[0]) + rgbToLinear(b[1]) + rgbToLinear(b[2]))));
    let paletteBuffer = new Uint16Array(256);
    let truePalette = "uint16_t texPalette[] = {";
    for (var i = 0; i < palette.length; i++) {
        paletteBuffer[i] = (((palette[i][0] >> 3) << 10) + ((palette[i][1] >> 3) << 5) + (palette[i][2] >> 3));
        truePalette += "0x" + paletteBuffer[i].toString(16);
        truePalette += ", ";
        if (i%16 >= 15) {
            truePalette += "\n";
        }
    }
    truePalette += "\n";
    for (var i = 0; i < palette.length; i++) {
        paletteBuffer[i + palette.length] = (((darkenColor(palette[i][0]) >> 3) << 10) + ((darkenColor(palette[i][1]) >> 3) << 5) + (darkenColor(palette[i][2]) >> 3));
        truePalette += "0x" + (paletteBuffer[i + palette.length].toString(16));
        truePalette += ", ";
        if (i%16 >= 15) {
            truePalette += "\n";
        }
    }
    paletteBuffer[252] = 23254;
    paletteBuffer[253] = 13741;
    paletteBuffer[254] = 32767;
    paletteBuffer[255] = 17213;
    truePalette += "gfx_RGBTo1555(179,179,179),\ngfx_RGBTo1555(105,105,105),\ngfx_RGBTo1555(255,255,255),\ngfx_RGBTo1555(135,206,235)\n};"
    let indexedImage = "static const uint8_t textureMap[] = {\n";
    let indexedImageBuffer = new Uint8Array(imageData.width * imageData.height);
    errorRed = 0;
    errorGreen = 0;
    errorBlue = 0;
    for (var i = 0; i < imageData.data.length; i = i) {
        let originalRed = linearToRGB(rgbToLinear(imageData.data[i]) + errorRed);
        let originalGreen = linearToRGB(rgbToLinear(imageData.data[i + 1]) + errorGreen);
        let originalBlue = linearToRGB(rgbToLinear(imageData.data[i + 2]) + errorBlue);
        let diffPalette = [];
        for (var a = 0; a < palette.length; a++) {
            diffPalette.push(Math.pow(originalRed - palette[a][0], 2) + Math.pow(originalGreen - palette[a][1], 2) + Math.pow(originalBlue - palette[a][2], 2));
        }
        let index = diffPalette.indexOf(Math.min(...diffPalette));
        if (imageData.data[i + 3] < 128) {
            indexedImage += "255";
            indexedImageBuffer[i/4] = 255;
        } else {
            indexedImage += index.toString(10);
            indexedImageBuffer[i/4] = index;
        }
        if (i < imageData.data.length - 5) {
            indexedImage += ", ";
        }
        if (i % 64 >= 59) {
            indexedImage += "\n";
        }
        if (i % 1024 >= 1019) {
            indexedImage += "\n";
        }
        let closestPaletteColor = palette[index];
        let red = (closestPaletteColor[0]);
        let green = (closestPaletteColor[1]);
        let blue = (closestPaletteColor[2]);
        if (dithering == true) {
            if (i % 16 == 0) {
                errorRed = 0;
                errorGreen = 0;
                errorBlue = 0;
            } else {
                errorRed = (rgbToLinear(red) - rgbToLinear(originalRed));
                errorGreen = (rgbToLinear(green) - rgbToLinear(originalGreen));
                errorBlue = (rgbToLinear(blue) - rgbToLinear(originalBlue));
            }
        }
        imageData.data[i] = red;
        imageData.data[i + 1] = green;
        imageData.data[i + 2] = blue;
        i += 4;
    }
    indexedImage += "};";
    fs.writeFileSync("palette.h", truePalette);
    fs.writeFileSync("image.h", indexedImage);
    fs.writeFileSync("image.bin", paletteBuffer);
    fs.appendFileSync("image.bin", indexedImageBuffer);
    exec("convbin -j bin -k 8xv -i image.bin -o textures.8xv -n TEXTURES");
    image.write("TextureMap--Converted.png");
});

function darkenColor(color) {
    return linearToRGB(rgbToLinear(color)*0.4);
}

function rgbToLinear(color) {
    return Math.pow(color, 2.2);
}

function linearToRGB(color) {
    if (color < 0) {
        return 0;
    }
    return Math.pow(color, 1/2.2);
}