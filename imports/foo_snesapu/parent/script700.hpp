#ifndef __SCRIPT_700___
#define __SCRIPT_700___

//#include <foobar2000.h>

class script700
{
  public:

	bool load(const char *p_path);
	void* get_ptr();
	t_size get_size();

  private:

	bool read_file(pfc::string8 &p_path);
	pfc::array_t<char> m_script_buffer;
};

#endif
