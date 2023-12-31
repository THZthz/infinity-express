#ifndef CANDYBOX_COLOR_HPP__
#define CANDYBOX_COLOR_HPP__

#include <string>
#include <cstdint>

#ifdef _MSC_VER
#	pragma warning(push)
#	pragma warning(disable : 4201) // nonstandard extension used : nameless struct/union
#endif

namespace candybox {

// ██████╗ ██████╗ ██╗      ██████╗ ██████╗
//██╔════╝██╔═══██╗██║     ██╔═══██╗██╔══██╗
//██║     ██║   ██║██║     ██║   ██║██████╔╝
//██║     ██║   ██║██║     ██║   ██║██╔══██╗
//╚██████╗╚██████╔╝███████╗╚██████╔╝██║  ██║
// ╚═════╝ ╚═════╝ ╚══════╝ ╚═════╝ ╚═╝  ╚═╝

//! \defgroup
//! @{

/// RGB values, 0xff max for each channel.
enum class Colors : uint32_t
{
	SNOW = 0xfffafa,
	GHOSTWHITE = 0xf8f8ff,
	WHITESMOKE = 0xf5f5f5,
	GAINSBORO = 0xdcdcdc,
	FLORALWHITE = 0xfffaf0,
	OLDLACE = 0xfdf5e6,
	LINEN = 0xfaf0e6,
	ANTIQUEWHITE = 0xfaebd7,
	PAPAYAWHIP = 0xffefd5,
	BLANCHEDALMOND = 0xffebcd,
	BISQUE = 0xffe4c4,
	PEACHPUFF = 0xffdab9,
	NAVAJOWHITE = 0xffdead,
	MOCCASIN = 0xffe4b5,
	CORNSILK = 0xfff8dc,
	IVORY = 0xfffff0,
	LEMONCHIFFON = 0xfffacd,
	SEASHELL = 0xfff5ee,
	HONEYDEW = 0xf0fff0,
	MINTCREAM = 0xf5fffa,
	AZURE = 0xf0ffff,
	ALICEBLUE = 0xf0f8ff,
	LAVENDER = 0xe6e6fa,
	LAVENDERBLUSH = 0xfff0f5,
	MISTYROSE = 0xffe4e1,
	WHITE = 0xffffff,
	BLACK = 0x000000,
	DARKSLATEGRAY = 0x2f4f4f,
	DARKSLATEGREY = 0x2f4f4f,
	DIMGRAY = 0x696969,
	DIMGREY = 0x696969,
	SLATEGRAY = 0x708090,
	SLATEGREY = 0x708090,
	LIGHTSLATEGRAY = 0x778899,
	LIGHTSLATEGREY = 0x778899,
	GRAY = 0xbebebe,
	GREY = 0xbebebe,
	X11GRAY = 0xbebebe,
	X11GREY = 0xbebebe,
	WEBGRAY = 0x808080,
	WEBGREY = 0x808080,
	LIGHTGREY = 0xd3d3d3,
	LIGHTGRAY = 0xd3d3d3,
	MIDNIGHTBLUE = 0x191970,
	NAVY = 0x000080,
	NAVYBLUE = 0x000080,
	CORNFLOWERBLUE = 0x6495ed,
	DARKSLATEBLUE = 0x483d8b,
	SLATEBLUE = 0x6a5acd,
	MEDIUMSLATEBLUE = 0x7b68ee,
	LIGHTSLATEBLUE = 0x8470ff,
	MEDIUMBLUE = 0x0000cd,
	ROYALBLUE = 0x4169e1,
	BLUE = 0x0000ff,
	DODGERBLUE = 0x1e90ff,
	DEEPSKYBLUE = 0x00bfff,
	SKYBLUE = 0x87ceeb,
	LIGHTSKYBLUE = 0x87cefa,
	STEELBLUE = 0x4682b4,
	LIGHTSTEELBLUE = 0xb0c4de,
	LIGHTBLUE = 0xadd8e6,
	POWDERBLUE = 0xb0e0e6,
	PALETURQUOISE = 0xafeeee,
	DARKTURQUOISE = 0x00ced1,
	MEDIUMTURQUOISE = 0x48d1cc,
	TURQUOISE = 0x40e0d0,
	CYAN = 0x00ffff,
	AQUA = 0x00ffff,
	LIGHTCYAN = 0xe0ffff,
	CADETBLUE = 0x5f9ea0,
	MEDIUMAQUAMARINE = 0x66cdaa,
	AQUAMARINE = 0x7fffd4,
	DARKGREEN = 0x006400,
	DARKOLIVEGREEN = 0x556b2f,
	DARKSEAGREEN = 0x8fbc8f,
	SEAGREEN = 0x2e8b57,
	MEDIUMSEAGREEN = 0x3cb371,
	LIGHTSEAGREEN = 0x20b2aa,
	PALEGREEN = 0x98fb98,
	SPRINGGREEN = 0x00ff7f,
	LAWNGREEN = 0x7cfc00,
	GREEN = 0x00ff00,
	LIME = 0x00ff00,
	X11GREEN = 0x00ff00,
	WEBGREEN = 0x008000,
	CHARTREUSE = 0x7fff00,
	MEDIUMSPRINGGREEN = 0x00fa9a,
	GREENYELLOW = 0xadff2f,
	LIMEGREEN = 0x32cd32,
	YELLOWGREEN = 0x9acd32,
	FORESTGREEN = 0x228b22,
	OLIVEDRAB = 0x6b8e23,
	DARKKHAKI = 0xbdb76b,
	KHAKI = 0xf0e68c,
	PALEGOLDENROD = 0xeee8aa,
	LIGHTGOLDENRODYELLOW = 0xfafad2,
	LIGHTYELLOW = 0xffffe0,
	YELLOW = 0xffff00,
	GOLD = 0xffd700,
	LIGHTGOLDENROD = 0xeedd82,
	GOLDENROD = 0xdaa520,
	DARKGOLDENROD = 0xb8860b,
	ROSYBROWN = 0xbc8f8f,
	INDIANRED = 0xcd5c5c,
	SADDLEBROWN = 0x8b4513,
	SIENNA = 0xa0522d,
	PERU = 0xcd853f,
	BURLYWOOD = 0xdeb887,
	BEIGE = 0xf5f5dc,
	WHEAT = 0xf5deb3,
	SANDYBROWN = 0xf4a460,
	TAN = 0xd2b48c,
	CHOCOLATE = 0xd2691e,
	FIREBRICK = 0xb22222,
	BROWN = 0xa52a2a,
	DARKSALMON = 0xe9967a,
	SALMON = 0xfa8072,
	LIGHTSALMON = 0xffa07a,
	ORANGE = 0xffa500,
	DARKORANGE = 0xff8c00,
	CORAL = 0xff7f50,
	LIGHTCORAL = 0xf08080,
	TOMATO = 0xff6347,
	ORANGERED = 0xff4500,
	RED = 0xff0000,
	HOTPINK = 0xff69b4,
	DEEPPINK = 0xff1493,
	PINK = 0xffc0cb,
	LIGHTPINK = 0xffb6c1,
	PALEVIOLETRED = 0xdb7093,
	MAROON = 0xb03060,
	X11MAROON = 0xb03060,
	WEBMAROON = 0x800000,
	MEDIUMVIOLETRED = 0xc71585,
	VIOLETRED = 0xd02090,
	MAGENTA = 0xff00ff,
	FUCHSIA = 0xff00ff,
	VIOLET = 0xee82ee,
	PLUM = 0xdda0dd,
	ORCHID = 0xda70d6,
	MEDIUMORCHID = 0xba55d3,
	DARKORCHID = 0x9932cc,
	DARKVIOLET = 0x9400d3,
	BLUEVIOLET = 0x8a2be2,
	PURPLE = 0xa020f0,
	X11PURPLE = 0xa020f0,
	WEBPURPLE = 0x800080,
	MEDIUMPURPLE = 0x9370db,
	THISTLE = 0xd8bfd8,
	SNOW1 = 0xfffafa,
	SNOW2 = 0xeee9e9,
	SNOW3 = 0xcdc9c9,
	SNOW4 = 0x8b8989,
	SEASHELL1 = 0xfff5ee,
	SEASHELL2 = 0xeee5de,
	SEASHELL3 = 0xcdc5bf,
	SEASHELL4 = 0x8b8682,
	ANTIQUEWHITE1 = 0xffefdb,
	ANTIQUEWHITE2 = 0xeedfcc,
	ANTIQUEWHITE3 = 0xcdc0b0,
	ANTIQUEWHITE4 = 0x8b8378,
	BISQUE1 = 0xffe4c4,
	BISQUE2 = 0xeed5b7,
	BISQUE3 = 0xcdb79e,
	BISQUE4 = 0x8b7d6b,
	PEACHPUFF1 = 0xffdab9,
	PEACHPUFF2 = 0xeecbad,
	PEACHPUFF3 = 0xcdaf95,
	PEACHPUFF4 = 0x8b7765,
	NAVAJOWHITE1 = 0xffdead,
	NAVAJOWHITE2 = 0xeecfa1,
	NAVAJOWHITE3 = 0xcdb38b,
	NAVAJOWHITE4 = 0x8b795e,
	LEMONCHIFFON1 = 0xfffacd,
	LEMONCHIFFON2 = 0xeee9bf,
	LEMONCHIFFON3 = 0xcdc9a5,
	LEMONCHIFFON4 = 0x8b8970,
	CORNSILK1 = 0xfff8dc,
	CORNSILK2 = 0xeee8cd,
	CORNSILK3 = 0xcdc8b1,
	CORNSILK4 = 0x8b8878,
	IVORY1 = 0xfffff0,
	IVORY2 = 0xeeeee0,
	IVORY3 = 0xcdcdc1,
	IVORY4 = 0x8b8b83,
	HONEYDEW1 = 0xf0fff0,
	HONEYDEW2 = 0xe0eee0,
	HONEYDEW3 = 0xc1cdc1,
	HONEYDEW4 = 0x838b83,
	LAVENDERBLUSH1 = 0xfff0f5,
	LAVENDERBLUSH2 = 0xeee0e5,
	LAVENDERBLUSH3 = 0xcdc1c5,
	LAVENDERBLUSH4 = 0x8b8386,
	MISTYROSE1 = 0xffe4e1,
	MISTYROSE2 = 0xeed5d2,
	MISTYROSE3 = 0xcdb7b5,
	MISTYROSE4 = 0x8b7d7b,
	AZURE1 = 0xf0ffff,
	AZURE2 = 0xe0eeee,
	AZURE3 = 0xc1cdcd,
	AZURE4 = 0x838b8b,
	SLATEBLUE1 = 0x836fff,
	SLATEBLUE2 = 0x7a67ee,
	SLATEBLUE3 = 0x6959cd,
	SLATEBLUE4 = 0x473c8b,
	ROYALBLUE1 = 0x4876ff,
	ROYALBLUE2 = 0x436eee,
	ROYALBLUE3 = 0x3a5fcd,
	ROYALBLUE4 = 0x27408b,
	BLUE1 = 0x0000ff,
	BLUE2 = 0x0000ee,
	BLUE3 = 0x0000cd,
	BLUE4 = 0x00008b,
	DODGERBLUE1 = 0x1e90ff,
	DODGERBLUE2 = 0x1c86ee,
	DODGERBLUE3 = 0x1874cd,
	DODGERBLUE4 = 0x104e8b,
	STEELBLUE1 = 0x63b8ff,
	STEELBLUE2 = 0x5cacee,
	STEELBLUE3 = 0x4f94cd,
	STEELBLUE4 = 0x36648b,
	DEEPSKYBLUE1 = 0x00bfff,
	DEEPSKYBLUE2 = 0x00b2ee,
	DEEPSKYBLUE3 = 0x009acd,
	DEEPSKYBLUE4 = 0x00688b,
	SKYBLUE1 = 0x87ceff,
	SKYBLUE2 = 0x7ec0ee,
	SKYBLUE3 = 0x6ca6cd,
	SKYBLUE4 = 0x4a708b,
	LIGHTSKYBLUE1 = 0xb0e2ff,
	LIGHTSKYBLUE2 = 0xa4d3ee,
	LIGHTSKYBLUE3 = 0x8db6cd,
	LIGHTSKYBLUE4 = 0x607b8b,
	SLATEGRAY1 = 0xc6e2ff,
	SLATEGRAY2 = 0xb9d3ee,
	SLATEGRAY3 = 0x9fb6cd,
	SLATEGRAY4 = 0x6c7b8b,
	LIGHTSTEELBLUE1 = 0xcae1ff,
	LIGHTSTEELBLUE2 = 0xbcd2ee,
	LIGHTSTEELBLUE3 = 0xa2b5cd,
	LIGHTSTEELBLUE4 = 0x6e7b8b,
	LIGHTBLUE1 = 0xbfefff,
	LIGHTBLUE2 = 0xb2dfee,
	LIGHTBLUE3 = 0x9ac0cd,
	LIGHTBLUE4 = 0x68838b,
	LIGHTCYAN1 = 0xe0ffff,
	LIGHTCYAN2 = 0xd1eeee,
	LIGHTCYAN3 = 0xb4cdcd,
	LIGHTCYAN4 = 0x7a8b8b,
	PALETURQUOISE1 = 0xbbffff,
	PALETURQUOISE2 = 0xaeeeee,
	PALETURQUOISE3 = 0x96cdcd,
	PALETURQUOISE4 = 0x668b8b,
	CADETBLUE1 = 0x98f5ff,
	CADETBLUE2 = 0x8ee5ee,
	CADETBLUE3 = 0x7ac5cd,
	CADETBLUE4 = 0x53868b,
	TURQUOISE1 = 0x00f5ff,
	TURQUOISE2 = 0x00e5ee,
	TURQUOISE3 = 0x00c5cd,
	TURQUOISE4 = 0x00868b,
	CYAN1 = 0x00ffff,
	CYAN2 = 0x00eeee,
	CYAN3 = 0x00cdcd,
	CYAN4 = 0x008b8b,
	DARKSLATEGRAY1 = 0x97ffff,
	DARKSLATEGRAY2 = 0x8deeee,
	DARKSLATEGRAY3 = 0x79cdcd,
	DARKSLATEGRAY4 = 0x528b8b,
	AQUAMARINE1 = 0x7fffd4,
	AQUAMARINE2 = 0x76eec6,
	AQUAMARINE3 = 0x66cdaa,
	AQUAMARINE4 = 0x458b74,
	DARKSEAGREEN1 = 0xc1ffc1,
	DARKSEAGREEN2 = 0xb4eeb4,
	DARKSEAGREEN3 = 0x9bcd9b,
	DARKSEAGREEN4 = 0x698b69,
	SEAGREEN1 = 0x54ff9f,
	SEAGREEN2 = 0x4eee94,
	SEAGREEN3 = 0x43cd80,
	SEAGREEN4 = 0x2e8b57,
	PALEGREEN1 = 0x9aff9a,
	PALEGREEN2 = 0x90ee90,
	PALEGREEN3 = 0x7ccd7c,
	PALEGREEN4 = 0x548b54,
	SPRINGGREEN1 = 0x00ff7f,
	SPRINGGREEN2 = 0x00ee76,
	SPRINGGREEN3 = 0x00cd66,
	SPRINGGREEN4 = 0x008b45,
	GREEN1 = 0x00ff00,
	GREEN2 = 0x00ee00,
	GREEN3 = 0x00cd00,
	GREEN4 = 0x008b00,
	CHARTREUSE1 = 0x7fff00,
	CHARTREUSE2 = 0x76ee00,
	CHARTREUSE3 = 0x66cd00,
	CHARTREUSE4 = 0x458b00,
	OLIVEDRAB1 = 0xc0ff3e,
	OLIVEDRAB2 = 0xb3ee3a,
	OLIVEDRAB3 = 0x9acd32,
	OLIVEDRAB4 = 0x698b22,
	DARKOLIVEGREEN1 = 0xcaff70,
	DARKOLIVEGREEN2 = 0xbcee68,
	DARKOLIVEGREEN3 = 0xa2cd5a,
	DARKOLIVEGREEN4 = 0x6e8b3d,
	KHAKI1 = 0xfff68f,
	KHAKI2 = 0xeee685,
	KHAKI3 = 0xcdc673,
	KHAKI4 = 0x8b864e,
	LIGHTGOLDENROD1 = 0xffec8b,
	LIGHTGOLDENROD2 = 0xeedc82,
	LIGHTGOLDENROD3 = 0xcdbe70,
	LIGHTGOLDENROD4 = 0x8b814c,
	LIGHTYELLOW1 = 0xffffe0,
	LIGHTYELLOW2 = 0xeeeed1,
	LIGHTYELLOW3 = 0xcdcdb4,
	LIGHTYELLOW4 = 0x8b8b7a,
	YELLOW1 = 0xffff00,
	YELLOW2 = 0xeeee00,
	YELLOW3 = 0xcdcd00,
	YELLOW4 = 0x8b8b00,
	GOLD1 = 0xffd700,
	GOLD2 = 0xeec900,
	GOLD3 = 0xcdad00,
	GOLD4 = 0x8b7500,
	GOLDENROD1 = 0xffc125,
	GOLDENROD2 = 0xeeb422,
	GOLDENROD3 = 0xcd9b1d,
	GOLDENROD4 = 0x8b6914,
	DARKGOLDENROD1 = 0xffb90f,
	DARKGOLDENROD2 = 0xeead0e,
	DARKGOLDENROD3 = 0xcd950c,
	DARKGOLDENROD4 = 0x8b6508,
	ROSYBROWN1 = 0xffc1c1,
	ROSYBROWN2 = 0xeeb4b4,
	ROSYBROWN3 = 0xcd9b9b,
	ROSYBROWN4 = 0x8b6969,
	INDIANRED1 = 0xff6a6a,
	INDIANRED2 = 0xee6363,
	INDIANRED3 = 0xcd5555,
	INDIANRED4 = 0x8b3a3a,
	SIENNA1 = 0xff8247,
	SIENNA2 = 0xee7942,
	SIENNA3 = 0xcd6839,
	SIENNA4 = 0x8b4726,
	BURLYWOOD1 = 0xffd39b,
	BURLYWOOD2 = 0xeec591,
	BURLYWOOD3 = 0xcdaa7d,
	BURLYWOOD4 = 0x8b7355,
	WHEAT1 = 0xffe7ba,
	WHEAT2 = 0xeed8ae,
	WHEAT3 = 0xcdba96,
	WHEAT4 = 0x8b7e66,
	TAN1 = 0xffa54f,
	TAN2 = 0xee9a49,
	TAN3 = 0xcd853f,
	TAN4 = 0x8b5a2b,
	CHOCOLATE1 = 0xff7f24,
	CHOCOLATE2 = 0xee7621,
	CHOCOLATE3 = 0xcd661d,
	CHOCOLATE4 = 0x8b4513,
	FIREBRICK1 = 0xff3030,
	FIREBRICK2 = 0xee2c2c,
	FIREBRICK3 = 0xcd2626,
	FIREBRICK4 = 0x8b1a1a,
	BROWN1 = 0xff4040,
	BROWN2 = 0xee3b3b,
	BROWN3 = 0xcd3333,
	BROWN4 = 0x8b2323,
	SALMON1 = 0xff8c69,
	SALMON2 = 0xee8262,
	SALMON3 = 0xcd7054,
	SALMON4 = 0x8b4c39,
	LIGHTSALMON1 = 0xffa07a,
	LIGHTSALMON2 = 0xee9572,
	LIGHTSALMON3 = 0xcd8162,
	LIGHTSALMON4 = 0x8b5742,
	ORANGE1 = 0xffa500,
	ORANGE2 = 0xee9a00,
	ORANGE3 = 0xcd8500,
	ORANGE4 = 0x8b5a00,
	DARKORANGE1 = 0xff7f00,
	DARKORANGE2 = 0xee7600,
	DARKORANGE3 = 0xcd6600,
	DARKORANGE4 = 0x8b4500,
	CORAL1 = 0xff7256,
	CORAL2 = 0xee6a50,
	CORAL3 = 0xcd5b45,
	CORAL4 = 0x8b3e2f,
	TOMATO1 = 0xff6347,
	TOMATO2 = 0xee5c42,
	TOMATO3 = 0xcd4f39,
	TOMATO4 = 0x8b3626,
	ORANGERED1 = 0xff4500,
	ORANGERED2 = 0xee4000,
	ORANGERED3 = 0xcd3700,
	ORANGERED4 = 0x8b2500,
	RED1 = 0xff0000,
	RED2 = 0xee0000,
	RED3 = 0xcd0000,
	RED4 = 0x8b0000,
	DEEPPINK1 = 0xff1493,
	DEEPPINK2 = 0xee1289,
	DEEPPINK3 = 0xcd1076,
	DEEPPINK4 = 0x8b0a50,
	HOTPINK1 = 0xff6eb4,
	HOTPINK2 = 0xee6aa7,
	HOTPINK3 = 0xcd6090,
	HOTPINK4 = 0x8b3a62,
	PINK1 = 0xffb5c5,
	PINK2 = 0xeea9b8,
	PINK3 = 0xcd919e,
	PINK4 = 0x8b636c,
	LIGHTPINK1 = 0xffaeb9,
	LIGHTPINK2 = 0xeea2ad,
	LIGHTPINK3 = 0xcd8c95,
	LIGHTPINK4 = 0x8b5f65,
	PALEVIOLETRED1 = 0xff82ab,
	PALEVIOLETRED2 = 0xee799f,
	PALEVIOLETRED3 = 0xcd6889,
	PALEVIOLETRED4 = 0x8b475d,
	MAROON1 = 0xff34b3,
	MAROON2 = 0xee30a7,
	MAROON3 = 0xcd2990,
	MAROON4 = 0x8b1c62,
	VIOLETRED1 = 0xff3e96,
	VIOLETRED2 = 0xee3a8c,
	VIOLETRED3 = 0xcd3278,
	VIOLETRED4 = 0x8b2252,
	MAGENTA1 = 0xff00ff,
	MAGENTA2 = 0xee00ee,
	MAGENTA3 = 0xcd00cd,
	MAGENTA4 = 0x8b008b,
	ORCHID1 = 0xff83fa,
	ORCHID2 = 0xee7ae9,
	ORCHID3 = 0xcd69c9,
	ORCHID4 = 0x8b4789,
	PLUM1 = 0xffbbff,
	PLUM2 = 0xeeaeee,
	PLUM3 = 0xcd96cd,
	PLUM4 = 0x8b668b,
	MEDIUMORCHID1 = 0xe066ff,
	MEDIUMORCHID2 = 0xd15fee,
	MEDIUMORCHID3 = 0xb452cd,
	MEDIUMORCHID4 = 0x7a378b,
	DARKORCHID1 = 0xbf3eff,
	DARKORCHID2 = 0xb23aee,
	DARKORCHID3 = 0x9a32cd,
	DARKORCHID4 = 0x68228b,
	PURPLE1 = 0x9b30ff,
	PURPLE2 = 0x912cee,
	PURPLE3 = 0x7d26cd,
	PURPLE4 = 0x551a8b,
	MEDIUMPURPLE1 = 0xab82ff,
	MEDIUMPURPLE2 = 0x9f79ee,
	MEDIUMPURPLE3 = 0x8968cd,
	MEDIUMPURPLE4 = 0x5d478b,
	THISTLE1 = 0xffe1ff,
	THISTLE2 = 0xeed2ee,
	THISTLE3 = 0xcdb5cd,
	THISTLE4 = 0x8b7b8b,
	GRAY0 = 0x000000,
	GREY0 = 0x000000,
	GRAY1 = 0x030303,
	GREY1 = 0x030303,
	GRAY2 = 0x050505,
	GREY2 = 0x050505,
	GRAY3 = 0x080808,
	GREY3 = 0x080808,
	GRAY4 = 0x0a0a0a,
	GREY4 = 0x0a0a0a,
	GRAY5 = 0x0d0d0d,
	GREY5 = 0x0d0d0d,
	GRAY6 = 0x0f0f0f,
	GREY6 = 0x0f0f0f,
	GRAY7 = 0x121212,
	GREY7 = 0x121212,
	GRAY8 = 0x141414,
	GREY8 = 0x141414,
	GRAY9 = 0x171717,
	GREY9 = 0x171717,
	GRAY10 = 0x1a1a1a,
	GREY10 = 0x1a1a1a,
	GRAY11 = 0x1c1c1c,
	GREY11 = 0x1c1c1c,
	GRAY12 = 0x1f1f1f,
	GREY12 = 0x1f1f1f,
	GRAY13 = 0x212121,
	GREY13 = 0x212121,
	GRAY14 = 0x242424,
	GREY14 = 0x242424,
	GRAY15 = 0x262626,
	GREY15 = 0x262626,
	GRAY16 = 0x292929,
	GREY16 = 0x292929,
	GRAY17 = 0x2b2b2b,
	GREY17 = 0x2b2b2b,
	GRAY18 = 0x2e2e2e,
	GREY18 = 0x2e2e2e,
	GRAY19 = 0x303030,
	GREY19 = 0x303030,
	GRAY20 = 0x333333,
	GREY20 = 0x333333,
	GRAY21 = 0x363636,
	GREY21 = 0x363636,
	GRAY22 = 0x383838,
	GREY22 = 0x383838,
	GRAY23 = 0x3b3b3b,
	GREY23 = 0x3b3b3b,
	GRAY24 = 0x3d3d3d,
	GREY24 = 0x3d3d3d,
	GRAY25 = 0x404040,
	GREY25 = 0x404040,
	GRAY26 = 0x424242,
	GREY26 = 0x424242,
	GRAY27 = 0x454545,
	GREY27 = 0x454545,
	GRAY28 = 0x474747,
	GREY28 = 0x474747,
	GRAY29 = 0x4a4a4a,
	GREY29 = 0x4a4a4a,
	GRAY30 = 0x4d4d4d,
	GREY30 = 0x4d4d4d,
	GRAY31 = 0x4f4f4f,
	GREY31 = 0x4f4f4f,
	GRAY32 = 0x525252,
	GREY32 = 0x525252,
	GRAY33 = 0x545454,
	GREY33 = 0x545454,
	GRAY34 = 0x575757,
	GREY34 = 0x575757,
	GRAY35 = 0x595959,
	GREY35 = 0x595959,
	GRAY36 = 0x5c5c5c,
	GREY36 = 0x5c5c5c,
	GRAY37 = 0x5e5e5e,
	GREY37 = 0x5e5e5e,
	GRAY38 = 0x616161,
	GREY38 = 0x616161,
	GRAY39 = 0x636363,
	GREY39 = 0x636363,
	GRAY40 = 0x666666,
	GREY40 = 0x666666,
	GRAY41 = 0x696969,
	GREY41 = 0x696969,
	GRAY42 = 0x6b6b6b,
	GREY42 = 0x6b6b6b,
	GRAY43 = 0x6e6e6e,
	GREY43 = 0x6e6e6e,
	GRAY44 = 0x707070,
	GREY44 = 0x707070,
	GRAY45 = 0x737373,
	GREY45 = 0x737373,
	GRAY46 = 0x757575,
	GREY46 = 0x757575,
	GRAY47 = 0x787878,
	GREY47 = 0x787878,
	GRAY48 = 0x7a7a7a,
	GREY48 = 0x7a7a7a,
	GRAY49 = 0x7d7d7d,
	GREY49 = 0x7d7d7d,
	GRAY50 = 0x7f7f7f,
	GREY50 = 0x7f7f7f,
	GRAY51 = 0x828282,
	GREY51 = 0x828282,
	GRAY52 = 0x858585,
	GREY52 = 0x858585,
	GRAY53 = 0x878787,
	GREY53 = 0x878787,
	GRAY54 = 0x8a8a8a,
	GREY54 = 0x8a8a8a,
	GRAY55 = 0x8c8c8c,
	GREY55 = 0x8c8c8c,
	GRAY56 = 0x8f8f8f,
	GREY56 = 0x8f8f8f,
	GRAY57 = 0x919191,
	GREY57 = 0x919191,
	GRAY58 = 0x949494,
	GREY58 = 0x949494,
	GRAY59 = 0x969696,
	GREY59 = 0x969696,
	GRAY60 = 0x999999,
	GREY60 = 0x999999,
	GRAY61 = 0x9c9c9c,
	GREY61 = 0x9c9c9c,
	GRAY62 = 0x9e9e9e,
	GREY62 = 0x9e9e9e,
	GRAY63 = 0xa1a1a1,
	GREY63 = 0xa1a1a1,
	GRAY64 = 0xa3a3a3,
	GREY64 = 0xa3a3a3,
	GRAY65 = 0xa6a6a6,
	GREY65 = 0xa6a6a6,
	GRAY66 = 0xa8a8a8,
	GREY66 = 0xa8a8a8,
	GRAY67 = 0xababab,
	GREY67 = 0xababab,
	GRAY68 = 0xadadad,
	GREY68 = 0xadadad,
	GRAY69 = 0xb0b0b0,
	GREY69 = 0xb0b0b0,
	GRAY70 = 0xb3b3b3,
	GREY70 = 0xb3b3b3,
	GRAY71 = 0xb5b5b5,
	GREY71 = 0xb5b5b5,
	GRAY72 = 0xb8b8b8,
	GREY72 = 0xb8b8b8,
	GRAY73 = 0xbababa,
	GREY73 = 0xbababa,
	GRAY74 = 0xbdbdbd,
	GREY74 = 0xbdbdbd,
	GRAY75 = 0xbfbfbf,
	GREY75 = 0xbfbfbf,
	GRAY76 = 0xc2c2c2,
	GREY76 = 0xc2c2c2,
	GRAY77 = 0xc4c4c4,
	GREY77 = 0xc4c4c4,
	GRAY78 = 0xc7c7c7,
	GREY78 = 0xc7c7c7,
	GRAY79 = 0xc9c9c9,
	GREY79 = 0xc9c9c9,
	GRAY80 = 0xcccccc,
	GREY80 = 0xcccccc,
	GRAY81 = 0xcfcfcf,
	GREY81 = 0xcfcfcf,
	GRAY82 = 0xd1d1d1,
	GREY82 = 0xd1d1d1,
	GRAY83 = 0xd4d4d4,
	GREY83 = 0xd4d4d4,
	GRAY84 = 0xd6d6d6,
	GREY84 = 0xd6d6d6,
	GRAY85 = 0xd9d9d9,
	GREY85 = 0xd9d9d9,
	GRAY86 = 0xdbdbdb,
	GREY86 = 0xdbdbdb,
	GRAY87 = 0xdedede,
	GREY87 = 0xdedede,
	GRAY88 = 0xe0e0e0,
	GREY88 = 0xe0e0e0,
	GRAY89 = 0xe3e3e3,
	GREY89 = 0xe3e3e3,
	GRAY90 = 0xe5e5e5,
	GREY90 = 0xe5e5e5,
	GRAY91 = 0xe8e8e8,
	GREY91 = 0xe8e8e8,
	GRAY92 = 0xebebeb,
	GREY92 = 0xebebeb,
	GRAY93 = 0xededed,
	GREY93 = 0xededed,
	GRAY94 = 0xf0f0f0,
	GREY94 = 0xf0f0f0,
	GRAY95 = 0xf2f2f2,
	GREY95 = 0xf2f2f2,
	GRAY96 = 0xf5f5f5,
	GREY96 = 0xf5f5f5,
	GRAY97 = 0xf7f7f7,
	GREY97 = 0xf7f7f7,
	GRAY98 = 0xfafafa,
	GREY98 = 0xfafafa,
	GRAY99 = 0xfcfcfc,
	GREY99 = 0xfcfcfc,
	GRAY100 = 0xffffff,
	GREY100 = 0xffffff,
	DARKGREY = 0xa9a9a9,
	DARKGRAY = 0xa9a9a9,
	DARKBLUE = 0x00008b,
	DARKCYAN = 0x008b8b,
	DARKMAGENTA = 0x8b008b,
	DARKRED = 0x8b0000,
	LIGHTGREEN = 0x90ee90,
	CRIMSON = 0xdc143c,
	INDIGO = 0x4b0082,
	OLIVE = 0x808000,
	REBECCAPURPLE = 0x663399,
	SILVER = 0xc0c0c0,
	TEAL = 0x008080
};

struct Colorf
{
	float r, g, b, a;
};

struct ColorIndex
{
	std::string name;
	unsigned char r, g, b;
};

class Color
{
public:
	union
	{
		unsigned int c;
		unsigned char rgba[4];
		struct
		{
			unsigned char r, g, b, a;
		};
	};

	Color() : r(0), g(0), b(0), a(0) { }

	/// Return color by using 24bits RGB value. Alpha is set to 255.
	explicit Color(Colors color);

	/// Returns a color value from red, green, blue values.
	/// Alpha will be set to 255.
	Color(unsigned char r_, unsigned char g_, unsigned char b_) : r(r_), g(g_), b(b_), a(255)
	{
	}

	/// Returns a color value from red, green, blue and alpha values.
	Color(unsigned char r_, unsigned char g_, unsigned char b_, unsigned char a_)
	    : r(r_), g(g_), b(b_), a(a_)
	{
	}

	/// Returns a color value from red, green, blue values. Alpha will be set to 1.0f.
	Color(float r_, float g_, float b_);

	/// Returns a color value from red, green, blue and alpha values.
	Color(float r_, float g_, float b_, float a_);

	/// Render color from name.
	/// If the color of the given name does not exist, return RGB(0, 0, 0),
	/// which is a fully opaque black.
	explicit Color(const char *name);

	/*----------------------------------------------------------------------------*/

	bool operator==(const Color &c) const;

	Color &operator=(const Colors &rhs);

	bool Equal(const Color &c) const;

	/// Linearly interpolates from color c0 to c1, and returns resulting color value.
	static Color Lerp(Color c0, Color c1, float u);

	/// Sets transparency of a color value.
	Color &Trans(unsigned char a_);

	/// Sets transparency of a color value.
	Color &Trans(float a_);

	/// Averages the color channels into one value
	float Luminance() const;

	// http://www.w3.org/TR/AERT#color-contrast
	float Brightness() const;

	bool Dark() const;

	bool Light() const;

	bool Black() const;

	static Color Rainbow(float t);

	/// Convert RGB to HSL(not accounting for alpha).
	void ToHSL(float &h, float &s, float &l) const;

	const Color &Brighten(int amount = 10);

	const Color &Lighten(int amount = 10);

	const Color &Darken(int amount = 10);

	/// Spin takes a positive or negative amount within [-360, 360] indicating the change of hue.
	/// Values outside of this range will be wrapped into this range.
	const Color &Spin(int amount = 180);

	// convert a single color channel (R,G,B) from sRGB color space to linear RGB color space
	static float SRGBtoLinear(unsigned char c);

	static Color Rand(uint32_t i);

	static const struct ColorIndex *LookUpColorIndex(const char *str, unsigned int len);

	static float Hue(float h, float m1, float m2);
};

/// Returns color value specified by hue, saturation and lightness.
/// HSL values are all in range [0..1], alpha will be set to 1.
Color HSL(float h, float s, float l);

/// Returns color value specified by hue, saturation and lightness and alpha.
/// HSL values are all in range [0..1], alpha in range [0..1].
/// See http://marcocorvi.altervista.org/games/imgpr/rgb-hsl.htm.
Color HSLA(float h, float s, float l, float a);

Color RGB_uc(unsigned char r_, unsigned char g_, unsigned char b_);

Color RGBA(unsigned char r_, unsigned char g_, unsigned char b_, unsigned char a_);

inline bool
Color::Equal(const Color &c) const
{
	return r == c.r && g == c.g && b == c.b && a == c.a;
}

inline bool
Color::operator==(const Color &c) const
{
	return Equal(c);
}

inline Color &
Color::operator=(const Colors &rhs)
{
	r = ((uint32_t)rhs & 0xff0000) >> 16;
	g = ((uint32_t)rhs & 0x00ff00) >> 8;
	b = (uint32_t)rhs & 0x0000ff;
	a = 255;
	return *this;
}

inline float
Color::Brightness() const
{
	return (float)(r * 299 + g * 587 + b * 114) / 1000.f;
}

inline bool
Color::Dark() const
{
	return Brightness() < 128;
}

inline bool
Color::Light() const
{
	return !Dark();
}

inline bool
Color::Black() const
{
	return r == 0 && g == 0 && b == 0 && a == 0;
}

//! @}

} // namespace candybox

#ifdef _MSC_VER
#	pragma warning(pop)
#endif

#endif // CANDYBOX_COLOR_HPP__
