#include "stdafx.h"

#include "input_snesapu.hpp"
#include "snesapu/Types.h"
#include "snesapu/SNESAPU.h"
#include "resource.h"
#if FOOBAR2000_SDK_VERSION >= 20221020
#include <SDK/coreDarkMode.h>
#endif

#pragma warning(disable : 4996)

static const GUID guid_samplerate =
{ 0xdbe7c79e, 0x25aa, 0x47e8, { 0x95, 0x61, 0x77, 0x9d, 0x26, 0x29, 0x2f, 0x13 } };
static const GUID guid_bitspersample =
{ 0x5db802fb, 0xa0eb, 0x4b54, { 0x90, 0xfe, 0xb5, 0x79, 0x55, 0x4, 0x2b, 0xed } };
static const GUID guid_channels =
{ 0x6c5eb8d0, 0x277c, 0x4cf5, { 0xac, 0xd1, 0xf1, 0x43, 0xd2, 0x39, 0x53, 0x43 } };
static const GUID guid_interpolation =
{ 0x95adc33a, 0xb84e, 0x4b07, { 0xb7, 0x67, 0xd, 0x30, 0xa0, 0xe6, 0x3d, 0x43 } };
static const GUID guid_options =
{ 0xb1880ea5, 0x3dea, 0x485a, { 0xa4, 0x2d, 0x36, 0xa4, 0x83, 0xf6, 0x88, 0xa5 } };
static const GUID guid_fastseek =
{ 0x16f8f107, 0x2201, 0x4415, { 0x81, 0x12, 0x29, 0x5a, 0xea, 0xe2, 0x7d, 0xe8 } };
static const GUID guid_usespclength =
{ 0x4a85c063, 0xf697, 0x4afc, { 0x99, 0x90, 0xb4, 0xf1, 0x6, 0xa3, 0xd5, 0x48 } };
static const GUID guid_forcelength =
{ 0x7f03e6dd, 0xdf8e, 0x49b7, { 0xa9, 0x2c, 0x7c, 0x94, 0x4e, 0x9c, 0xe9, 0x60 } };
static const GUID guid_songlength =
{ 0x51b67b9b, 0xc10c, 0x480c, { 0x95, 0xd, 0x89, 0x9e, 0x7c, 0xe4, 0x38, 0x2e } };
static const GUID guid_fadelength =
{ 0x4c6b7f2e, 0x8b30, 0x4bce, { 0x91, 0x2a, 0xf5, 0x17, 0x46, 0xc4, 0x3e, 0x7f } };
static const GUID guid_amplification =
{ 0x21bfb1c2, 0xac00, 0x472a, { 0xa0, 0xec, 0xb9, 0x7c, 0x77, 0x14, 0x75, 0xd9 } };
static const GUID guid_ignore_script700 =
{ 0xc762e25f, 0x502, 0x48f5, { 0xbf, 0xf3, 0xa7, 0x2b, 0x7, 0xf, 0x4d, 0xcb } };
static const GUID guid_ignore_xid6_amplification =
{ 0xa140f72e, 0x54e7, 0x4b03, { 0xa6, 0xe8, 0xf3, 0xc8, 0x57, 0x59, 0x89, 0x5e } };
#ifdef _WIN64
static const GUID guid_sem71_enabled =
{ 0xb7c3a912, 0x6f11, 0x4d8e, { 0x8a, 0x15, 0xc2, 0x3d, 0x67, 0xf9, 0x01, 0xae } };
#endif

const GUID guid_snesapu_preference_page =
{ 0xa8e61d30, 0xb2e2, 0x4cce, { 0xad, 0x31, 0x73, 0x9e, 0x6c, 0xa8, 0x5d, 0xf1 } };

cfg_int cfg_samplerate		          (guid_samplerate,		       32000);
cfg_int cfg_bitspersample	          (guid_bitspersample,	          16);
cfg_int cfg_channels		          (guid_channels,			       2);
cfg_int cfg_interpolation	          (guid_interpolation,	   INT_GAUSS);
cfg_int cfg_dsp_option		          (guid_options,			       0);
#ifdef _WIN64
cfg_int cfg_fastseek                  (guid_fastseek,                  0);
#else
cfg_int cfg_fastseek		          (guid_fastseek,			       1);
#endif
cfg_int cfg_usespclength	          (guid_usespclength,		       1);
cfg_int cfg_forcelength		          (guid_forcelength,		       1);
cfg_int cfg_songlength		          (guid_songlength,		      180000);
cfg_int cfg_fadelength		          (guid_fadelength,		        2000);
cfg_int cfg_amplification	          (guid_amplification,	          16);
cfg_int cfg_ignore_script700          (guid_ignore_script700,          0);
cfg_int cfg_ignore_xid6_amplification (guid_ignore_xid6_amplification, 0);
#ifdef _WIN64
cfg_int cfg_sem71_enabled             (guid_sem71_enabled,             1);  // default ON
#endif

// combobox setting helper
#define COMBOGET(ID, X) \
	{ \
		int data; \
		data = SendDlgItemMessage(ID, CB_GETCURSEL, 0, 0); \
		data = SendDlgItemMessage(ID, CB_GETITEMDATA, data, 0); \
		if (data != CB_ERR) \
		  { \
			  X = data; \
		  } \
	} \

#define CHECKGET(ID, X) \
  if (SendMessage(ID, BM_GETCHECK, 0, 0) == BST_CHECKED) \
    { \
        X = 1; \
    } \
  else \
    { \
        X = 0; \
    } \

// our config class
class SnesApuPreferences : public CDialogImpl<SnesApuPreferences>, public preferences_page_instance
{
  public:
	SnesApuPreferences(preferences_page_callback::ptr callback) : m_callback(callback) {}

	enum {IDD = IDD_CONFIG};

	t_uint32 get_state();
	void apply();
	void reset();

	//WTL message map
	BEGIN_MSG_MAP(SnesApuPreferences)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_HSCROLL(OnHScroll)
		COMMAND_ID_HANDLER_EX(IDC_DSP_OLDADPCM,     OnEditChange)
		COMMAND_ID_HANDLER_EX(IDC_DSP_REVERSE,      OnEditChange)
		COMMAND_ID_HANDLER_EX(IDC_DSP_DISECHO,      OnEditChange)
		COMMAND_ID_HANDLER_EX(IDC_DSP_DISPITM,      OnEditChange)
		COMMAND_ID_HANDLER_EX(IDC_DSP_DISPITR,      OnEditChange)
		COMMAND_ID_HANDLER_EX(IDC_DSP_DISFIR,       OnEditChange)
		COMMAND_ID_HANDLER_EX(IDC_DSP_BASSBOOST,    OnEditChange)
		COMMAND_ID_HANDLER_EX(IDC_DSP_DISENV,       OnEditChange)
		COMMAND_ID_HANDLER_EX(IDC_DSP_DISNOISE,     OnEditChange)
		COMMAND_ID_HANDLER_EX(IDC_DSP_ECHOMEM,      OnEditChange)
		COMMAND_ID_HANDLER_EX(IDC_DSP_NOSURND,      OnEditChange)
		COMMAND_ID_HANDLER_EX(IDC_DSP_ANALOG,       OnEditChange)
		COMMAND_ID_HANDLER_EX(IDC_USESPCLENGTH,     OnEditChange)
		COMMAND_ID_HANDLER_EX(IDC_FORCELENGTH,      OnEditChange)
		COMMAND_ID_HANDLER_EX(IDC_FASTSEEK,         OnEditChange)
		COMMAND_ID_HANDLER_EX(IDC_IGNORE_SCRIPT700, OnEditChange)
		COMMAND_ID_HANDLER_EX(IDC_IGNORE_XID6_AMP,  OnEditChange)
#ifdef _WIN64
		COMMAND_ID_HANDLER_EX(IDC_SEM71_ENABLED,    OnEditChange)
#endif
		COMMAND_HANDLER_EX(IDC_SONGLENGTH,    EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_FADELENGTH,    EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_SAMPLERATE,    CBN_SELCHANGE, OnComboChange)
		COMMAND_HANDLER_EX(IDC_BITS,          CBN_SELCHANGE, OnComboChange)
		COMMAND_HANDLER_EX(IDC_CHANNELS,      CBN_SELCHANGE, OnComboChange)
		COMMAND_HANDLER_EX(IDC_INTERPOLATION, CBN_SELCHANGE, OnComboChange)
	END_MSG_MAP()

  private:

	BOOL OnInitDialog(CWindow, LPARAM);
	void OnEditChange(UINT, int, CWindow);
	void OnComboChange(UINT, int, CWindow);
	void OnHScroll(int, short, CWindow);
	bool HasChanged();
	void OnChanged();

	const preferences_page_callback::ptr m_callback;
	CTrackBarCtrl m_Slider;

	int m_IDLastChanged;

	void ComboPopulate(int combo, const wchar_t **names, const int *vals, int len, int def);

	static const wchar_t *sampleratenames[13];
	static const int samplerates[13];

	static const wchar_t *internames[6];
	static const int intermodes[6];

	static const wchar_t *bitnames[4];
	static const int bitmodes[4];

	static const wchar_t *channames[2];
	static const int chanmodes[2];

#if FOOBAR2000_SDK_VERSION >= 20221020
	fb2k::CCoreDarkModeHooks m_dark;
#endif
};

const wchar_t *SnesApuPreferences::sampleratenames[13] = {_T("8000"),_T("11025"),_T("16000"),_T("22050"),_T("24000"),_T("32000"),_T("44100"),_T("48000"),_T("64000"),_T("88200"),_T("96000"), _T("176400"), _T("192000")};
const int SnesApuPreferences::samplerates[13] = { 8000,  11025,  16000,  22050,  24000,  32000,  44100,  48000,  64000,  88200,  96000, 176400, 192000};

const wchar_t *SnesApuPreferences::internames[6] = {_T("None"),_T("Linear"),_T("Cubic"),_T("Gaussian"),_T("Sinc"),_T("Gaussian4")};
const int SnesApuPreferences::intermodes[6] = {INT_NONE,	INT_LINEAR,	INT_CUBIC,	INT_GAUSS,	INT_SINC,	INT_GAUSS4};

const wchar_t *SnesApuPreferences::bitnames[4] = {_T("8"),_T("16"),_T("24"),_T("32")};
const int SnesApuPreferences::bitmodes[4] = { 8,  16,  24,  32};

const wchar_t *SnesApuPreferences::channames[2] = {_T("Mono"),_T("Stereo")};
const int SnesApuPreferences::chanmodes[2] = { 1,	 2};

// combo populator helper
inline void SnesApuPreferences::ComboPopulate(int combo, const wchar_t **names, const int *vals, int len, int def)
{
	SendDlgItemMessage(combo, CB_RESETCONTENT, 0, 0);

	int index;
	int defindex = 0;
	for (int i=0 ; i<len ; ++i) {
		index = SendDlgItemMessage(combo, CB_INSERTSTRING, -1, (LPARAM)names[i]);
		SendDlgItemMessage(combo, CB_SETITEMDATA, index, vals[i]);

		if (vals[i] == def) {
			defindex = i;
		}
	}
	SendDlgItemMessage(combo, CB_SETCURSEL, defindex, -1);
}

#define MAX_DSP_OPT 12
// populate dsp options (IDC_DSP_SURROUND slot is now Semantic 7.1 output)
static const int g_DSP_OPT[][2] = {
	{IDC_DSP_OLDADPCM,	DSP_OLDSMP},
	{IDC_DSP_REVERSE,	DSP_REVERSE},
	{IDC_DSP_DISECHO,	DSP_NOECHO},
	{IDC_DSP_DISPITM,	DSP_NOPMOD},
	{IDC_DSP_DISPITR,	DSP_NOPREAD},
	{IDC_DSP_DISFIR,	DSP_NOFIR},
	{IDC_DSP_BASSBOOST,	DSP_BASS},
	{IDC_DSP_DISENV,	DSP_NOENV},
	{IDC_DSP_DISNOISE,	DSP_NONOISE},
	{IDC_DSP_ECHOMEM,	DSP_ECHOMEM},
	{IDC_DSP_NOSURND,	DSP_NOSURND},
	{IDC_DSP_ANALOG,	DSP_ANALOG}
};

BOOL SnesApuPreferences::OnInitDialog(CWindow pWin, LPARAM)
{
	HWND hwnd = pWin;
#if FOOBAR2000_SDK_VERSION >= 20220810
	m_dark.AddDialogWithControls(m_hWnd);
#endif

	// populate comboboxes
	ComboPopulate(IDC_SAMPLERATE,	sampleratenames, samplerates, 13, cfg_samplerate);
	ComboPopulate(IDC_BITS,			bitnames,		 bitmodes,	  4,  cfg_bitspersample);
	ComboPopulate(IDC_CHANNELS,		channames,		 chanmodes,   2,  cfg_channels);
	ComboPopulate(IDC_INTERPOLATION, internames,	 intermodes,  6,  cfg_interpolation);

	for (int i=0 ; i<MAX_DSP_OPT; i++) {
		SendDlgItemMessage(g_DSP_OPT[i][0], BM_SETCHECK, !!(cfg_dsp_option & g_DSP_OPT[i][1]), 0);
	}

	SendDlgItemMessage(IDC_FORCELENGTH,  BM_SETCHECK, cfg_forcelength, 0);
	SendDlgItemMessage(IDC_USESPCLENGTH,  BM_SETCHECK, cfg_usespclength, 0);

	SetDlgItemInt(IDC_SONGLENGTH, cfg_songlength, FALSE);
	SetDlgItemInt(IDC_FADELENGTH, cfg_fadelength, FALSE);

#ifdef _WIN64
	SendDlgItemMessage(IDC_FASTSEEK, BM_SETCHECK, 0, 0);
	::EnableWindow(GetDlgItem(IDC_FASTSEEK), FALSE);
#else
	SendDlgItemMessage(IDC_FASTSEEK,  BM_SETCHECK, cfg_fastseek, 0);
#endif

	m_Slider = GetDlgItem(IDC_VOLUME);
	m_Slider.SetTicFreq(16);
	m_Slider.SetRange(0, 255);
	m_Slider.SetPos(cfg_amplification);

	SetDlgItemInt(IDC_VOLUME_VALUE, cfg_amplification, FALSE);

	SendDlgItemMessage(IDC_IGNORE_SCRIPT700, BM_SETCHECK, cfg_ignore_script700, 0);
	SendDlgItemMessage(IDC_IGNORE_XID6_AMP, BM_SETCHECK, cfg_ignore_xid6_amplification, 0);
#ifdef _WIN64
	SendDlgItemMessage(IDC_SEM71_ENABLED, BM_SETCHECK, cfg_sem71_enabled, 0);
#else
	// Semantic 7.1 is x64-only; disable the checkbox on x86
	::EnableWindow(GetDlgItem(IDC_SEM71_ENABLED), FALSE);
#endif

	m_IDLastChanged = -1;

	return TRUE;
}

void SnesApuPreferences::OnEditChange(UINT pNotify, int pId, CWindow)
{
	m_IDLastChanged = pId;
	OnChanged();
}

void SnesApuPreferences::OnComboChange(UINT pNotify, int pId, CWindow)
{
	m_IDLastChanged = pId;
	OnChanged();
}

void SnesApuPreferences::OnHScroll(int nSBCode, short nPos, CWindow hWnd)
{
	if (m_Slider == hWnd) {
		CTrackBarCtrl hSlider = hWnd.m_hWnd;
		SetDlgItemInt(IDC_VOLUME_VALUE, hSlider.GetPos(), FALSE);
		m_IDLastChanged = IDC_VOLUME;
		OnChanged();
	}
}

t_uint32 SnesApuPreferences::get_state()
{
	t_uint32 state = preferences_state::resettable;
#if FOOBAR2000_SDK_VERSION >= 20220810
	state |= preferences_state::dark_mode_supported;
#endif
	if (HasChanged()) state |= preferences_state::changed;
	return state;
}

void SnesApuPreferences::reset()
{
	for (int i=0 ; i<MAX_DSP_OPT; i++) {
		CButton btn = GetDlgItem(g_DSP_OPT[i][0]).m_hWnd;
		btn.SetCheck(0);
	}
	((CButton)GetDlgItem(IDC_USESPCLENGTH)).SetCheck(1);
	((CButton)GetDlgItem(IDC_FORCELENGTH)).SetCheck(1);
#ifdef _WIN64
	((CButton)GetDlgItem(IDC_FASTSEEK)).SetCheck(0);
#else
	((CButton)GetDlgItem(IDC_FASTSEEK)).SetCheck(1);
#endif
	((CButton)GetDlgItem(IDC_IGNORE_SCRIPT700)).SetCheck(0);
	((CButton)GetDlgItem(IDC_IGNORE_XID6_AMP)).SetCheck(0);
#ifdef _WIN64
	((CButton)GetDlgItem(IDC_SEM71_ENABLED)).SetCheck(1);  // default ON
#endif

	SetDlgItemInt(IDC_SONGLENGTH, 180000, FALSE);
	SetDlgItemInt(IDC_FADELENGTH, 2000, FALSE);

	m_Slider.SetPos(16);
	SetDlgItemInt(IDC_VOLUME_VALUE, 16, FALSE);

	((CComboBox)GetDlgItem(IDC_SAMPLERATE)).SetCurSel(5);
	((CComboBox)GetDlgItem(IDC_INTERPOLATION)).SetCurSel(3);
	((CComboBox)GetDlgItem(IDC_BITS)).SetCurSel(1);
	((CComboBox)GetDlgItem(IDC_CHANNELS)).SetCurSel(1);

	m_IDLastChanged = -1;
	OnChanged();
}

void SnesApuPreferences::apply()
{
	int dsp_opt = 0;
	for (int i=0 ; i<MAX_DSP_OPT; i++) {
		CButton btn = GetDlgItem(g_DSP_OPT[i][0]).m_hWnd;
		if (btn.GetCheck()) {
			dsp_opt |= g_DSP_OPT[i][1];
		}
	}
	cfg_dsp_option = dsp_opt;

	COMBOGET(IDC_INTERPOLATION, cfg_interpolation);
	COMBOGET(IDC_CHANNELS, cfg_channels);
	cfg_bitspersample = GetDlgItemInt(IDC_BITS, NULL, FALSE);
	cfg_samplerate    = GetDlgItemInt(IDC_SAMPLERATE, NULL, FALSE);

	cfg_songlength = GetDlgItemInt(IDC_SONGLENGTH, NULL, FALSE);
	cfg_fadelength = GetDlgItemInt(IDC_FADELENGTH, NULL, FALSE);

	cfg_fastseek = SendDlgItemMessage(IDC_FASTSEEK, BM_GETCHECK, 0, 0);
	cfg_amplification = m_Slider.GetPos();

	cfg_forcelength = SendDlgItemMessage(IDC_FORCELENGTH,  BM_GETCHECK, 0, 0);
	cfg_usespclength = SendDlgItemMessage(IDC_USESPCLENGTH,  BM_GETCHECK, 0, 0);

	cfg_ignore_script700 = SendDlgItemMessage(IDC_IGNORE_SCRIPT700, BM_GETCHECK, 0, 0);
	cfg_ignore_xid6_amplification = SendDlgItemMessage(IDC_IGNORE_XID6_AMP, BM_GETCHECK, 0, 0);
#ifdef _WIN64
	cfg_sem71_enabled = SendDlgItemMessage(IDC_SEM71_ENABLED, BM_GETCHECK, 0, 0);
#endif

	OnChanged();
}
bool SnesApuPreferences::HasChanged()
{
	int m_IDLastChanged = this->m_IDLastChanged; this->m_IDLastChanged = -1;
	int dsp_opt = cfg_dsp_option;
	for (int i=0 ; i<MAX_DSP_OPT; i++) {
		if (m_IDLastChanged == -1 || g_DSP_OPT[i][0] == m_IDLastChanged) {
			CButton btn = GetDlgItem(g_DSP_OPT[i][0]).m_hWnd;
			if (btn.GetCheck()) {
				dsp_opt |= g_DSP_OPT[i][1];
			}
			else {
				dsp_opt &= ~g_DSP_OPT[i][1];
			}
			if (m_IDLastChanged != -1) break;
		}
	}
	if (dsp_opt != cfg_dsp_option) return true;
	if (m_IDLastChanged == -1 || m_IDLastChanged == IDC_INTERPOLATION) {
		COMBOGET(IDC_INTERPOLATION, dsp_opt); if (dsp_opt != cfg_interpolation) return true;
	}
	if (m_IDLastChanged == -1 || m_IDLastChanged == IDC_CHANNELS) {
		COMBOGET(IDC_CHANNELS, dsp_opt); if (dsp_opt != cfg_channels) return true;
	}
	if (m_IDLastChanged == -1 || m_IDLastChanged == IDC_BITS) {
		if (GetDlgItemInt(IDC_BITS, NULL, FALSE) != cfg_bitspersample) return true;
	}
	if (m_IDLastChanged == -1 || m_IDLastChanged == IDC_SAMPLERATE) {
		if (GetDlgItemInt(IDC_SAMPLERATE, NULL, FALSE) != cfg_samplerate) return true;
	}
	if (m_IDLastChanged == -1 || m_IDLastChanged == IDC_SONGLENGTH) {
		if (GetDlgItemInt(IDC_SONGLENGTH, NULL, FALSE) != cfg_songlength) return true;
	}
	if (m_IDLastChanged == -1 || m_IDLastChanged == IDC_FADELENGTH) {
		if (GetDlgItemInt(IDC_FADELENGTH, NULL, FALSE) != cfg_fadelength) return true;
	}
	if (m_IDLastChanged == -1 || m_IDLastChanged == IDC_USESPCLENGTH) {
		if (SendDlgItemMessage(IDC_USESPCLENGTH, BM_GETCHECK) != cfg_usespclength) return true;
	}
	if (m_IDLastChanged == -1 || m_IDLastChanged == IDC_FORCELENGTH) {
		if (SendDlgItemMessage(IDC_FORCELENGTH, BM_GETCHECK) != cfg_forcelength) return true;
	}
	if (m_IDLastChanged == -1 || m_IDLastChanged == IDC_FASTSEEK) {
		if (SendDlgItemMessage(IDC_FASTSEEK, BM_GETCHECK) != cfg_fastseek) return true;
	}
	if (m_IDLastChanged == -1 || m_IDLastChanged == IDC_VOLUME) {
		if (m_Slider.GetPos() != cfg_amplification) return true;
	}
	if (m_IDLastChanged == -1 || m_IDLastChanged == IDC_IGNORE_SCRIPT700) {
		if (SendDlgItemMessage(IDC_IGNORE_SCRIPT700, BM_GETCHECK) != cfg_ignore_script700) return true;
	}
	if (m_IDLastChanged == -1 || m_IDLastChanged == IDC_IGNORE_XID6_AMP) {
		if (SendDlgItemMessage(IDC_IGNORE_XID6_AMP, BM_GETCHECK) != cfg_ignore_xid6_amplification) return true;
	}
#ifdef _WIN64
	if (m_IDLastChanged == -1 || m_IDLastChanged == IDC_SEM71_ENABLED) {
		if (SendDlgItemMessage(IDC_SEM71_ENABLED, BM_GETCHECK) != cfg_sem71_enabled) return true;
	}
#endif
	return false;
}
void SnesApuPreferences::OnChanged()
{
	m_callback->on_state_changed();
}

class preferences_page_snesapu : public preferences_page_impl<SnesApuPreferences>
{
  public:
	const char * get_name() {return "SNESAPU input";}
	GUID get_guid() {
		return guid_snesapu_preference_page;
	}
	GUID get_parent_guid() {return guid_input;}
};

static preferences_page_factory_t<preferences_page_snesapu> g_preferences_snesapu_factory;
