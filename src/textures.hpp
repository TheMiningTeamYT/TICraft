#include <cstdint>
#include <cstring>
#include <graphx.h>
#define TICRAFTTexturePackVersion 3
#define TICRAFTTexturePackMagic "TICRAFT_TEXTURES"
struct texturePack {
    char magic[17];
    uint16_t version;
    uint16_t palette[256];
    uint16_t shadedSkyColor;
    uint8_t textures[15872];
    uint16_t icon[1024];
    char metadata[];
    /*
    metadata:
    char name[];
    char description[];
    char author[];
    char version[];
    char copyright[];
    uint32_t crc;
    */
};

extern char* texturePackName;
extern uint16_t* texPalette;
extern uint8_t* bedrock_texture[];
extern uint8_t* bookshelf_texture[];
extern uint8_t* bricks_texture[];
extern uint8_t* coal_ore_texture[];
extern uint8_t* cobblestone_texture[];
extern uint8_t* cobblestone_mossy_texture[];
extern uint8_t* crafting_table_texture[];
extern uint8_t* dirt_texture[];
extern uint8_t* furnace_texture[];
extern uint8_t* gold_block_texture[];
extern uint8_t* grass_texture[];
extern uint8_t* gravel_texture[];
extern uint8_t* iron_block_texture[];
extern uint8_t* iron_ore_texture[];
extern uint8_t* jukebox_texture[];
extern uint8_t* leaves_oak_texture[];
extern uint8_t* log_oak_texture[];
extern uint8_t* planks_texture[];
extern uint8_t* sand_texture[];
extern uint8_t* smooth_stone_texture[];
extern uint8_t* sponge_texture[];
extern uint8_t* stone_texture[];
extern uint8_t* cursor_texture[];
extern uint8_t* diamond_texture[];
extern uint8_t* glass_texture[];
extern uint8_t* stonebrick_texture[];
extern uint8_t* emerald_texture[];
extern uint8_t* lapis_texture[];
extern uint8_t* pumpkin_texture[];
extern uint8_t* snow_texture[];
extern uint8_t* red_wool_texture[];
extern uint8_t* orange_wool_texture[];
extern uint8_t* yellow_wool_texture[];
extern uint8_t* lime_green_wool_texture[];
extern uint8_t* green_wool_texture[];
extern uint8_t* cyan_wool_texture[];
extern uint8_t* light_blue_wool_texture[];
extern uint8_t* blue_wool_texture[];
extern uint8_t* pink_wool_texture[];
extern uint8_t* magenta_wool_texture[];
extern uint8_t* black_wool_texture[];
extern uint8_t* brown_wool_texture[];
extern uint8_t* grey_wool_texture[];
extern uint8_t* light_grey_wool_texture[];
extern uint8_t* white_wool_texture[];
extern uint8_t* quartz_texture[];

extern uint8_t** textures[];

void initPalette();