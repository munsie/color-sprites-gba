#include <gba.h>

class Sprite {
public:
  Sprite() {} 

  void setPos(s16 x, s16 y) {
    _attr0 &= 0xff00;
    _attr0 |= (y & 0xff);

    _attr1 &= 0xfe00;
    _attr1 |= (x & 0x1ff);
  }

  s16 x() const {
    s16 x = _attr1 & 0x1ff;
    if(x & 0x100) {
      x |= 0xfe00;
    }
    return x;
  }
  
  s16 y() const {
    s16 y = _attr0 & 0xff;
    if(y & 0x80) {
      y |= 0xff00;
    }
    return y;
  }

  void setTileIndex(u16 index) {
    _attr2 &= ~0x03ff;
    _attr2 |= index & 0x03ff;
  }

  u16 tileIndex() const {
    return _attr2 & 0x03ff;
  }

  void setPalette(u8 index) {
    _attr2 &= ~0xf000;
    _attr2 |= (index & 0xf) << 12;
  }

  void setVFlip(bool flag) {
    if(flag) {
      _attr1 |= 0x2000;
    } else {
      _attr1 &= ~0x2000;
    }
  }

  void setHFlip(bool flag) {
    if(flag) {
      _attr1 |= 0x1000;
    } else {
      _attr1 &= ~0x1000;
    }
  }

protected:
  u16 _attr0 = 0;
  u16 _attr1 = 0;
  u16 _attr2 = 0;
  u16 _attr3 = 0;

private:
};

class SpriteManager {
public:
  static void init() {
    for(unsigned i = 0; i < NumSprites; i++) {
      _sprites[i].setPos(SCREEN_WIDTH, SCREEN_HEIGHT);
    }
    _freeSprites[0] = 0xffffffff;
    _freeSprites[1] = 0xffffffff;
    _freeSprites[2] = 0xffffffff;
    _freeSprites[3] = 0xffffffff;
  }

  static Sprite *newSprite() {
    // find the next free sprite
    int index = -1;
    for(unsigned i = 0; i < 4; i++) {
      if(_freeSprites[i] != 0) {
        auto bit = __builtin_clz(_freeSprites[i]);
        index = bit + (i * 32);
        _freeSprites[i] &= ~(1 << (31 - bit)); 
        break;
      }
    }
    return (index == -1) ? nullptr : &_sprites[index];
  }

  static void freeSprite(Sprite *sprite) {
    sprite->setPos(SCREEN_WIDTH, SCREEN_HEIGHT);
    unsigned index = ((unsigned)(sprite - _sprites)) / sizeof(Sprite);
    _freeSprites[index >> 5] |= 1 << (index & 0x1f);
  }

  static void updateAll() {
    dmaCopy((u16 *)_sprites, (u16 *)OAM, sizeof(_sprites));
  }

  static const unsigned NumSprites = 128;

protected:
  static Sprite _sprites[NumSprites];
  static u32 _freeSprites[4];

private:
};

Sprite SpriteManager::_sprites[NumSprites];
u32 SpriteManager::_freeSprites[4];

static s32 fixsin[360];

static const u16 SinCos[] = {
  0x0000, 0x0477, 0x08EF, 0x0D65, 0x11DB, 0x164F, 0x1AC2, 0x1F32,
  0x23A0, 0x280C, 0x2C74, 0x30D8, 0x3539, 0x3996, 0x3DEE, 0x4241,
  0x4690, 0x4AD8, 0x4F1B, 0x5358, 0x578E, 0x5BBE, 0x5FE6, 0x6406,
  0x681F, 0x6C30, 0x7039, 0x7438, 0x782F, 0x7C1C, 0x7FFF, 0x83D9,
  0x87A8, 0x8B6D, 0x8F27, 0x92D5, 0x9679, 0x9A10, 0x9D9B, 0xA11B,
  0xA48D, 0xA7F3, 0xAB4C, 0xAE97, 0xB1D5, 0xB504, 0xB826, 0xBB39,
  0xBE3E, 0xC134, 0xC41B, 0xC6F3, 0xC9BB, 0xCC73, 0xCF1B, 0xD1B3,
  0xD43B, 0xD6B3, 0xD919, 0xDB6F, 0xDDB3, 0xDFE7, 0xE208, 0xE419,
  0xE617, 0xE803, 0xE9DE, 0xEBA6, 0xED5B, 0xEEFF, 0xF08F, 0xF20D,
  0xF378, 0xF4D0, 0xF615, 0xF746, 0xF865, 0xF970, 0xFA67, 0xFB4B,
  0xFC1C, 0xFCD9, 0xFD82, 0xFE17, 0xFE98, 0xFF06, 0xFF60, 0xFFA6,
  0xFFD8, 0xFFF6
};

static void initSinCosTables() {
  for(int i = 1; i < 90; i++) {
    fixsin[i]       = (s32)SinCos[i];
    fixsin[i + 90]  = (s32)SinCos[90 - i];
    fixsin[i + 180] = (s32)((~SinCos[i] + 1) | 0xffff0000);
    fixsin[i + 270] = (s32)((~SinCos[90 - i] + 1) | 0xffff0000);
  }

  fixsin[90] = (s32)0x00010000;
  fixsin[180] = (s32)0x00000000;
  fixsin[270] = (s32)0xffff0000;
  fixsin[0] = (s32)0x00000000;
}

static const unsigned NumColors = 6;
static const unsigned NumSpritesPerColor = 16;

int main() {
  // turn off the display
  REG_DISPCNT = MODE_0;
  
  initSinCosTables();

  // initialize our sprite tiles
  for(unsigned i = 0; i < 16; i++) {
    u8 *tile = (u8 *)SPR_VRAM(i);
    for(unsigned y = 0; y < 8; y+= 2) {
      for(unsigned x = 0; x < 4; x++) {
        tile[((y + 0) << 2) + x] = i;
        tile[((y + 1) << 2) + x] = i << 4;
      }
    }
  }

  // initialize the color table
  static const unsigned ColorLUT[] = {
    0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xc, 0xe, 0x11, 0x13, 0x15, 0x17, 0x19, 0x1b, 0x1d, 0x1f,
  };
  for(unsigned i = 0; i < 16; i++) {
    unsigned c = ColorLUT[i];
    OBJ_COLORS[ 0 + i] = RGB5(c, 0, 0);
    OBJ_COLORS[16 + i] = RGB5(0, c, 0);
    OBJ_COLORS[32 + i] = RGB5(0, 0, c);
    OBJ_COLORS[48 + i] = RGB5(c, c, 0);
    OBJ_COLORS[64 + i] = RGB5(c, 0, c);
    OBJ_COLORS[80 + i] = RGB5(0, c, c);
  }

  BG_COLORS[0] = RGB5(0, 0, 0);

  // setup the sprites
  SpriteManager::init();

  Sprite *sprites[NumColors * NumSpritesPerColor];
  for(int i = NumSpritesPerColor - 1; i >= 0; i--) {
    for(unsigned c = 0; c < NumColors; c++) {
      auto sprite = SpriteManager::newSprite();
      sprite->setPos((16 * i) - (c * 8), 80);
      sprite->setTileIndex(i);
      sprite->setPalette(c);
      sprite->setHFlip(c & 1);
      sprites[(c << 4) + i] = sprite;
    }
  }

  // turn on the display
  REG_DISPCNT = MODE_0 | OBJ_ENABLE | OBJ_1D_MAP;

  irqInit();
  irqEnable(IRQ_VBLANK);

  unsigned angle = 0;
  for( ; ; ) {
    angle++;
    for(unsigned i = 0; i < NumSpritesPerColor; i++) {
      for(unsigned c = 0; c < NumColors; c++) {
        auto sprite = sprites[(c << 4) + i];
        // get the current position
        auto x = sprite->x() + 1;
        if(x > SCREEN_WIDTH) {
          x -= SCREEN_WIDTH + 8;
        }
        int aa = ((angle * (c + 1)) + (i * 16) + (45 * c)) % 360;
        s16 y = ((int64_t)fixsin[aa] * (int64_t)(40 << 16) >> 32) + 76;
        sprite->setPos(x, y);
      }
    }

    // wait for vblank before we update all the sprite in OAM
    VBlankIntrWait();
    SpriteManager::updateAll();
  }

  return 0;
}
