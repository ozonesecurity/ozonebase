#ifndef LIBIMG_RGB_H
#define LIBIMG_RGB_H

typedef unsigned int Rgb;   // RGB colour type
typedef unsigned int Yuv;   // RGB colour type

#define RGB_R_OFF(ptr)  (*(ptr))
#define RGB_G_OFF(ptr)  (*(ptr+1))
#define RGB_B_OFF(ptr)  (*(ptr+2))

#define RED(ptr)    (*(ptr))
#define GREEN(ptr)  (*(ptr+1))
#define BLUE(ptr)   (*(ptr+2))

#define RED16(ptr)  (*(ptr))
#define GREEN16(ptr)    (*(ptr+2))
#define BLUE16(ptr) (*(ptr+4))

#define WHITE       0xff
#define WHITE_R     0xff
#define WHITE_G     0xff
#define WHITE_B     0xff

#define BLACK       0x00
#define BLACK_R     0x00
#define BLACK_G     0x00
#define BLACK_B     0x00

#define RGB_WHITE       (0x00ffffff)
#define RGB_BLACK       (0x00000000)
#define RGB_RED         (0x00ff0000)
#define RGB_GREEN       (0x0000ff00)
#define RGB_BLUE        (0x000000ff)
#define RGB_ORANGE      (0x00ffa500)
#define RGB_YELLOW      (0x00ffff00)
#define RGB_MAGENTA     (0x00ff00ff)
#define RGB_CYAN        (0x0000ffff)
#define RGB_PURPLE      (0x00800080)
#define RGB_TRANSPARENT (0x01000000)

#define RGB_VAL(v,c)        (((v)>>(16-((c)*8)))&0xff)
#define RGB_RED_VAL(v)      (((v)>>16)&0xff)
#define RGB_GREEN_VAL(v)    (((v)>>8)&0xff)
#define RGB_BLUE_VAL(v)     ((v)&0xff)

#endif // LIBIMG_RGB_H
