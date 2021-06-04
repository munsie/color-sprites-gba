#ifndef __SPRITES_HH__
#define __SPRITES_HH__

#include <gba.h>
#include <cstdint>

class Sprite {
public:
  friend class SpriteManager;
  
  void setPos(int16_t x, int16_t y) {
    _attr0 &= 0xff00;
    _attr0 |= (y & 0xff);

    _attr1 &= 0xfe00;
    _attr1 |= (x & 0x1ff);
  }

  int16_t x() const {
    int16_t x = _attr1 & 0x1ff;
    if(x & 0x100) {
      x |= 0xfe00;
    }
    return x;
  }
  
  int16_t y() const {
    int16_t y = _attr0 & 0xff;
    if(y & 0x80) {
      y |= 0xff00;
    }
    return y;
  }

  void setTileIndex(uint16_t index) {
    _attr2 &= ~0x03ff;
    _attr2 |= index & 0x03ff;
  }

  uint16_t tileIndex() const {
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
  uint16_t _attr0 = 0;
  uint16_t _attr1 = 0;
  uint16_t _attr2 = 0;
  uint16_t _attr3 = 0;

private:
  Sprite() {}
};

class SpriteManager {
public:
  static inline SpriteManager &instance() {
    static SpriteManager instance;
    return instance;
  }
  
  Sprite *newSprite() {
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

  void freeSprite(Sprite *sprite) {
    sprite->setPos(SCREEN_WIDTH, SCREEN_HEIGHT);
    unsigned index = ((unsigned)(sprite - _sprites)) / sizeof(Sprite);
    _freeSprites[index >> 5] |= 1 << (index & 0x1f);
  }

  void updateAll() {
    dmaCopy((uint16_t *)_sprites, (uint16_t *)OAM, sizeof(_sprites));
  }

  static const unsigned NumSprites = 128;

protected:
  Sprite _sprites[NumSprites];
  uint32_t _freeSprites[4];

private:
  SpriteManager() {
    for(unsigned i = 0; i < NumSprites; i++) {
      _sprites[i].setPos(SCREEN_WIDTH, SCREEN_HEIGHT);
    }
    _freeSprites[0] = 0xffffffff;
    _freeSprites[1] = 0xffffffff;
    _freeSprites[2] = 0xffffffff;
    _freeSprites[3] = 0xffffffff;
  }
};
#endif