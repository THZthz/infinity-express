#include <cstring>
#include <cassert>
#include "utils/Linear.hpp"
#include "utils/Color.hpp"
using namespace ie;

float
Color::Hue(float h, float m1, float m2)
{
	if (h < 0) h += 1;
	if (h > 1) h -= 1;
	if (h < 1.0f / 6.0f) return m1 + (m2 - m1) * h * 6.0f;
	else if (h < 3.0f / 6.0f) return m2;
	else if (h < 4.0f / 6.0f) return m1 + (m2 - m1) * (2.0f / 3.0f - h) * 6.0f;
	return m1;
}

Color
Color::Rand(uint32_t i)
{
	const float colors[8][4] = {
	    {0xb5 / 255.0f, 0x89 / 255.0f, 0x00 / 255.0f, 1.0f},
	    {0xcb / 255.0f, 0x4b / 255.0f, 0x16 / 255.0f, 1.0f},
	    {0xdc / 255.0f, 0x32 / 255.0f, 0x2f / 255.0f, 1.0f},
	    {0xd3 / 255.0f, 0x36 / 255.0f, 0x82 / 255.0f, 1.0f},
	    {0x6c / 255.0f, 0x71 / 255.0f, 0xc4 / 255.0f, 1.0f},
	    {0x26 / 255.0f, 0x8b / 255.0f, 0xd2 / 255.0f, 1.0f},
	    {0x2a / 255.0f, 0xa1 / 255.0f, 0x98 / 255.0f, 1.0f},
	    {0x85 / 255.0f, 0x99 / 255.0f, 0x00 / 255.0f, 1.0f},
	};
	i = i % 8;
	return {colors[i][0], colors[i][1], colors[i][2], colors[i][3]};
}

float
Color::SRGBtoLinear(unsigned char c)
{
	// float c = srgb / 255.0f;
	// return c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);  -- from
	// sRGB definition
	//  from stb_image_resize.h
	static float sRGBtoLinearLUT[256] = {
	    0.000000f, 0.000304f, 0.000607f, 0.000911f, 0.001214f, 0.001518f, 0.001821f, 0.002125f,
	    0.002428f, 0.002732f, 0.003035f, 0.003347f, 0.003677f, 0.004025f, 0.004391f, 0.004777f,
	    0.005182f, 0.005605f, 0.006049f, 0.006512f, 0.006995f, 0.007499f, 0.008023f, 0.008568f,
	    0.009134f, 0.009721f, 0.010330f, 0.010960f, 0.011612f, 0.012286f, 0.012983f, 0.013702f,
	    0.014444f, 0.015209f, 0.015996f, 0.016807f, 0.017642f, 0.018500f, 0.019382f, 0.020289f,
	    0.021219f, 0.022174f, 0.023153f, 0.024158f, 0.025187f, 0.026241f, 0.027321f, 0.028426f,
	    0.029557f, 0.030713f, 0.031896f, 0.033105f, 0.034340f, 0.035601f, 0.036889f, 0.038204f,
	    0.039546f, 0.040915f, 0.042311f, 0.043735f, 0.045186f, 0.046665f, 0.048172f, 0.049707f,
	    0.051269f, 0.052861f, 0.054480f, 0.056128f, 0.057805f, 0.059511f, 0.061246f, 0.063010f,
	    0.064803f, 0.066626f, 0.068478f, 0.070360f, 0.072272f, 0.074214f, 0.076185f, 0.078187f,
	    0.080220f, 0.082283f, 0.084376f, 0.086500f, 0.088656f, 0.090842f, 0.093059f, 0.095307f,
	    0.097587f, 0.099899f, 0.102242f, 0.104616f, 0.107023f, 0.109462f, 0.111932f, 0.114435f,
	    0.116971f, 0.119538f, 0.122139f, 0.124772f, 0.127438f, 0.130136f, 0.132868f, 0.135633f,
	    0.138432f, 0.141263f, 0.144128f, 0.147027f, 0.149960f, 0.152926f, 0.155926f, 0.158961f,
	    0.162029f, 0.165132f, 0.168269f, 0.171441f, 0.174647f, 0.177888f, 0.181164f, 0.184475f,
	    0.187821f, 0.191202f, 0.194618f, 0.198069f, 0.201556f, 0.205079f, 0.208637f, 0.212231f,
	    0.215861f, 0.219526f, 0.223228f, 0.226966f, 0.230740f, 0.234551f, 0.238398f, 0.242281f,
	    0.246201f, 0.250158f, 0.254152f, 0.258183f, 0.262251f, 0.266356f, 0.270498f, 0.274677f,
	    0.278894f, 0.283149f, 0.287441f, 0.291771f, 0.296138f, 0.300544f, 0.304987f, 0.309469f,
	    0.313989f, 0.318547f, 0.323143f, 0.327778f, 0.332452f, 0.337164f, 0.341914f, 0.346704f,
	    0.351533f, 0.356400f, 0.361307f, 0.366253f, 0.371238f, 0.376262f, 0.381326f, 0.386430f,
	    0.391573f, 0.396755f, 0.401978f, 0.407240f, 0.412543f, 0.417885f, 0.423268f, 0.428691f,
	    0.434154f, 0.439657f, 0.445201f, 0.450786f, 0.456411f, 0.462077f, 0.467784f, 0.473532f,
	    0.479320f, 0.485150f, 0.491021f, 0.496933f, 0.502887f, 0.508881f, 0.514918f, 0.520996f,
	    0.527115f, 0.533276f, 0.539480f, 0.545725f, 0.552011f, 0.558340f, 0.564712f, 0.571125f,
	    0.577581f, 0.584078f, 0.590619f, 0.597202f, 0.603827f, 0.610496f, 0.617207f, 0.623960f,
	    0.630757f, 0.637597f, 0.644480f, 0.651406f, 0.658375f, 0.665387f, 0.672443f, 0.679543f,
	    0.686685f, 0.693872f, 0.701102f, 0.708376f, 0.715694f, 0.723055f, 0.730461f, 0.737911f,
	    0.745404f, 0.752942f, 0.760525f, 0.768151f, 0.775822f, 0.783538f, 0.791298f, 0.799103f,
	    0.806952f, 0.814847f, 0.822786f, 0.830770f, 0.838799f, 0.846873f, 0.854993f, 0.863157f,
	    0.871367f, 0.879622f, 0.887923f, 0.896269f, 0.904661f, 0.913099f, 0.921582f, 0.930111f,
	    0.938686f, 0.947307f, 0.955974f, 0.964686f, 0.973445f, 0.982251f, 0.991102f, 1.0f};

	return sRGBtoLinearLUT[c];
}

Color
ie::RGBA(unsigned char r_, unsigned char g_, unsigned char b_, unsigned char a_)
{
	return {r_, g_, b_, a_};
}

Color
ie::RGB_uc(unsigned char r_, unsigned char g_, unsigned char b_)
{
	return {r_, g_, b_};
}

void
Color::ToHSL(float &h, float &s, float &l) const
{
	unsigned char max = std::max(std::max(r, g), b);
	unsigned char min = std::min(std::min(r, g), b);
	auto max_f = (float)max / 255.f;
	auto min_f = (float)min / 255.f;
	l = (max_f + min_f) / 2;
	if (max == min)
	{
		h = s = 0; // achromatic
	}
	else
	{
		float d = max_f - min_f;
		s = l > 0.5 ? d / (2 - max_f - min_f) : d / (max_f + min_f);
		if (max == r) h = (g / 255.f - b / 255.f) / d + (g < b ? 6.f : 0.f);
		else if (max == g) h = (b / 255.f - r / 255.f) / d + 2;
		else if (max == b) h = (r / 255.f - g / 255.f) / d + 4;
		h /= 6;
	}
}

Color
Color::Rainbow(float t)
{
	const float r = Sin(t);
	const float g = Sin(t + 0.33f * 2.0f * PI);
	const float b = Sin(t + 0.66f * 2.0f * PI);
	return {r * r, g * g, b * b};
}

float
Color::Luminance() const
{
	float l[3] = {0.212671f, 0.715160f, 0.072169f};
	return (r / 255.f) * l[0] + (g / 255.f) * l[1] + (b / 255.f) * l[2];
}

const Color &
Color::Brighten(int amount)
{
	assert(amount >= 0 && amount <= 100);

	r = Max(0, Min(255, r - (unsigned char)std::round(255 * -(amount / 100))));
	g = Max(0, Min(255, g - (unsigned char)std::round(255 * -(amount / 100))));
	b = Max(0, Min(255, b - (unsigned char)std::round(255 * -(amount / 100))));
	return *this;
}

const Color &
Color::Darken(int amount)
{
	assert(amount >= 0 && amount <= 100);
	r = (unsigned char)(((float)r * 255 / 100.f) * (float)amount / 255.f);
	g = (unsigned char)(((float)g * 255 / 100.f) * (float)amount / 255.f);
	b = (unsigned char)(((float)b * 255 / 100.f) * (float)amount / 255.f);
	return *this;
}

const Color &
Color::Lighten(int amount)
{
	assert(amount >= 0 && amount <= 100);

	int in = (int)(255.f * ((float)amount / 100.f));
	r = Min(255, r + in);
	g = Min(255, g + in);
	b = Min(255, b + in);
	return *this;
}

const Color &
Color::Spin(int amount)
{
	assert(amount >= -360 && amount <= 360);

	float h, s, l;
	ToHSL(h, s, l);
	auto hue = (int)(h * 360 + amount) % 360;
	hue = hue < 0 ? 360 + hue : hue;
	*this = HSLA((float)hue / 360.f, s, l, (float)a / 255.f);
	return *this;
}

Color
ie::HSLA(float h, float s, float l, float a)
{
	float m1, m2, r, g, b;
	h = Mod(h, 1.0f);
	if (h < 0.0f) h += 1.0f;
	s = Clamp(s, 0.0f, 1.0f);
	l = Clamp(l, 0.0f, 1.0f);
	m2 = l <= 0.5f ? (l * (1 + s)) : (l + s - l * s);
	m1 = 2 * l - m2;
	r = Clamp(Color::Hue(h + 1.0f / 3.0f, m1, m2), 0.0f, 1.0f);
	g = Clamp(Color::Hue(h, m1, m2), 0.0f, 1.0f);
	b = Clamp(Color::Hue(h - 1.0f / 3.0f, m1, m2), 0.0f, 1.0f);
	return {r, g, b, a};
}

Color
ie::HSL(float h, float s, float l)
{
	return HSLA(h, s, l, 1.f);
}

Color &
Color::Trans(float a_)
{
	a = (unsigned char)Round(a_ * 255.f);
	return *this;
}

Color &
Color::Trans(unsigned char a_)
{
	a = a_;
	return *this;
}

Color
Color::Lerp(Color c0, Color c1, float u)
{
	Color cint{0.f, 0.f, 0.f, 0.f};

	float v;
	u = Clamp(u, 0.0f, 1.0f);
	float oneminu = 1.0f - u;

	// r
	v = (float)c0.r * oneminu + (float)c1.r * u;
	cint.r = (unsigned char)Round(v);
	// g
	v = (float)c0.g * oneminu + (float)c1.g * u;
	cint.g = (unsigned char)Round(v);
	// b
	v = (float)c0.b * oneminu + (float)c1.b * u;
	cint.b = (unsigned char)Round(v);
	// a
	v = (float)c0.a * oneminu + (float)c1.a * u;
	cint.a = (unsigned char)Round(v);

	return cint;
}

Color::Color(const char *name)
{
	const ColorIndex *index = LookUpColorIndex(name, (unsigned int)strlen(name));
	if (index == nullptr)
	{
		r = 0;
		g = 0;
		b = 0;
		a = 255;
	}
	else
	{
		Color c{index->r, index->g, index->b};
		r = c.r;
		g = c.g;
		b = c.b;
		a = c.a;
	}
}

Color::Color(float r_, float g_, float b_, float a_)
{
	r = (unsigned char)Round(r_ * 255.0f);
	g = (unsigned char)Round(g_ * 255.0f);
	b = (unsigned char)Round(b_ * 255.0f);
	a = (unsigned char)Round(a_ * 255.0f);
}

Color::Color(Colors color) { operator=(color); }

Color::Color(float r_, float g_, float b_)
{
	r = (unsigned char)Round(r_ * 255.0f);
	g = (unsigned char)Round(g_ * 255.0f);
	b = (unsigned char)Round(b_ * 255.0f);
	a = 255;
}

static const struct ColorIndex colorIndexKeywords_[] = {
    {"cyan", 0, 255, 255},
    {"gray", 128, 128, 128},
    {"chartreuse", 127, 255, 0},
    {"grey", 128, 128, 128},
    {"green", 0, 128, 0},
    {"lightgrey", 211, 211, 211},
    {"lightgreen", 144, 238, 144},
    {"lightgray", 211, 211, 211},
    {"skyblue", 135, 206, 235},
    {"slategrey", 112, 128, 144},
    {"sienna", 160, 82, 45},
    {"slategray", 112, 128, 144},
    {"seashell", 255, 245, 238},
    {"teal", 0, 128, 128},
    {"coral", 255, 127, 80},
    {"lightsalmon", 255, 160, 122},
    {"lightslategrey", 119, 136, 153},
    {"black", 0, 0, 0},
    {"lightslategray", 119, 136, 153},
    {"orange", 255, 165, 0},
    {"orangered", 255, 69, 0},
    {"bisque", 255, 228, 196},
    {"lime", 0, 255, 0},
    {"red", 255, 0, 0},
    {"limegreen", 50, 205, 50},
    {"lightcoral", 240, 128, 128},
    {"royalblue", 65, 105, 225},
    {"linen", 250, 240, 230},
    {"fuchsia", 255, 0, 255},
    {"darkgreen", 0, 100, 0},
    {"lightblue", 173, 216, 230},
    {"darkorchid", 153, 50, 204},
    {"springgreen", 0, 255, 127},
    {"magenta", 255, 0, 255},
    {"gold", 255, 215, 0},
    {"orchid", 218, 112, 214},
    {"slateblue", 106, 90, 205},
    {"darkmagenta", 139, 0, 139},
    {"darkblue", 0, 0, 139},
    {"lightsteelblue", 176, 196, 222},
    {"silver", 192, 192, 192},
    {"seagreen", 46, 139, 87},
    {"steelblue", 70, 130, 180},
    {"tan", 210, 180, 140},
    {"peru", 205, 133, 63},
    {"purple", 128, 0, 128},
    {"darkred", 139, 0, 0},
    {"mintcream", 245, 255, 250},
    {"firebrick", 178, 34, 34},
    {"lightseagreen", 32, 178, 170},
    {"darkolivegreen", 85, 107, 47},
    {"mistyrose", 255, 228, 225},
    {"indigo", 75, 0, 130},
    {"oldlace", 253, 245, 230},
    {"pink", 255, 192, 203},
    {"darksalmon", 233, 150, 122},
    {"lavender", 230, 230, 250},
    {"ivory", 255, 255, 240},
    {"moccasin", 255, 228, 181},
    {"cadetblue", 95, 158, 160},
    {"darkviolet", 148, 0, 211},
    {"saddlebrown", 139, 69, 19},
    {"darkslateblue", 72, 61, 139},
    {"palegreen", 152, 251, 152},
    {"snow", 255, 250, 250},
    {"indianred", 205, 92, 92},
    {"lightgoldenrodyellow", 250, 250, 210},
    {"tomato", 255, 99, 71},
    {"lemonchiffon", 255, 250, 205},
    {"lightpink", 255, 182, 193},
    {"maroon", 128, 0, 0},
    {"lavenderblush", 255, 240, 245},
    {"turquoise", 64, 224, 208},
    {"darkorange", 255, 140, 0},
    {"navy", 0, 0, 128},
    {"dodgerblue", 30, 144, 255},
    {"forestgreen", 34, 139, 34},
    {"midnightblue", 25, 25, 112},
    {"mediumseagreen", 60, 179, 113},
    {"darkseagreen", 143, 188, 143},
    {"aqua", 0, 255, 255},
    {"azure", 240, 255, 255},
    {"salmon", 250, 128, 114},
    {"wheat", 245, 222, 179},
    {"brown", 165, 42, 42},
    {"aquamarine", 127, 255, 212},
    {"chocolate", 210, 105, 30},
    {"lawngreen", 124, 252, 0},
    {"sandybrown", 244, 164, 96},
    {"lightcyan", 224, 255, 255},
    {"violet", 238, 130, 238},
    {"lightyellow", 255, 255, 224},
    {"mediumblue", 0, 0, 205},
    {"peachpuff", 255, 218, 185},
    {"greenyellow", 173, 255, 47},
    {"antiquewhite", 250, 235, 215},
    {"blue", 0, 0, 255},
    {"mediumvioletred", 199, 21, 133},
    {"mediumpurple", 147, 112, 219},
    {"goldenrod", 218, 165, 32},
    {"blanchedalmond", 255, 235, 205},
    {"khaki", 240, 230, 140},
    {"plum", 221, 160, 221},
    {"mediumorchid", 186, 85, 211},
    {"rosybrown", 188, 143, 143},
    {"mediumslateblue", 123, 104, 238},
    {"darkturquoise", 0, 206, 209},
    {"palevioletred", 219, 112, 147},
    {"papayawhip", 255, 239, 213},
    {"mediumspringgreen", 0, 250, 154},
    {"darkgrey", 169, 169, 169},
    {"mediumturquoise", 72, 209, 204},
    {"darkgray", 169, 169, 169},
    {"darkgoldenrod", 184, 134, 11},
    {"dimgrey", 105, 105, 105},
    {"dimgray", 105, 105, 105},
    {"honeydew", 240, 255, 240},
    {"beige", 245, 245, 220},
    {"thistle", 216, 191, 216},
    {"cornsilk", 255, 248, 220},
    {"olive", 128, 128, 0},
    {"blueviolet", 138, 43, 226},
    {"mediumaquamarine", 102, 205, 170},
    {"cornflowerblue", 100, 149, 237},
    {"aliceblue", 240, 248, 255},
    {"powderblue", 176, 224, 230},
    {"paleturquoise", 175, 238, 238},
    {"darkslategrey", 47, 79, 79},
    {"darkkhaki", 189, 183, 107},
    {"darkslategray", 47, 79, 79},
    {"ghostwhite", 248, 248, 255},
    {"olivedrab", 107, 142, 35},
    {"palegoldenrod", 238, 232, 170},
    {"darkcyan", 0, 139, 139},
    {"hotpink", 255, 105, 180},
    {"gainsboro", 220, 220, 220},
    {"deeppink", 255, 20, 147},
    {"crimson", 220, 20, 60},
    {"burlywood", 222, 184, 135},
    {"floralwhite", 255, 250, 240},
    {"white", 255, 255, 255},
    {"navajowhite", 255, 222, 173},
    {"yellow", 255, 255, 0},
    {"yellowgreen", 154, 205, 50},
    {"lightskyblue", 135, 206, 250},
    {"deepskyblue", 0, 191, 255},
    {"whitesmoke", 245, 245, 245}};

const struct ColorIndex *NVGcolorindex_keywords = colorIndexKeywords_;

#define TOTAL_KEYWORDS  147
#define MIN_WORD_LENGTH 3
#define MAX_WORD_LENGTH 20
#define MIN_HASH_VALUE  4
#define MAX_HASH_VALUE  565
// maximum key range = 562, duplicates = 0

static inline unsigned int
colorIndexHash(const char *str, unsigned int len)
{
	static const unsigned short asso_values[] = {
	    566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566,
	    566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566,
	    566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566,
	    566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 5,   55,  0,
	    35,  0,   75,  10,  5,   0,   566, 250, 10,  40,  85,  60,  70,  144, 0,   20,  45,
	    10,  30,  185, 95,  195, 566, 0,   566, 566, 566, 566, 566, 5,   55,  0,   35,  0,
	    75,  10,  5,   0,   566, 250, 10,  40,  85,  60,  70,  144, 0,   20,  45,  10,  30,
	    185, 95,  195, 566, 0,   566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566,
	    566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566,
	    566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566,
	    566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566,
	    566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566,
	    566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566,
	    566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566,
	    566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566, 566,
	    566, 566, 566};
	unsigned int hval = len;

	switch (hval)
	{
		default:
			hval += asso_values[(unsigned char)str[12]];
			/*FALLTHROUGH*/
		case 12:
			hval += asso_values[(unsigned char)str[11]];
			/*FALLTHROUGH*/
		case 11:
		case 10:
		case 9:
		case 8:
			hval += asso_values[(unsigned char)str[7]];
			/*FALLTHROUGH*/
		case 7:
			hval += asso_values[(unsigned char)str[6]];
			/*FALLTHROUGH*/
		case 6:
			hval += asso_values[(unsigned char)str[5]];
			/*FALLTHROUGH*/
		case 5:
		case 4:
		case 3:
			hval += asso_values[(unsigned char)str[2] + 2];
			/*FALLTHROUGH*/
		case 2:
		case 1: hval += asso_values[(unsigned char)str[0]]; break;
	}
	return hval;
}

const struct ColorIndex *
Color::LookUpColorIndex(const char *str, unsigned int len)
{
	if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
	{
		unsigned int key = colorIndexHash(str, len);

		if (key <= MAX_HASH_VALUE && key >= MIN_HASH_VALUE)
		{
			const struct ColorIndex *resword;

			switch (key - 4)
			{
				case 0: resword = &NVGcolorindex_keywords[0]; goto compare;
				case 10: resword = &NVGcolorindex_keywords[1]; goto compare;
				case 16: resword = &NVGcolorindex_keywords[2]; goto compare;
				case 20: resword = &NVGcolorindex_keywords[3]; goto compare;
				case 21: resword = &NVGcolorindex_keywords[4]; goto compare;
				case 25: resword = &NVGcolorindex_keywords[5]; goto compare;
				case 26: resword = &NVGcolorindex_keywords[6]; goto compare;
				case 30: resword = &NVGcolorindex_keywords[7]; goto compare;
				case 33: resword = &NVGcolorindex_keywords[8]; goto compare;
				case 35: resword = &NVGcolorindex_keywords[9]; goto compare;
				case 37: resword = &NVGcolorindex_keywords[10]; goto compare;
				case 40: resword = &NVGcolorindex_keywords[11]; goto compare;
				case 44: resword = &NVGcolorindex_keywords[12]; goto compare;
				case 45: resword = &NVGcolorindex_keywords[13]; goto compare;
				case 46: resword = &NVGcolorindex_keywords[14]; goto compare;
				case 52: resword = &NVGcolorindex_keywords[15]; goto compare;
				case 55: resword = &NVGcolorindex_keywords[16]; goto compare;
				case 56: resword = &NVGcolorindex_keywords[17]; goto compare;
				case 60: resword = &NVGcolorindex_keywords[18]; goto compare;
				case 62: resword = &NVGcolorindex_keywords[19]; goto compare;
				case 65: resword = &NVGcolorindex_keywords[20]; goto compare;
				case 67: resword = &NVGcolorindex_keywords[21]; goto compare;
				case 70: resword = &NVGcolorindex_keywords[22]; goto compare;
				case 74: resword = &NVGcolorindex_keywords[23]; goto compare;
				case 75: resword = &NVGcolorindex_keywords[24]; goto compare;
				case 76: resword = &NVGcolorindex_keywords[25]; goto compare;
				case 80: resword = &NVGcolorindex_keywords[26]; goto compare;
				case 81: resword = &NVGcolorindex_keywords[27]; goto compare;
				case 83: resword = &NVGcolorindex_keywords[28]; goto compare;
				case 85: resword = &NVGcolorindex_keywords[29]; goto compare;
				case 90: resword = &NVGcolorindex_keywords[30]; goto compare;
				case 91: resword = &NVGcolorindex_keywords[31]; goto compare;
				case 92: resword = &NVGcolorindex_keywords[32]; goto compare;
				case 93: resword = &NVGcolorindex_keywords[33]; goto compare;
				case 95: resword = &NVGcolorindex_keywords[34]; goto compare;
				case 97: resword = &NVGcolorindex_keywords[35]; goto compare;
				case 100: resword = &NVGcolorindex_keywords[36]; goto compare;
				case 102: resword = &NVGcolorindex_keywords[37]; goto compare;
				case 104: resword = &NVGcolorindex_keywords[38]; goto compare;
				case 105: resword = &NVGcolorindex_keywords[39]; goto compare;
				case 107: resword = &NVGcolorindex_keywords[40]; goto compare;
				case 109: resword = &NVGcolorindex_keywords[41]; goto compare;
				case 110: resword = &NVGcolorindex_keywords[42]; goto compare;
				case 114: resword = &NVGcolorindex_keywords[43]; goto compare;
				case 115: resword = &NVGcolorindex_keywords[44]; goto compare;
				case 117: resword = &NVGcolorindex_keywords[45]; goto compare;
				case 118: resword = &NVGcolorindex_keywords[46]; goto compare;
				case 120: resword = &NVGcolorindex_keywords[47]; goto compare;
				case 125: resword = &NVGcolorindex_keywords[48]; goto compare;
				case 129: resword = &NVGcolorindex_keywords[49]; goto compare;
				case 130: resword = &NVGcolorindex_keywords[50]; goto compare;
				case 135: resword = &NVGcolorindex_keywords[51]; goto compare;
				case 137: resword = &NVGcolorindex_keywords[52]; goto compare;
				case 138: resword = &NVGcolorindex_keywords[53]; goto compare;
				case 140: resword = &NVGcolorindex_keywords[54]; goto compare;
				case 141: resword = &NVGcolorindex_keywords[55]; goto compare;
				case 144: resword = &NVGcolorindex_keywords[56]; goto compare;
				case 145: resword = &NVGcolorindex_keywords[57]; goto compare;
				case 149: resword = &NVGcolorindex_keywords[58]; goto compare;
				case 155: resword = &NVGcolorindex_keywords[59]; goto compare;
				case 156: resword = &NVGcolorindex_keywords[60]; goto compare;
				case 157: resword = &NVGcolorindex_keywords[61]; goto compare;
				case 159: resword = &NVGcolorindex_keywords[62]; goto compare;
				case 160: resword = &NVGcolorindex_keywords[63]; goto compare;
				case 164: resword = &NVGcolorindex_keywords[64]; goto compare;
				case 165: resword = &NVGcolorindex_keywords[65]; goto compare;
				case 166: resword = &NVGcolorindex_keywords[66]; goto compare;
				case 167: resword = &NVGcolorindex_keywords[67]; goto compare;
				case 168: resword = &NVGcolorindex_keywords[68]; goto compare;
				case 170: resword = &NVGcolorindex_keywords[69]; goto compare;
				case 172: resword = &NVGcolorindex_keywords[70]; goto compare;
				case 174: resword = &NVGcolorindex_keywords[71]; goto compare;
				case 175: resword = &NVGcolorindex_keywords[72]; goto compare;
				case 176: resword = &NVGcolorindex_keywords[73]; goto compare;
				case 180: resword = &NVGcolorindex_keywords[74]; goto compare;
				case 181: resword = &NVGcolorindex_keywords[75]; goto compare;
				case 182: resword = &NVGcolorindex_keywords[76]; goto compare;
				case 183: resword = &NVGcolorindex_keywords[77]; goto compare;
				case 185: resword = &NVGcolorindex_keywords[78]; goto compare;
				case 188: resword = &NVGcolorindex_keywords[79]; goto compare;
				case 190: resword = &NVGcolorindex_keywords[80]; goto compare;
				case 191: resword = &NVGcolorindex_keywords[81]; goto compare;
				case 192: resword = &NVGcolorindex_keywords[82]; goto compare;
				case 196: resword = &NVGcolorindex_keywords[83]; goto compare;
				case 200: resword = &NVGcolorindex_keywords[84]; goto compare;
				case 201: resword = &NVGcolorindex_keywords[85]; goto compare;
				case 209: resword = &NVGcolorindex_keywords[86]; goto compare;
				case 210: resword = &NVGcolorindex_keywords[87]; goto compare;
				case 211: resword = &NVGcolorindex_keywords[88]; goto compare;
				case 215: resword = &NVGcolorindex_keywords[89]; goto compare;
				case 221: resword = &NVGcolorindex_keywords[90]; goto compare;
				case 222: resword = &NVGcolorindex_keywords[91]; goto compare;
				case 226: resword = &NVGcolorindex_keywords[92]; goto compare;
				case 230: resword = &NVGcolorindex_keywords[93]; goto compare;
				case 232: resword = &NVGcolorindex_keywords[94]; goto compare;
				case 238: resword = &NVGcolorindex_keywords[95]; goto compare;
				case 240: resword = &NVGcolorindex_keywords[96]; goto compare;
				case 241: resword = &NVGcolorindex_keywords[97]; goto compare;
				case 243: resword = &NVGcolorindex_keywords[98]; goto compare;
				case 245: resword = &NVGcolorindex_keywords[99]; goto compare;
				case 250: resword = &NVGcolorindex_keywords[100]; goto compare;
				case 251: resword = &NVGcolorindex_keywords[101]; goto compare;
				case 255: resword = &NVGcolorindex_keywords[102]; goto compare;
				case 258: resword = &NVGcolorindex_keywords[103]; goto compare;
				case 260: resword = &NVGcolorindex_keywords[104]; goto compare;
				case 261: resword = &NVGcolorindex_keywords[105]; goto compare;
				case 263: resword = &NVGcolorindex_keywords[106]; goto compare;
				case 269: resword = &NVGcolorindex_keywords[107]; goto compare;
				case 271: resword = &NVGcolorindex_keywords[108]; goto compare;
				case 278: resword = &NVGcolorindex_keywords[109]; goto compare;
				case 279: resword = &NVGcolorindex_keywords[110]; goto compare;
				case 281: resword = &NVGcolorindex_keywords[111]; goto compare;
				case 284: resword = &NVGcolorindex_keywords[112]; goto compare;
				case 289: resword = &NVGcolorindex_keywords[113]; goto compare;
				case 293: resword = &NVGcolorindex_keywords[114]; goto compare;
				case 298: resword = &NVGcolorindex_keywords[115]; goto compare;
				case 299: resword = &NVGcolorindex_keywords[116]; goto compare;
				case 306: resword = &NVGcolorindex_keywords[117]; goto compare;
				case 308: resword = &NVGcolorindex_keywords[118]; goto compare;
				case 309: resword = &NVGcolorindex_keywords[119]; goto compare;
				case 311: resword = &NVGcolorindex_keywords[120]; goto compare;
				case 316: resword = &NVGcolorindex_keywords[121]; goto compare;
				case 321: resword = &NVGcolorindex_keywords[122]; goto compare;
				case 330: resword = &NVGcolorindex_keywords[123]; goto compare;
				case 335: resword = &NVGcolorindex_keywords[124]; goto compare;
				case 336: resword = &NVGcolorindex_keywords[125]; goto compare;
				case 338: resword = &NVGcolorindex_keywords[126]; goto compare;
				case 344: resword = &NVGcolorindex_keywords[127]; goto compare;
				case 345: resword = &NVGcolorindex_keywords[128]; goto compare;
				case 349: resword = &NVGcolorindex_keywords[129]; goto compare;
				case 350: resword = &NVGcolorindex_keywords[130]; goto compare;
				case 355: resword = &NVGcolorindex_keywords[131]; goto compare;
				case 364: resword = &NVGcolorindex_keywords[132]; goto compare;
				case 369: resword = &NVGcolorindex_keywords[133]; goto compare;
				case 373: resword = &NVGcolorindex_keywords[134]; goto compare;
				case 380: resword = &NVGcolorindex_keywords[135]; goto compare;
				case 384: resword = &NVGcolorindex_keywords[136]; goto compare;
				case 398: resword = &NVGcolorindex_keywords[137]; goto compare;
				case 410: resword = &NVGcolorindex_keywords[138]; goto compare;
				case 426: resword = &NVGcolorindex_keywords[139]; goto compare;
				case 436: resword = &NVGcolorindex_keywords[140]; goto compare;
				case 437: resword = &NVGcolorindex_keywords[141]; goto compare;
				case 467: resword = &NVGcolorindex_keywords[142]; goto compare;
				case 482: resword = &NVGcolorindex_keywords[143]; goto compare;
				case 483: resword = &NVGcolorindex_keywords[144]; goto compare;
				case 552: resword = &NVGcolorindex_keywords[145]; goto compare;
				case 561: resword = &NVGcolorindex_keywords[146]; goto compare;
			}
			return nullptr;
compare : {
	const char *s = resword->name.c_str();

	if ((((unsigned char)*str ^ (unsigned char)*s) & ~32) == 0 && !strncmp(str, s, len) &&
	    s[len] == '\0')
		return resword;
}
		}
	}
	return nullptr;
}
