/**
 *
 * Sunburst版SNESAPU用SCRIPT700読み込みクラス
 *
 */
#include "stdafx.h"
#include "script700.hpp"

inline bool script700::read_file(pfc::string8 &p_path) {
	service_ptr_t<file> r700;
	abort_callback_impl abort_cb;
	try {
		filesystem::g_open(r700, p_path, filesystem::open_mode_read, abort_cb);
		t_size size = static_cast<t_size>(r700->get_size(abort_cb));
		m_script_buffer.set_size(size + 1);
		r700->read(m_script_buffer.get_ptr(), size, abort_cb);
		m_script_buffer[size] = '\0';
		return true;
	} catch (exception_io e) {
		// ここでcatchしておかないとファイル読み込みエラーで演奏終了する。
		return false;
	}
}

bool script700::load(const char *p_path)
{
	// .700
	pfc::string8 file_name(p_path);
	int num = file_name.find_last('.');
	file_name.remove_chars(num+1, file_name.length()-(num+1));
	file_name.add_string("700");
	if (read_file(file_name))
	  return true;

	// .7SE
	file_name.remove_chars(num+1, file_name.length()-(num+1));
	file_name.add_string("7SE");
	if (read_file(file_name))
	  return true;

	// 65816.700
	num = file_name.find_last('\\');
	int num2 = file_name.find_last('|');
	num = (num>num2)?num:num2;
	file_name.remove_chars(num+1, file_name.length()-(num+1));
	file_name.add_string("65816.700");
	if (read_file(file_name))
	  return true;

	// 65816.7SE
	file_name.remove_chars(num+1, file_name.length()-(num+1));
	file_name.add_string("65816.7SE");
	if (read_file(file_name))
	  return true;

	return false;
}

void* script700::get_ptr()
{
	if (m_script_buffer.get_size() == 0) {
		return NULL;
	}
	return m_script_buffer.get_ptr();
}

t_size script700::get_size()
{
	return m_script_buffer.get_size();
}
