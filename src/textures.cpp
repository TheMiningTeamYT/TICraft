#include <cstdint>
#include <graphx.h>
#include "textures.hpp"
#include <fileioc.h>
#include <ti/screen.h>
#include <ti/getcsc.h>

char* texturePackName;
uint16_t* texPalette;

// assuming textures are 16x16;
uint8_t* bedrock_texture[6];
uint8_t* bookshelf_texture[6];
uint8_t* bricks_texture[6];
uint8_t* coal_ore_texture[6];
uint8_t* cobblestone_texture[6];
uint8_t* cobblestone_mossy_texture[6];
uint8_t* crafting_table_texture[6];
uint8_t* dirt_texture[6];
uint8_t* furnace_texture[6];
uint8_t* gold_block_texture[6];
uint8_t* grass_texture[6];
uint8_t* gravel_texture[6];
uint8_t* iron_block_texture[6];
uint8_t* iron_ore_texture[6];
uint8_t* jukebox_texture[6];
uint8_t* leaves_oak_texture[6];
uint8_t* log_oak_texture[6];
uint8_t* planks_texture[6];
uint8_t* sand_texture[6];
uint8_t* smooth_stone_texture[6];
uint8_t* sponge_texture[6];
uint8_t* stone_texture[6];
uint8_t* tnt_texture[6];

// new textures
uint8_t* diamond_texture[6];
uint8_t* glass_texture[6];
uint8_t* stonebrick_texture[6];
uint8_t* emerald_texture[6];
uint8_t* lapis_texture[6];
uint8_t* pumpkin_texture[6];
uint8_t* snow_texture[6];
uint8_t* red_wool_texture[6];
uint8_t* orange_wool_texture[6];
uint8_t* yellow_wool_texture[6];
uint8_t* lime_green_wool_texture[6];
uint8_t* green_wool_texture[6];
uint8_t* cyan_wool_texture[6];
uint8_t* light_blue_wool_texture[6];
uint8_t* blue_wool_texture[6];
uint8_t* pink_wool_texture[6];
uint8_t* magenta_wool_texture[6];
uint8_t* black_wool_texture[6];
uint8_t* brown_wool_texture[6];
uint8_t* grey_wool_texture[6];
uint8_t* light_grey_wool_texture[6];
uint8_t* white_wool_texture[6];
uint8_t* quartz_texture[6];
uint8_t* hay_block_texture[6];
uint8_t* bacon_texture[6];

uint8_t** textures[] = {bedrock_texture, bookshelf_texture, bricks_texture, coal_ore_texture, cobblestone_texture, cobblestone_mossy_texture, crafting_table_texture, dirt_texture, furnace_texture, gold_block_texture, grass_texture, gravel_texture, iron_block_texture, iron_ore_texture, jukebox_texture, leaves_oak_texture, log_oak_texture, planks_texture, sand_texture, smooth_stone_texture, sponge_texture, stone_texture, tnt_texture, glass_texture, diamond_texture, lapis_texture, emerald_texture, stonebrick_texture, snow_texture, pumpkin_texture, red_wool_texture, orange_wool_texture, yellow_wool_texture, lime_green_wool_texture, green_wool_texture, cyan_wool_texture, light_blue_wool_texture, blue_wool_texture, pink_wool_texture, magenta_wool_texture, black_wool_texture, brown_wool_texture, grey_wool_texture, light_grey_wool_texture, white_wool_texture, quartz_texture, hay_block_texture, bacon_texture};

void initPalette() {
    uint8_t texturePackHandle = ti_Open(texturePackName, "r");
    texPalette = ((texturePack*)ti_GetDataPtr(texturePackHandle))->palette;
    ti_Close(texturePackHandle);
    memcpy(gfx_palette, texPalette, sizeof(uint16_t) * 256);
    uint8_t* textureMap = (uint8_t*) (texPalette + 257);
    #define bedrock textureMap + (0)
    #define bookshelf textureMap + (1*256)
    #define bricks textureMap + (2*256)
    #define coal_ore textureMap + (3*256)
    #define cobblestone textureMap + (4*256)
    #define cobblestone_mossy textureMap + (5*256)
    #define crafting_table_top textureMap + (6*256)
    #define crafting_table_front textureMap + (7*256)
    #define crafting_table_side textureMap + (8*256)
    #define dirt textureMap + (9*256)
    #define furnace_top textureMap + (10*256)
    #define furnace_front textureMap + (11*256)
    #define furnace_side textureMap + (12*256)
    #define gold_block textureMap + (13*256)
    #define grass_top textureMap + (14*256)
    #define grass_side textureMap + (15*256)
    #define gravel textureMap + (16*256)
    #define iron_block textureMap + (17*256)
    #define iron_ore textureMap + (18*256)
    #define jukebox_top textureMap + (19*256)
    #define jukebox_side textureMap + (20*256)
    #define leaves_oak textureMap + (21*256)
    #define log_oak_top textureMap + (22*256)
    #define log_oak textureMap + (23*256)
    #define planks_oak textureMap + (24*256)
    #define sand textureMap + (25*256)
    #define smooth_stone_top textureMap + (26*256)
    #define smooth_stone_side textureMap + (27*256)
    #define sponge textureMap + (28*256)
    #define stone textureMap + (29*256)
    #define tnt_top textureMap + (30*256)
    #define tnt_side textureMap + (31*256)
    #define tnt_bottom textureMap + (32*256)
    #define glass textureMap + (33*256)
    #define diamond_block textureMap + (34*256)
    #define lapis_block textureMap + (35*256)
    #define emerald_block textureMap + (36*256)
    #define stonebrick textureMap + (37*256)
    #define snow textureMap + (38*256)
    #define pumpkin_top textureMap + (39*256)
    #define pumpkin_side textureMap + (40*256)
    #define pumpkin_face textureMap + (41*256)
    #define red_wool textureMap + (42*256)
    #define orange_wool textureMap + (43*256)
    #define yellow_wool textureMap + (44*256)
    #define lime_green_wool textureMap + (45*256)
    #define green_wool textureMap + (46*256)
    #define cyan_wool textureMap + (47*256)
    #define light_blue_wool textureMap + (48*256)
    #define blue_wool textureMap + (49*256)
    #define pink_wool textureMap + (50*256)
    #define magenta_wool textureMap + (51*256)
    #define purple_wool textureMap + (52*256)
    #define black_wool textureMap + (53*256)
    #define brown_wool textureMap + (54*256)
    #define grey_wool textureMap + (55*256)
    #define light_grey_wool textureMap + (56*256)
    #define white_wool textureMap + (57*256)
    #define quartz_top textureMap + (58*256)
    #define hay_block_top textureMap + (59*256)
    #define hay_block_side textureMap + (60*256)
    #define bacon textureMap + (61*256)

    // ugh
    bedrock_texture[0] = bedrock;
    bedrock_texture[1] = bedrock;
    bedrock_texture[2] = bedrock;
    bedrock_texture[3] = bedrock;
    bedrock_texture[4] = bedrock;
    bedrock_texture[5] = bedrock;

    bookshelf_texture[0] = planks_oak;
    bookshelf_texture[1] = bookshelf;
    bookshelf_texture[2] = bookshelf;
    bookshelf_texture[3] = bookshelf;
    bookshelf_texture[4] = bookshelf;
    bookshelf_texture[5] = planks_oak;

    bricks_texture[0] = bricks;
    bricks_texture[1] = bricks;
    bricks_texture[2] = bricks;
    bricks_texture[3] = bricks;
    bricks_texture[4] = bricks;
    bricks_texture[5] = bricks;

    coal_ore_texture[0] = coal_ore;
    coal_ore_texture[1] = coal_ore;
    coal_ore_texture[2] = coal_ore;
    coal_ore_texture[3] = coal_ore;
    coal_ore_texture[4] = coal_ore;
    coal_ore_texture[5] = coal_ore;

    cobblestone_texture[0] = cobblestone;
    cobblestone_texture[1] = cobblestone;
    cobblestone_texture[2] = cobblestone;
    cobblestone_texture[3] = cobblestone;
    cobblestone_texture[4] = cobblestone;
    cobblestone_texture[5] = cobblestone;

    cobblestone_mossy_texture[0] = cobblestone_mossy;
    cobblestone_mossy_texture[1] = cobblestone_mossy;
    cobblestone_mossy_texture[2] = cobblestone_mossy;
    cobblestone_mossy_texture[3] = cobblestone_mossy;
    cobblestone_mossy_texture[4] = cobblestone_mossy;
    cobblestone_mossy_texture[5] = cobblestone_mossy;

    crafting_table_texture[0] = crafting_table_top;
    crafting_table_texture[1] = crafting_table_front;
    crafting_table_texture[2] = crafting_table_side;
    crafting_table_texture[3] = crafting_table_side;
    crafting_table_texture[4] = crafting_table_side;
    crafting_table_texture[5] = planks_oak;

    dirt_texture[0] = dirt;
    dirt_texture[1] = dirt;
    dirt_texture[2] = dirt;
    dirt_texture[3] = dirt;
    dirt_texture[4] = dirt;
    dirt_texture[5] = dirt;

    furnace_texture[0] = furnace_top;
    furnace_texture[1] = furnace_front;
    furnace_texture[2] = furnace_side;
    furnace_texture[3] = furnace_side;
    furnace_texture[4] = furnace_side;
    furnace_texture[5] = furnace_top;

    gold_block_texture[0] = gold_block;
    gold_block_texture[1] = gold_block;
    gold_block_texture[2] = gold_block;
    gold_block_texture[3] = gold_block;
    gold_block_texture[4] = gold_block;
    gold_block_texture[5] = gold_block;

    grass_texture[0] = grass_top;
    grass_texture[1] = grass_side;
    grass_texture[2] = grass_side;
    grass_texture[3] = grass_side;
    grass_texture[4] = grass_side;
    grass_texture[5] = dirt;
    
    gravel_texture[0] = gravel;
    gravel_texture[1] = gravel;
    gravel_texture[2] = gravel;
    gravel_texture[3] = gravel;
    gravel_texture[4] = gravel;
    gravel_texture[5] = gravel;

    iron_block_texture[0] = iron_block;
    iron_block_texture[1] = iron_block;
    iron_block_texture[2] = iron_block;
    iron_block_texture[3] = iron_block;
    iron_block_texture[4] = iron_block;
    iron_block_texture[5] = iron_block;

    iron_ore_texture[0] = iron_ore;
    iron_ore_texture[1] = iron_ore;
    iron_ore_texture[2] = iron_ore;
    iron_ore_texture[3] = iron_ore;
    iron_ore_texture[4] = iron_ore;
    iron_ore_texture[5] = iron_ore;

    jukebox_texture[0] = jukebox_top;
    jukebox_texture[1] = jukebox_side;
    jukebox_texture[2] = jukebox_side;
    jukebox_texture[3] = jukebox_side;
    jukebox_texture[4] = jukebox_side;
    jukebox_texture[5] = jukebox_side;

    leaves_oak_texture[0] = leaves_oak;
    leaves_oak_texture[1] = leaves_oak;
    leaves_oak_texture[2] = leaves_oak;
    leaves_oak_texture[3] = leaves_oak;
    leaves_oak_texture[4] = leaves_oak;
    leaves_oak_texture[5] = leaves_oak;

    log_oak_texture[0] = log_oak_top;
    log_oak_texture[1] = log_oak;
    log_oak_texture[2] = log_oak;
    log_oak_texture[3] = log_oak;
    log_oak_texture[4] = log_oak;
    log_oak_texture[5] = log_oak_top;

    planks_texture[0] = planks_oak;
    planks_texture[1] = planks_oak;
    planks_texture[2] = planks_oak;
    planks_texture[3] = planks_oak;
    planks_texture[4] = planks_oak;
    planks_texture[5] = planks_oak;

    sand_texture[0] = sand;
    sand_texture[1] = sand;
    sand_texture[2] = sand;
    sand_texture[3] = sand;
    sand_texture[4] = sand;
    sand_texture[5] = sand;

    smooth_stone_texture[0] = smooth_stone_top;
    smooth_stone_texture[1] = smooth_stone_side;
    smooth_stone_texture[2] = smooth_stone_side;
    smooth_stone_texture[3] = smooth_stone_side;
    smooth_stone_texture[4] = smooth_stone_side;
    smooth_stone_texture[5] = smooth_stone_top;

    sponge_texture[0] = sponge;
    sponge_texture[1] = sponge;
    sponge_texture[2] = sponge;
    sponge_texture[3] = sponge;
    sponge_texture[4] = sponge;
    sponge_texture[5] = sponge;

    stone_texture[0] = stone;
    stone_texture[1] = stone;
    stone_texture[2] = stone;
    stone_texture[3] = stone;
    stone_texture[4] = stone;
    stone_texture[5] = stone;

    tnt_texture[0] = tnt_top;
    tnt_texture[1] = tnt_side;
    tnt_texture[2] = tnt_side;
    tnt_texture[3] = tnt_side;
    tnt_texture[4] = tnt_side;
    tnt_texture[5] = tnt_bottom;

    // new textures
    diamond_texture[0] = diamond_block;
    diamond_texture[1] = diamond_block;
    diamond_texture[2] = diamond_block;
    diamond_texture[3] = diamond_block;
    diamond_texture[4] = diamond_block;
    diamond_texture[5] = diamond_block;

    glass_texture[0] = glass;
    glass_texture[1] = glass;
    glass_texture[2] = glass;
    glass_texture[3] = glass;
    glass_texture[4] = glass;
    glass_texture[5] = glass;

    stonebrick_texture[0] = stonebrick;
    stonebrick_texture[1] = stonebrick;
    stonebrick_texture[2] = stonebrick;
    stonebrick_texture[3] = stonebrick;
    stonebrick_texture[4] = stonebrick;
    stonebrick_texture[5] = stonebrick;

    emerald_texture[0] = emerald_block;
    emerald_texture[1] = emerald_block;
    emerald_texture[2] = emerald_block;
    emerald_texture[3] = emerald_block;
    emerald_texture[4] = emerald_block;
    emerald_texture[5] = emerald_block;

    lapis_texture[0] = lapis_block;
    lapis_texture[1] = lapis_block;
    lapis_texture[2] = lapis_block;
    lapis_texture[3] = lapis_block;
    lapis_texture[4] = lapis_block;
    lapis_texture[5] = lapis_block;

    pumpkin_texture[0] = pumpkin_top;
    pumpkin_texture[1] = pumpkin_face;
    pumpkin_texture[2] = pumpkin_side;
    pumpkin_texture[3] = pumpkin_side;
    pumpkin_texture[4] = pumpkin_side;
    pumpkin_texture[5] = pumpkin_top;

    snow_texture[0] = snow;
    snow_texture[1] = snow;
    snow_texture[2] = snow;
    snow_texture[3] = snow;
    snow_texture[4] = snow;
    snow_texture[5] = snow;

    red_wool_texture[0] = red_wool;
    red_wool_texture[1] = red_wool;
    red_wool_texture[2] = red_wool;
    red_wool_texture[3] = red_wool;
    red_wool_texture[4] = red_wool;
    red_wool_texture[5] = red_wool;

    orange_wool_texture[0] = orange_wool;
    orange_wool_texture[1] = orange_wool;
    orange_wool_texture[2] = orange_wool;
    orange_wool_texture[3] = orange_wool;
    orange_wool_texture[4] = orange_wool;
    orange_wool_texture[5] = orange_wool;

    yellow_wool_texture[0] = yellow_wool;
    yellow_wool_texture[1] = yellow_wool;
    yellow_wool_texture[2] = yellow_wool;
    yellow_wool_texture[3] = yellow_wool;
    yellow_wool_texture[4] = yellow_wool;
    yellow_wool_texture[5] = yellow_wool;

    lime_green_wool_texture[0] = lime_green_wool;
    lime_green_wool_texture[1] = lime_green_wool;
    lime_green_wool_texture[2] = lime_green_wool;
    lime_green_wool_texture[3] = lime_green_wool;
    lime_green_wool_texture[4] = lime_green_wool;
    lime_green_wool_texture[5] = lime_green_wool;

    green_wool_texture[0] = green_wool;
    green_wool_texture[1] = green_wool;
    green_wool_texture[2] = green_wool;
    green_wool_texture[3] = green_wool;
    green_wool_texture[4] = green_wool;
    green_wool_texture[5] = green_wool;
    
    cyan_wool_texture[0] = cyan_wool;
    cyan_wool_texture[1] = cyan_wool;
    cyan_wool_texture[2] = cyan_wool;
    cyan_wool_texture[3] = cyan_wool;
    cyan_wool_texture[4] = cyan_wool;
    cyan_wool_texture[5] = cyan_wool;

    light_blue_wool_texture[0] = light_blue_wool;
    light_blue_wool_texture[1] = light_blue_wool;
    light_blue_wool_texture[2] = light_blue_wool;
    light_blue_wool_texture[3] = light_blue_wool;
    light_blue_wool_texture[4] = light_blue_wool;
    light_blue_wool_texture[5] = light_blue_wool;

    blue_wool_texture[0] = blue_wool;
    blue_wool_texture[1] = blue_wool;
    blue_wool_texture[2] = blue_wool;
    blue_wool_texture[3] = blue_wool;
    blue_wool_texture[4] = blue_wool;
    blue_wool_texture[5] = blue_wool;

    pink_wool_texture[0] = pink_wool;
    pink_wool_texture[1] = pink_wool;
    pink_wool_texture[2] = pink_wool;
    pink_wool_texture[3] = pink_wool;
    pink_wool_texture[4] = pink_wool;
    pink_wool_texture[5] = pink_wool;

    magenta_wool_texture[0] = magenta_wool;
    magenta_wool_texture[1] = magenta_wool;
    magenta_wool_texture[2] = magenta_wool;
    magenta_wool_texture[3] = magenta_wool;
    magenta_wool_texture[4] = magenta_wool;
    magenta_wool_texture[5] = magenta_wool;

    black_wool_texture[0] = black_wool;
    black_wool_texture[1] = black_wool;
    black_wool_texture[2] = black_wool;
    black_wool_texture[3] = black_wool;
    black_wool_texture[4] = black_wool;
    black_wool_texture[5] = black_wool;

    brown_wool_texture[0] = brown_wool;
    brown_wool_texture[1] = brown_wool;
    brown_wool_texture[2] = brown_wool;
    brown_wool_texture[3] = brown_wool;
    brown_wool_texture[4] = brown_wool;
    brown_wool_texture[5] = brown_wool;

    grey_wool_texture[0] = grey_wool;
    grey_wool_texture[1] = grey_wool;
    grey_wool_texture[2] = grey_wool;
    grey_wool_texture[3] = grey_wool;
    grey_wool_texture[4] = grey_wool;
    grey_wool_texture[5] = grey_wool;

    light_grey_wool_texture[0] = light_grey_wool;
    light_grey_wool_texture[1] = light_grey_wool;
    light_grey_wool_texture[2] = light_grey_wool;
    light_grey_wool_texture[3] = light_grey_wool;
    light_grey_wool_texture[4] = light_grey_wool;
    light_grey_wool_texture[5] = light_grey_wool;

    white_wool_texture[0] = white_wool;
    white_wool_texture[1] = white_wool;
    white_wool_texture[2] = white_wool;
    white_wool_texture[3] = white_wool;
    white_wool_texture[4] = white_wool;
    white_wool_texture[5] = white_wool;

    quartz_texture[0] = quartz_top;
    quartz_texture[1] = quartz_top;
    quartz_texture[2] = quartz_top;
    quartz_texture[3] = quartz_top;
    quartz_texture[4] = quartz_top;
    quartz_texture[5] = quartz_top;

    hay_block_texture[0] = hay_block_top;
    hay_block_texture[1] = hay_block_side;
    hay_block_texture[2] = hay_block_side;
    hay_block_texture[3] = hay_block_side;
    hay_block_texture[4] = hay_block_side;
    hay_block_texture[5] = hay_block_side;

    bacon_texture[0] = bacon;
    bacon_texture[1] = bacon;
    bacon_texture[2] = bacon;
    bacon_texture[3] = bacon;
    bacon_texture[4] = bacon;
    bacon_texture[5] = bacon;
}