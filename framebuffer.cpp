#include "framebuffer.h"

#if defined(__linux__)

// Linux
#include <linux/fb.h>

// POSIX
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>

// PDTK
#include <cxxutils/error_helpers.h>
#include <cxxutils/colors.h>


bool FrameBuffer::open(const char* device)
{
  m_fd = posix::open(device, O_RDWR);

  flaw(m_fd < 0, posix::warning,, false,
       "Unable to open framebuffer device: %s", std::strerror(errno))

  struct fb_var_screeninfo screen_info;
  flaw(posix::ioctl(m_fd, FBIOGET_VSCREENINFO, &screen_info), posix::warning,, false,
       "Unable to get virtual screen info from framebuffer: %s", std::strerror(errno))

  struct fb_fix_screeninfo fixed_info;
  flaw(posix::ioctl(m_fd, FBIOGET_FSCREENINFO, &fixed_info), posix::warning,, false,
       "Unable to get fixed screen info from framebuffer: %s", std::strerror(errno))

  m_bufheight = screen_info.yres_virtual;
  m_bufwidth  = fixed_info.line_length;
  m_buffer = ::mmap(nullptr, m_bufwidth * m_bufheight, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, 0);

  return m_buffer != nullptr &&
         m_buffer != MAP_FAILED;
}
// FBIOPUT_VSCREENINFO
void FrameBuffer::close(void)
{
  if(m_buffer != nullptr &&
     m_buffer != MAP_FAILED)
  {
    ::munmap(m_buffer, m_bufwidth * m_bufheight);
    m_buffer = nullptr;
  }

  if(m_fd != posix::error_response)
  {
    posix::close(m_fd);
    m_fd = posix::error_response;
  }
}

#else

#endif
