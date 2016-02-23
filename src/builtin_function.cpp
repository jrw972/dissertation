#include "builtin_function.hpp"

#include <error.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/timerfd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <netdb.h>

#include "symbol_visitor.hpp"
#include "executor_base.hpp"
#include "heap.hpp"
#include "ast.hpp"
#include "runtime.hpp"
#include "semantic.hpp"
#include "parameter_list.hpp"

namespace decl
{

using namespace type;
using namespace ast;
using namespace semantic;

BuiltinFunction::BuiltinFunction (const std::string& id,
                                  const util::Location& loc,
                                  const type::Function* type)
  : Symbol (id, loc)
  , type_ (type)
{
  allocate_parameters (memory_model_, type_->parameter_list);
  allocate_symbol (memory_model_, type_->GetReturnParameter ());
}

void
BuiltinFunction::accept (decl::SymbolVisitor& visitor)
{
  visitor.visit (*this);
}

void
BuiltinFunction::accept (decl::ConstSymbolVisitor& visitor) const
{
  visitor.visit (*this);
}

size_t BuiltinFunction::arguments_size () const
{
  return type_->parameter_list->allocation_size ();
}

Readable::Readable (const util::Location& loc)
  : BuiltinFunction ("readable",
                     loc,
                     new type::Function (type::Function::FUNCTION, (new ParameterList ())
                                         ->append (ParameterSymbol::make (loc, "fd", &type::named_file_descriptor, Immutable, Foreign)),
                                         (new ParameterList ())->append (ParameterSymbol::makeReturn (loc, ReturnSymbol, &type::named_bool, Immutable))))
{ }

void
Readable::call (runtime::ExecutorBase& exec) const
{
  runtime::FileDescriptor** fd = static_cast< runtime::FileDescriptor**> (exec.stack ().get_address (type_->parameter_list->at (0)->offset ()));
  Bool::ValueType* r = static_cast<Bool::ValueType*> (exec.stack ().get_address (type_->GetReturnParameter ()->offset ()));

  struct pollfd pfd;
  pfd.fd = (*fd)->fd ();
  pfd.events = POLLIN;

  int s = poll (&pfd, 1, 0);

  if (s < 0)
    {
      error (EXIT_FAILURE, errno, "poll");
    }

  exec.checkedForReadability (*fd);

  *r = (pfd.revents & POLLIN) != 0;
}

Read::Read (const util::Location& loc)
  : BuiltinFunction ("read",
                     loc,
                     new type::Function (type::Function::FUNCTION, (new ParameterList ())
                                         ->append (ParameterSymbol::make (loc, "fd", &type::named_file_descriptor, Immutable, Mutable))
                                         ->append (ParameterSymbol::make (loc, "buf", type::named_byte.get_slice (), Immutable, Mutable)),
                                         (new ParameterList ())->append (ParameterSymbol::makeReturn (loc, ReturnSymbol, Int::Instance (), Immutable))))
{ }

void
Read::call (runtime::ExecutorBase& exec) const
{
  runtime::FileDescriptor** fd = static_cast< runtime::FileDescriptor**> (exec.stack ().get_address (type_->parameter_list->at (0)->offset ()));
  Slice::ValueType* buf = static_cast<Slice::ValueType*> (exec.stack ().get_address (type_->parameter_list->at (1)->offset ()));
  Int::ValueType* r = static_cast<Int::ValueType*> (exec.stack ().get_address (type_->GetReturnParameter ()->offset ()));
  *r = read ((*fd)->fd (), buf->ptr, buf->length);
}

Writable::Writable (const util::Location& loc)
  : BuiltinFunction ("writable",
                     loc,
                     new type::Function (type::Function::FUNCTION, (new ParameterList ())
                                         ->append (ParameterSymbol::make (loc, "fd", &type::named_file_descriptor, Immutable, Foreign)),
                                         (new ParameterList ())->append (ParameterSymbol::makeReturn (loc, ReturnSymbol, &type::named_bool, Immutable))))
{ }

void
Writable::call (runtime::ExecutorBase& exec) const
{
  runtime::FileDescriptor** fd = static_cast< runtime::FileDescriptor**> (exec.stack ().get_address (type_->parameter_list->at (0)->offset ()));
  Bool::ValueType* r = static_cast<Bool::ValueType*> (exec.stack ().get_address (type_->GetReturnParameter ()->offset ()));

  struct pollfd pfd;
  pfd.fd = (*fd)->fd ();
  pfd.events = POLLOUT;

  int s = poll (&pfd, 1, 0);

  if (s < 0)
    {
      error (EXIT_FAILURE, errno, "poll");
    }

  exec.checkedForWritability (*fd);

  *r = (pfd.revents & POLLOUT) != 0;
}

ClockGettime::ClockGettime (const util::Location& loc)
  : BuiltinFunction ("clock_gettime",
                     loc,
                     new type::Function (type::Function::FUNCTION, (new ParameterList ())
                                         ->append (ParameterSymbol::make (loc, "tp", type::named_timespec.get_pointer (), Immutable, Foreign)),
                                         (new ParameterList ())->append (ParameterSymbol::makeReturn (loc, ReturnSymbol, &type::named_int, Immutable))))
{ }

void
ClockGettime::call (runtime::ExecutorBase& exec) const
{
  struct timespec* ts = *static_cast< struct timespec**> (exec.stack ().get_address (type_->parameter_list->at (0)->offset ()));
  Int::ValueType* r = static_cast<Int::ValueType*> (exec.stack ().get_address (type_->GetReturnParameter ()->offset ()));
  *r = clock_gettime (CLOCK_REALTIME, ts);
}

TimerfdCreate::TimerfdCreate (const util::Location& loc)
  : BuiltinFunction ("timerfd_create",
                     loc,
                     new type::Function (type::Function::FUNCTION, new ParameterList (),
                                         (new ParameterList ())->append (ParameterSymbol::makeReturn (loc, ReturnSymbol, &type::named_file_descriptor, Mutable))))
{ }

void
TimerfdCreate::call (runtime::ExecutorBase& exec) const
{
  runtime::FileDescriptor** ret = static_cast< runtime::FileDescriptor**> (exec.stack ().get_address (type_->GetReturnParameter ()->offset ()));
  int fd = timerfd_create (CLOCK_MONOTONIC, TFD_NONBLOCK);
  if (fd != -1)
    {
      *ret = exec.allocateFileDescriptor (fd);
    }
  else
    {
      *ret = NULL;
    }
}

TimerfdSettime::TimerfdSettime (const util::Location& loc)
  : BuiltinFunction ("timerfd_settime",
                     loc,
                     new type::Function (type::Function::FUNCTION, (new ParameterList ())
                                         ->append (ParameterSymbol::make (loc, "fd", &type::named_file_descriptor, Immutable, Mutable))
                                         ->append (ParameterSymbol::make (loc, "s", &type::named_uint64, Immutable, Immutable)),
                                         (new ParameterList ())->append (ParameterSymbol::makeReturn (loc, ReturnSymbol, &type::named_int, Immutable))))
{ }

void
TimerfdSettime::call (runtime::ExecutorBase& exec) const
{
  runtime::FileDescriptor** fd = static_cast< runtime::FileDescriptor**> (exec.stack ().get_address (type_->parameter_list->at (0)->offset ()));
  Uint64::ValueType* v = static_cast<Uint64::ValueType*> (exec.stack ().get_address (type_->parameter_list->at (1)->offset ()));
  Int::ValueType* r = static_cast<Int::ValueType*> (exec.stack ().get_address (type_->GetReturnParameter ()->offset ()));

  struct itimerspec spec;
  spec.it_interval.tv_sec = *v;
  spec.it_interval.tv_nsec = 0;
  spec.it_value.tv_sec = *v;
  spec.it_value.tv_nsec = 0;
  *r = timerfd_settime ((*fd)->fd (), 0, &spec, NULL);
}

UdpSocket::UdpSocket (const util::Location& loc)
  : BuiltinFunction ("udp_socket",
                     loc,
                     new type::Function (type::Function::FUNCTION, new ParameterList (),
                                         (new ParameterList ())->append (ParameterSymbol::makeReturn (loc, ReturnSymbol, &type::named_file_descriptor, Mutable))))
{ }

void
UdpSocket::call (runtime::ExecutorBase& exec) const
{
  runtime::FileDescriptor** ret = static_cast< runtime::FileDescriptor**> (exec.stack ().get_address (type_->GetReturnParameter ()->offset ()));

  int fd = socket (AF_INET, SOCK_DGRAM, 0);
  if (fd == -1)
    {
      *ret = NULL;
      return;
    }

  int s = fcntl (fd, F_SETFL, O_NONBLOCK);
  if (s == -1)
    {
      *ret = NULL;
      return;
    }

  *ret = exec.allocateFileDescriptor (fd);
}

Sendto::Sendto (const util::Location& loc)
  : BuiltinFunction ("sendto",
                     loc,
                     new type::Function (type::Function::FUNCTION, (new ParameterList ())
                                         ->append (ParameterSymbol::make (loc, "fd", &type::named_file_descriptor, Immutable, Mutable))
                                         ->append (ParameterSymbol::make (loc, "host", &type::named_string, Immutable, Foreign))
                                         ->append (ParameterSymbol::make (loc, "port", &type::named_uint16, Immutable, Immutable))
                                         ->append (ParameterSymbol::make (loc, "buf", type::named_byte.get_slice (), Immutable, Foreign)),
                                         (new ParameterList ())->append (ParameterSymbol::makeReturn (loc, ReturnSymbol, Int::Instance (), Immutable))))
{ }

void
Sendto::call (runtime::ExecutorBase& exec) const
{
  runtime::FileDescriptor** fd = static_cast< runtime::FileDescriptor**> (exec.stack ().get_address (type_->parameter_list->at (0)->offset ()));
  StringU::ValueType* host = static_cast<StringU::ValueType*> (exec.stack ().get_address (type_->parameter_list->at (1)->offset ()));
  Uint16::ValueType* port = static_cast<Uint16::ValueType*> (exec.stack ().get_address (type_->parameter_list->at (2)->offset ()));
  Slice::ValueType* buf = static_cast<Slice::ValueType*> (exec.stack ().get_address (type_->parameter_list->at (3)->offset ()));
  Int::ValueType* ret = static_cast<Int::ValueType*> (exec.stack ().get_address (type_->GetReturnParameter ()->offset ()));

  std::string host2 (static_cast<const char*> (host->ptr), host->length);
  std::stringstream port2;
  port2 << *port;

  struct addrinfo* info;
  struct addrinfo hints;
  memset (&hints, 0, sizeof (struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG | AI_NUMERICSERV;
  int r = getaddrinfo (host2.c_str (), port2.str ().c_str (), &hints, &info);
  if (r != 0)
    {
      UNIMPLEMENTED;
    }

  ssize_t s = sendto ((*fd)->fd (), buf->ptr, buf->length, 0, info->ai_addr, info->ai_addrlen);
  if (s != static_cast<ssize_t> (buf->length))
    {
      UNIMPLEMENTED;
    }

  freeaddrinfo (info);
  *ret = 0;
}

}
