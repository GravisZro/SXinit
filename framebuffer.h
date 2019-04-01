#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

// PUT
#include <put/cxxutils/posix_helpers.h>

class FrameBuffer
{
public:
  bool open(const char* device = "/dev/fb0");
  void close(void);


private:
  size_t m_bufwidth;
  size_t m_bufheight;
  posix::fd_t m_fd;
  void* m_buffer;
};

#endif // FRAMEBUFFER_H
