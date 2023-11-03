#include <cstdint>
#include <graphx.h>
#include "textures.hpp"

#include "../processing/palette.h"

#include "../processing/image.h"

static const uint8_t* bedrock = &textureMap[0];
static const uint8_t* bookshelf = &textureMap[1*256];
static const uint8_t* bricks = &textureMap[2*256];
static const uint8_t* coal_ore = &textureMap[3*256];
static const uint8_t* cobblestone = &textureMap[4*256];
static const uint8_t* cobblestone_mossy = &textureMap[5*256];
static const uint8_t* crafting_table_top = &textureMap[6*256];
static const uint8_t* crafting_table_front = &textureMap[7*256];
static const uint8_t* crafting_table_side = &textureMap[8*256];
static const uint8_t* dirt = &textureMap[9*256];
static const uint8_t* furnace_top = &textureMap[10*256];
static const uint8_t* furnace_front = &textureMap[11*256];
static const uint8_t* furnace_side = &textureMap[12*256];
static const uint8_t* gold_block = &textureMap[13*256];
static const uint8_t* grass_top = &textureMap[14*256];
static const uint8_t* grass_side = &textureMap[15*256];
static const uint8_t* gravel = &textureMap[16*256];
static const uint8_t* iron_block = &textureMap[17*256];
static const uint8_t* iron_ore = &textureMap[18*256];
static const uint8_t* jukebox_top = &textureMap[19*256];
static const uint8_t* jukebox_side = &textureMap[20*256];
static const uint8_t* leaves_oak = &textureMap[21*256];
static const uint8_t* log_oak_top = &textureMap[22*256];
static const uint8_t* log_oak = &textureMap[23*256];
static const uint8_t* planks_oak = &textureMap[24*256];
static const uint8_t* sand = &textureMap[25*256];
static const uint8_t* smooth_stone_top = &textureMap[26*256];
static const uint8_t* smooth_stone_side = &textureMap[27*256];
static const uint8_t* sponge = &textureMap[28*256];
static const uint8_t* stone = &textureMap[29*256];
static const uint8_t* tnt_top = &textureMap[30*256];
static const uint8_t* tnt_side = &textureMap[31*256];
static const uint8_t* glass = &textureMap[32*256];
static const uint8_t* diamond_block = &textureMap[33*256];
static const uint8_t* lapis_block = &textureMap[34*256];
static const uint8_t* emerald_block = &textureMap[35*256];
static const uint8_t* stonebrick = &textureMap[36*256];
static const uint8_t* snow = &textureMap[37*256];
static const uint8_t* pumpkin_top = &textureMap[38*256];
static const uint8_t* pumpkin_side = &textureMap[39*256];
static const uint8_t* red_wool = &textureMap[40*256];
static const uint8_t* orange_wool = &textureMap[41*256];
static const uint8_t* yellow_wool = &textureMap[42*256];
static const uint8_t* lime_green_wool = &textureMap[43*256];
static const uint8_t* green_wool = &textureMap[44*256];
static const uint8_t* cyan_wool = &textureMap[45*256];
static const uint8_t* light_blue_wool = &textureMap[46*256];
static const uint8_t* blue_wool = &textureMap[47*256];
static const uint8_t* pink_wool = &textureMap[48*256];
static const uint8_t* magenta_wool = &textureMap[49*256];
static const uint8_t* purple_wool = &textureMap[50*256];
static const uint8_t* black_wool = &textureMap[51*256];
static const uint8_t* brown_wool = &textureMap[52*256];
static const uint8_t* grey_wool = &textureMap[53*256];
static const uint8_t* light_grey_wool = &textureMap[54*256];
static const uint8_t* white_wool = &textureMap[55*256];
static const uint8_t* quartz_top = &textureMap[56*256];
static const uint8_t* quartz_side = &textureMap[57*256];
static const uint8_t* hay_block_top = &textureMap[58*256];
static const uint8_t* hay_block_side = &textureMap[59*256];
static const uint8_t* stonebrick_mossy = &textureMap[60*256];

// assuming textures are 16x16;
const uint8_t* bedrock_texture[] = {bedrock, bedrock, bedrock};
const uint8_t* bookshelf_texture[] = {planks_oak, bookshelf, bookshelf};
const uint8_t* bricks_texture[] = {bricks, bricks, bricks};
const uint8_t* coal_ore_texture[] = {coal_ore, coal_ore, coal_ore};
const uint8_t* cobblestone_texture[] = {cobblestone, cobblestone, cobblestone};
const uint8_t* cobblestone_mossy_texture[] = {cobblestone_mossy, cobblestone_mossy, cobblestone_mossy};
const uint8_t* crafting_table_texture[] = {crafting_table_top, crafting_table_front, crafting_table_side};
const uint8_t* dirt_texture[] = {dirt, dirt, dirt};
const uint8_t* furnace_texture[] = {furnace_top, furnace_front, furnace_side};
const uint8_t* gold_block_texture[] = {gold_block, gold_block, gold_block};
const uint8_t* grass_texture[] = {grass_top, grass_side, grass_side};
const uint8_t* gravel_texture[] = {gravel, gravel, gravel};
const uint8_t* iron_block_texture[] = {iron_block, iron_block, iron_block};
const uint8_t* iron_ore_texture[] = {iron_ore, iron_ore, iron_ore};
const uint8_t* jukebox_texture[] = {jukebox_top, jukebox_side, jukebox_side};
const uint8_t* leaves_oak_texture[] = {leaves_oak, leaves_oak, leaves_oak};
const uint8_t* log_oak_texture[] = {log_oak_top, log_oak, log_oak};
const uint8_t* planks_texture[] = {planks_oak, planks_oak, planks_oak};
const uint8_t* sand_texture[] = {sand, sand, sand};
const uint8_t* smooth_stone_texture[] = {smooth_stone_top, smooth_stone_side, smooth_stone_side};
const uint8_t* sponge_texture[] = {sponge, sponge, sponge};
const uint8_t* stone_texture[] = {stone, stone, stone};
const uint8_t* tnt_texture[] = {tnt_top, tnt_side, tnt_side};
// new textures
const uint8_t* diamond_texture[] = {diamond_block, diamond_block, diamond_block};
const uint8_t* glass_texture[] = {glass, glass, glass};
const uint8_t* stonebrick_texture[] = {stonebrick, stonebrick, stonebrick};
const uint8_t* emerald_texture[] = {emerald_block, emerald_block, emerald_block};
const uint8_t* lapis_texture[] = {lapis_block, lapis_block, lapis_block};
const uint8_t* pumpkin_texture[] = {pumpkin_top, pumpkin_side, pumpkin_side};
const uint8_t* snow_texture[] = {snow, snow, snow};
const uint8_t* red_wool_texture[] = {red_wool, red_wool, red_wool};
const uint8_t* orange_wool_texture[] = {orange_wool, orange_wool, orange_wool};
const uint8_t* yellow_wool_texture[] = {yellow_wool, yellow_wool, yellow_wool};
const uint8_t* lime_green_wool_texture[] = {lime_green_wool, lime_green_wool, lime_green_wool};
const uint8_t* green_wool_texture[] = {green_wool, green_wool, green_wool};
const uint8_t* cyan_wool_texture[] = {cyan_wool, cyan_wool, cyan_wool};
const uint8_t* light_blue_wool_texture[] = {light_blue_wool, light_blue_wool, light_blue_wool};
const uint8_t* blue_wool_texture[] = {blue_wool, blue_wool, blue_wool};
const uint8_t* pink_wool_texture[] = {pink_wool, pink_wool, pink_wool};
const uint8_t* magenta_wool_texture[] = {magenta_wool, magenta_wool, magenta_wool};
const uint8_t* black_wool_texture[] = {black_wool, black_wool, black_wool};
const uint8_t* brown_wool_texture[] = {brown_wool, brown_wool, brown_wool};
const uint8_t* grey_wool_texture[] = {grey_wool, grey_wool, grey_wool};
const uint8_t* light_grey_wool_texture[] = {light_grey_wool, light_grey_wool, light_grey_wool};
const uint8_t* white_wool_texture[] = {white_wool, white_wool, white_wool};
const uint8_t* quartz_texture[] = {quartz_top, quartz_side, quartz_side};
const uint8_t* hay_block_texture[] = {hay_block_top, hay_block_side, hay_block_side};
const uint8_t* stonebrick_mossy_texture[] = {stonebrick_mossy, stonebrick_mossy, stonebrick_mossy};

// order is likely to change in the future
const uint8_t** textures[] = {bedrock_texture, bookshelf_texture, bricks_texture, coal_ore_texture, cobblestone_texture, cobblestone_mossy_texture, crafting_table_texture, dirt_texture, furnace_texture, gold_block_texture, grass_texture, gravel_texture, iron_block_texture, iron_ore_texture, jukebox_texture, leaves_oak_texture, log_oak_texture, planks_texture, sand_texture, smooth_stone_texture, sponge_texture, stone_texture, tnt_texture, glass_texture, diamond_texture, lapis_texture, emerald_texture, stonebrick_texture, snow_texture, pumpkin_texture, red_wool_texture, orange_wool_texture, yellow_wool_texture, lime_green_wool_texture, green_wool_texture, cyan_wool_texture, light_blue_wool_texture, blue_wool_texture, pink_wool_texture, magenta_wool_texture, black_wool_texture, brown_wool_texture, grey_wool_texture, light_grey_wool_texture, white_wool_texture, quartz_texture, hay_block_texture, stonebrick_mossy_texture};