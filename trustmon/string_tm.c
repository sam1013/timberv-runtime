//~ #include <string.h>
#include "trustmon.h"
#include "untrusted.h"
#include "libc.h"
#include "mini-printf.h"
#include "tag.h"
#include <stdint.h>
#include <ctype.h>

void* memcpy(void* dest, const void* src, size_t len)
{
  const char* s = src;
  char *d = dest;

  if ((((uintptr_t)dest | (uintptr_t)src) & (sizeof(uintptr_t)-1)) == 0) {
    while (d < ((char*)dest + len - (sizeof(uintptr_t)-1))) {
      *(uintptr_t*)d = *(const uintptr_t*)s;
      d += sizeof(uintptr_t);
      s += sizeof(uintptr_t);
    }
  }

  while (d < ((char*)dest + len))
    *d++ = *s++;

  return dest;
}

void* memset(void* dest, int byte, size_t len)
{
  if ((((uintptr_t)dest | len) & (sizeof(uintptr_t)-1)) == 0) {
    uintptr_t word = byte & 0xFF;
    word |= word << 8;
    word |= word << 16;
    word |= word << 16 << 16;

    uintptr_t *d = dest;
    while (d < (uintptr_t*)((char*)dest + len))
      *d++ = word;
  } else {
    char *d = dest;
    while (d < ((char*)dest + len))
      *d++ = byte;
  }
  return dest;
}

size_t strlen(const char *s)
{
  const char *p = s;
  while (*p)
    p++;
  return p - s;
}

int strcmp(const char* s1, const char* s2)
{
  unsigned char c1, c2;

  do {
    c1 = *s1++;
    c2 = *s2++;
  } while (c1 != 0 && c1 == c2);

  return c1 - c2;
}

char* strcpy(char* dest, const char* src)
{
  char* d = dest;
  while ((*d++ = *src++))
    ;
  return dest;
}

long atol(const char* str)
{
  long res = 0;
  int sign = 0;

  while (*str == ' ')
    str++;

  if (*str == '-' || *str == '+') {
    sign = *str == '-';
    str++;
  }

  while (*str) {
    res *= 10;
    res += *str++ - '0';
  }

  return sign ? -res : res;
}

#if TM_PRINTF == 1

void memcpy_insecure(void* dst, void* src, size_t len) {
  /* TODO: check alignment */
  uword_t* pdst = dst;
  uword_t* psrc = src;
  while(len -= sizeof(uword_t)) {
    ato_check_set_tag_data(pdst, T_NORMAL, T_NORMAL, *psrc);
    pdst++;
    psrc++;
  }
}

__attribute__((aligned(8)))
static char secure_buf[TM_BUF_SIZE];

int tm_printf (const char *format, ...)
{
   va_list arg;
   int done;

   va_start (arg, format);
   done = mini_vsnprintf (secure_buf, TM_BUF_SIZE, format, arg);
   va_end (arg);

   memcpy_insecure(normal_buf, secure_buf, TM_BUF_SIZE);

   asm volatile(".global tm_printf_exit\n"
               "la a0, normal_buf\n"
               "la ra, tm_printf_exit\n"
               "j puts\n" /* we cannot use jal here since caller will return into nops before tm_printf_exit, which are no valid callable entry point */
               ".align 3\n"
               "tm_printf_exit: \n" /* This is the only valid callable entry point */
               : : : "a0", "ra");

   return done;
}

#endif //TM_PRINTF == 1
