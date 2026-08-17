#if !defined(_ID666_HPP__)
#define _ID666_HPP__

/**
 * foobar2000 wrapper for SNESAPU.dll
 *
 *     Licence :
 *        GNU General Public License
 *
 */

static const size_t ID6_TEXT_MAX      = 32;
static const size_t ID6_DATE_MAX      = 11;
static const size_t ID6_DUMPER_MAX    = 16;
static const size_t ID6_FADE_MAX_MS   = 59999;	// 59.99[sec]
static const size_t ID6_LENGTH_MAX_MS = 959000;	// 16[min]

static const size_t ID6_OFS_TITLE    = 0x2E;
static const size_t ID6_OFS_GAME     = 0x4E;
static const size_t ID6_OFS_DUMPER   = 0x6E;
static const size_t ID6_OFS_COMMENT  = 0x7E;
static const size_t ID6_OFS_DATE          = 0x9E;
static const size_t ID6_OFS_PLAY          = 0xA9;
static const size_t ID6_OFS_FADE          = 0xAC;
static const size_t ID6_OFS_TEXT_COMPOSER = 0xB1;
static const size_t ID6_OFS_BIN_COMPOSER  = 0xB0;
static const size_t ID6_OFS_MUTE          = 0xD1;
static const size_t ID6_OFS_TEXT_EMU      = 0xD2;
static const size_t ID6_OFS_BIN_EMU       = 0xD1;

#endif
