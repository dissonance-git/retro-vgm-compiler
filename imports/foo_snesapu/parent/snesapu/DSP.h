/***************************************************************************************************
* Program:    SNES Digital Signal Processor (DSP) Emulator                                         *
* Platform:   Intel 80386                                                                          *
* Programmer: Anti Resonance (Alpha-II Productions), sunburst (degrade-factory)                    *
*                                                                                                  *
* "SNES" and "Super Nintendo Entertainment System" are trademarks of Nintendo Co., Limited and its *
* subsidiary companies.                                                                            *
*                                                                                                  *
* This program is free software; you can redistribute it and/or modify it under the terms of the   *
* GNU General Public License as published by the Free Software Foundation; either version 2 of     *
* the License, or (at your option) any later version.                                              *
*                                                                                                  *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;        *
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.        *
* See the GNU General Public License for more details.                                             *
*                                                                                                  *
* You should have received a copy of the GNU General Public License along with this program;       *
* if not, write to the Free Software Foundation, Inc.                                              *
* 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.                                        *
*                                                                                                  *
*                                                 Copyright (C) 1999-2006 Alpha-II Productions     *
*                                                 Copyright (C) 2003-2010 degrade-factory          *
***************************************************************************************************/

#ifndef	__INC_DSP
#define	__INC_DSP

#define	INT_NONE	0
#define	INT_LINEAR	1
#define	INT_CUBIC	2
#define	INT_GAUSS	3
#define	INT_SINC	4
#define	INT_GAUSS4	7

#define	DSP_ANALOG	0x01
#define	DSP_OLDSMP	0x02
#define	DSP_SURND	0x04
#define	DSP_REVERSE	0x08
#define	DSP_NOECHO	0x10
#define	DSP_NOPMOD	0x20
#define	DSP_NOPREAD	0x40
#define	DSP_NOFIR	0x80
#define	DSP_BASS	0x100
#define	DSP_NOENV	0x200
#define	DSP_NONOISE	0x400
#define	DSP_ECHOMEM	0x800
#define	DSP_NOSURND	0x1000
#define	DSP_FLOAT	0x40000000
#define	DSP_NOSAFE	0x80000000

#define	BRR_LINEAR	0x01
#define	BRR_LOOP	0x02
#define	BRR_NOINIT	0x04
#define	BRR_8BIT	0x10

#define	MFLG_MUTE	0x01
#define	MFLG_NOISE	0x02
#define	MFLG_USER	0x03
#define	MFLG_KOFF	0x04
#define	MFLG_OFF	0x08
#define	MFLG_END	0x10

#define	S700_MUTE	0x01
#define	S700_CHANGE	0x02
#define	S700_DETUNE	0x04
#define	S700_VOLUME	0x08
#define	S700_MVOL_L	0x00
#define	S700_MVOL_R	0x01
#define	S700_ECHO_L	0x02
#define	S700_ECHO_R	0x03
#define	EXT_CBEVENT	0x10010
#define	CBE_DSPREG	0x00

typedef struct DSPVoice
{
	s8	volL;
	s8	volR;
	u16	pitch;
	u8	srcn;
	u8	adsr[2];
	u8	gain;
	s8	envx;
	s8	outx;
	s8	__r[6];
} DSPVoice;

typedef struct DSPFIR
{
	s8	__r[15];
	s8	c;
} DSPFIR;

typedef union DSPReg
{
	DSPVoice	voice[8];
	struct
	{
		s8	__r00[12]; s8	mvolL; s8	efb; s8	__r0E; s8	c0;
		s8	__r10[12]; s8	mvolR; s8	__r1D; s8	__r1E; s8	c1;
		s8	__r20[12]; s8	evolL; u8	pmon; s8	__r2E; s8	c2;
		s8	__r30[12]; s8	evolR; u8	non; s8	__r3E; s8	c3;
		s8	__r40[12]; u8	kon; u8	eon; s8	__r4E; s8	c4;
		s8	__r50[12]; u8	kof; u8	dir; s8	__r5E; s8	c5;
		s8	__r60[12]; u8	flg; u8	esa; s8	__r6E; s8	c6;
		s8	__r70[12]; u8	endx; u8	edl; s8	__r7E; s8	c7;
	};
	DSPFIR	fir[8];
	u8		reg[128];
} DSPReg;

typedef struct MixF
{
	b8	mute:1;
	u8	noise:1;
	b8	keyOff:1;
	b8	inactive:1;
	b8	keyEnd:1;
	u8	__r2:3;
} MixF;

typedef enum EnvM
{
	ENV_DEC,
	ENV_EXP,
	ENV_INC,
	ENV_BENT = 6,
	ENV_DIR,
	ENV_REL,
	ENV_SUST,
	ENV_ATTACK,
	ENV_DECAY = 13,
} EnvM;

#define	ENVM_IDLE	0x80
#define	ENVM_MODE	0xF

typedef struct Voice
{
	u16		vAdsr;
	u8		vGain;
	u8		vRsv;
	s16		*sIdx;
	void	*bCur;
	u8		bHdr;
	u8		mFlg;
	u8		eMode;
	u8		eRIdx;
	u32		eRate;
	u32		eCnt;
	u32		eVal;
	s32		eAdj;
	u32		eDest;
	s32		vMaxL;
	s32		vMaxR;
	s16		sP1;
	s16		sP2;
	s16		sBufP[8];
	s16		sBuf[16];
	f32		mTgtL;
	f32		mTgtR;
	s32		mChnL;
	s32		mChnR;
	u32		mRate;
	u16		mDec;
	u8		mSrc;
	u8		mKOn;
	u32		mOrgP;
	s32		mOut;
} Voice;

typedef void (__cdecl *DSPDebug)(volatile u8 *reg, volatile u8 val);

#ifdef	__GNUC__
#define	_CallDSPDebug(proc, reg, val) \
			asm(" \
				pushl	%2 \
				pushl	%1 \
				calll	%0 \
				popl	%1 \
				popl	%2 \
			" : : "m" (proc), "m" (reg), "m" (val));
#else
#define	_CallDSPDebug(proc, reg, val) \
			_asm \
			{ \
				push	dword ptr [val]; \
				push	dword ptr [reg]; \
				call	dword ptr [proc]; \
				pop		dword ptr [reg]; \
				pop		dword ptr [val]; \
			}
#endif

#ifndef	SNESAPU_DLL
#ifdef	__cplusplus
extern	"C"	DSPReg	dsp;
extern	"C"	Voice	mix[8];
extern	"C"	u32		vMMaxL,vMMaxR;
#else
extern	DSPReg	dsp;
extern	Voice	mix[8];
extern	u32		vMMaxL,vMMaxR;
#endif

#ifdef	__cplusplus
extern	"C" {
#endif
void InitDSP();
void ResetDSP();
void SetDSPOpt(u32 mix, u32 chn, u32 bits, u32 rate, u32 inter, u32 opts);
DSPDebug SetDSPDbg(DSPDebug pTrace);
void FixDSP();
void FixSeek(u8 reset);
void SetDSPPitch(u32 base);
void SetDSPAmp(u32 amp);
void SetDSPVol(u32 vol);
void SetDSPStereo(u32 sep);
void SetDSPEFBCT(s32 leak);
b8 SetDSPReg(u8 reg, u8 val);
void* EmuDSP(void *pBuf, s32 size);
#ifdef	__cplusplus
}
#endif

#endif	//SNESAPU_DLL
#endif	//__INC_DSP
