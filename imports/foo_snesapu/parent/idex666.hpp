#if !defined(_IDEX666_HPP__)
#define _IDEX666_HPP__

static const size_t OFS_XID6 = 0x10200;
static const size_t XID6_HEADER_LENGTH = 8;
static const size_t XID6_LENGTH_MAX_MS = 5999999;	//383999999 ticks. A tick is 1 / 64000th of a second.
static const size_t XID6_EMULIST = 9;
static const s8 emuList[XID6_EMULIST][12] =
{"unknown","ZSNES","Snes9x","ZST2SPC","ETC","SNEShout","ZSNES/W","Snes9xpp","SNESGT"};

/// Extended ID666 Sub-chunk ID's
enum XID6_ID {
	XID6_SONG	= 0x01,
	XID6_GAME	= 0x02,
	XID6_ARTIST = 0x03,
	XID6_DUMPER = 0x04,
	XID6_DATE	= 0x05,
	XID6_EMU	= 0x06,
	XID6_CMNTS	= 0x07,
	XID6_INTRO	= 0x30,
	XID6_LOOP	= 0x31,
	XID6_END	= 0x32,
	XID6_FADE	= 0x33,
	XID6_MUTE	= 0x34,
	XID6_LOOPX	= 0x35,
	XID6_AMP	= 0x36,
	XID6_OST	= 0x10,
	XID6_DISC	= 0x11,
	XID6_TRACK	= 0x12,
	XID6_PUB	= 0x13,
	XID6_COPY	= 0x14
};

/// Extended ID666 Data types
enum XID6_TYPE {
	XID6_TVAL = 0x00,
	XID6_TSTR = 0x01,
	XID6_TINT = 0x04
};

#define XID6_MAGIC "xid6"

#pragma pack(push, 1)
struct XID6Chk {
	u8	id;							// Subchunk ID
	u8	type;						// Type of data
	u16	val;						// Data or Data length
	u32 data;						// Data
};
#pragma pack(pop)

#endif
