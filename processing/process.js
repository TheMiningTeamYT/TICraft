const Jimp = require("jimp");
const fs = require('fs');
Jimp.read("TextureMap.png").then((image) => { 
    let dithering = false;
    let paletteSize = 126;
    let tolerence = 15;
    let imageData = image.bitmap;
    let errorRed = 0;
    let errorGreen = 0;
    let errorBlue = 0;
    for (var i = 0; i < imageData.data.length; i = i) {
        let red = ((imageData.data[i] >> 3) + (errorRed >> 3))<<3;
        let green = ((imageData.data[i + 1] >> 3) + (errorGreen >> 3))<<3;
        let blue = ((imageData.data[i + 2] >> 3) + (errorBlue >> 3))<<3;
        if (dithering == true) {
            errorRed = (imageData.data[i] + errorRed) - red;
            errorGreen = (imageData.data[i + 1] + errorGreen) - green;
            errorBlue = (imageData.data[i + 2] + errorBlue) - blue;
        }
        imageData[i] = red;
        imageData[i + 1] = green;
        imageData[i + 2] = blue;
        imageData.data[i + 3] = 255;
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
                diffColors.push(Math.abs(red - colors[a][0]) + Math.abs(green - colors[a][1]) + Math.abs(blue - colors[a][2]));
            }
            let closestColor = colors[diffColors.indexOf(Math.min(...diffColors))];
            if (Math.abs(red - closestColor[0]) < tolerence && Math.abs(green - closestColor[1]) < tolerence && Math.abs(blue - closestColor[2]) < tolerence) {
                closestColor[3] += 1;
            } else {
                colors.push([red, green, blue, 1]);
            }
        } else {
            colors.push([red, green, blue, 1]);
        }
        i += 4;
    }
    let palette = colors.sort((a, b) => b[3] - a[3]).slice(0, paletteSize).sort((a, b) => (a[0] - b[0]));
    let truePalette = "uint16_t texPalette[] = {";
    for (var i = 0; i < palette.length; i++) {
        truePalette += "0x" + ((((palette[i][0] >> 3) << 10) + ((palette[i][1] >> 3) << 5) + (palette[i][2] >> 3)).toString(16));
        truePalette += ", ";
        if (i%16 >= 15) {
            truePalette += "\n";
        }
    }
    truePalette += "\n";
    for (var i = 0; i < palette.length; i++) {
        truePalette += "0x" + (((((palette[i][0] * 0.6) >> 3) << 10) + (((palette[i][1] * 0.6) >> 3) << 5) + ((palette[i][2] * 0.6) >> 3)).toString(16));
        truePalette += ", ";
        if (i%16 >= 15) {
            truePalette += "\n";
        }
    }
    truePalette += "gfx_RGBTo1555(179,179,179),\ngfx_RGBTo1555(105,105,105),\ngfx_RGBTo1555(255,255,255),\ngfx_RGBTo1555(135,206,235)\n};"
    let indexedImage = "static const uint8_t textureMap[] = {\n";
    errorRed = 0;
    errorGreen = 0;
    errorBlue = 0;
    for (var i = 0; i < imageData.data.length; i = i) {
        let originalRed = (imageData.data[i] + errorRed);
        let originalGreen = (imageData.data[i + 1] + errorGreen);
        let originalBlue = (imageData.data[i + 2] + errorBlue);
        let diffPalette = [];
        for (var a = 0; a < palette.length; a++) {
            diffPalette.push(Math.abs(originalRed - palette[a][0]) + Math.abs(originalGreen - palette[a][1]) + Math.abs(originalBlue - palette[a][2]));
        }
        let index = diffPalette.indexOf(Math.min(...diffPalette));
        if (((i >= 21504 && i <= 22528) || (i >= 32768 && i <= 33792)) && index == 0) {
            indexedImage += "255";
        } else {
            indexedImage += index.toString(10);
        }
        if (i < imageData.data.length - 5) {
            indexedImage += ", ";
        }
        if (i % 32 >= 27) {
            indexedImage += "\n";
        }
        let closestPaletteColor = palette[index];
        let red = (closestPaletteColor[0]);
        let green = (closestPaletteColor[1]);
        let blue = (closestPaletteColor[2]);
        if (dithering == true) {
            if (i % 16 == 0 || (i >= 57344 && i <= 59392)) {
                errorRed = 0;
                errorGreen = 0;
                errorBlue = 0;
            } else {
                errorRed = (originalRed - red) % 256;
                errorGreen = (originalGreen - green) % 256;
                errorBlue = (originalBlue - blue) % 256;
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
    image.write("TextureMap--Converted.png");
});