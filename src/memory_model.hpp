#ifndef RC_SRC_MEMORY_MODEL_HPP
#define RC_SRC_MEMORY_MODEL_HPP

#include "types.hpp"
#include "util.hpp"
#include "arch.hpp"

namespace runtime
{

/*
 * Utility class for calculate the offsets of arguments and local variables.
 *
 * We assume an x86 calling covention where the caller pushes arguments,
 * the callee then pushes a new base pointer and allocates space for local
 * variables.  This class helps calculate the offset from the base pointer to
 * the arguments and local variables and the size to allocate for local
 * variables at run-time.
 */
class MemoryModel
{
public:
  MemoryModel ()
    : argumentsOffset (-(ptrdiff_t)sizeof (void*))
    , localsOffset (sizeof (void*))
    , localsSize (0)
  { }

  bool ArgumentsEmpty () const
  {
    return argumentsOffset == -(ptrdiff_t)sizeof (void*);
  }

  void ArgumentsPush (size_t size)
  {
    argumentsOffset -= util::align_up (size, arch::stack_alignment ());
  }

  bool LocalsEmpty () const
  {
    return localsOffset == sizeof (void*);
  }

  void LocalsPush (size_t size)
  {
    size = util::align_up (size, arch::stack_alignment ());
    localsOffset += size;
    localsSize += size;
  }

  void LocalsPop (size_t size)
  {
    localsOffset -= util::align_up (size, arch::stack_alignment ());
  }

  ptrdiff_t ArgumentsOffset () const
  {
    return argumentsOffset;
  }

  ptrdiff_t LocalsOffset () const
  {
    return localsOffset;
  }

  size_t LocalsSize () const
  {
    return localsSize;
  }

  void SetReceiverOffset ()
  {
    receiverOffset = ArgumentsOffset ();
  }

  ptrdiff_t ReceiverOffset () const
  {
    return receiverOffset;
  }

private:
  ptrdiff_t argumentsOffset;
  ptrdiff_t localsOffset;
  size_t localsSize;
  ptrdiff_t receiverOffset;
};

}

#endif // RC_SRC_MEMORY_MODEL_HPP
