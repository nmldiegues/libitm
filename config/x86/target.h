/* Copyright (C) 2008-2013 Free Software Foundation, Inc.
   Contributed by Richard Henderson <rth@redhat.com>.

   This file is part of the GNU Transactional Memory Library (libitm).

   Libitm is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   Libitm is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   Under Section 7 of GPL version 3, you are granted additional
   permissions described in the GCC Runtime Library Exception, version
   3.1, as published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License and
   a copy of the GCC Runtime Library Exception along with this program;
   see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
   <http://www.gnu.org/licenses/>.  */

// We'll be using some of the cpu builtins, and their associated types.
#include <x86intrin.h>
#include <cpuid.h>

namespace GTM HIDDEN {

/* ??? This doesn't work for Win64.  */
typedef struct gtm_jmpbuf
{
  void *cfa;
#ifdef __x86_64__
  unsigned long long rbx;
  unsigned long long rbp;
  unsigned long long r12;
  unsigned long long r13;
  unsigned long long r14;
  unsigned long long r15;
  unsigned long long rip;
#else
  unsigned long ebx;
  unsigned long esi;
  unsigned long edi;
  unsigned long ebp;
  unsigned long eip;
#endif
} gtm_jmpbuf;

/* x86 doesn't require strict alignment for the basic types.  */
#define STRICT_ALIGNMENT 0

/* x86 uses a fixed page size of 4K.  */
#define PAGE_SIZE       4096
#define FIXED_PAGE_SIZE 1

/* The size of one line in hardware caches (in bytes). */
#define HW_CACHELINE_SIZE 64


static inline void
cpu_relax (void)
{
  __builtin_ia32_pause ();
}

// Use Intel RTM if supported by the assembler.
// See gtm_thread::begin_transaction for how these functions are used.
#ifdef HAVE_AS_RTM
#define USE_HTM_FASTPATH

static inline bool
htm_available ()
{
  const unsigned cpuid_rtm = bit_RTM;
  if (__get_cpuid_max (0, NULL) >= 7)
    {
      unsigned a, b, c, d;
      __cpuid_count (7, 0, a, b, c, d);
      if (b & cpuid_rtm)
	return true;
    }
  return false;
}

static inline uint32_t
htm_init ()
{
  // Maximum number of times we try to execute a transaction as a HW
  // transaction.
  // ??? Why 2?  Any offline or runtime tuning necessary?
  return htm_available () ? 5 : 0;
}

static inline uint32_t
htm_begin ()
{
  return _xbegin();
}

static inline bool
htm_begin_success (uint32_t begin_ret)
{
  return begin_ret == _XBEGIN_STARTED;
}

static inline void
htm_commit ()
{
  _xend();
}

static inline void
htm_abort ()
{
  // ??? According to a yet unpublished ABI rule, 0xff is reserved and
  // supposed to signal a busy lock.  Source: andi.kleen@intel.com
  _xabort(0xff);
}

static inline bool
htm_abort_should_retry (uint32_t begin_ret)
{
  return begin_ret & _XABORT_RETRY;
}
#endif


} // namespace GTM
