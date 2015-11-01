#include "BuiltinFunction.hpp"
#include "SymbolVisitor.hpp"
#include "executor_base.hpp"
#include "heap.hpp"
#include "ast.hpp"
#include "runtime.hpp"

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

using namespace Type;

void
BuiltinFunction::accept (SymbolVisitor& visitor)
{
  visitor.visit (*this);
}

void
BuiltinFunction::accept (ConstSymbolVisitor& visitor) const
{
  visitor.visit (*this);
}

Readable::Readable (ast_t* dn, const Type::Type* fd_type, const Type::Type* bool_type)
  : BuiltinFunction ("readable",
                     dn,
                     new Type::Function (Type::Function::FUNCTION, (new Signature ())
                                         ->Append (new parameter_t (dn, "fd", typed_value_t::make_value (fd_type, typed_value_t::STACK, MUTABLE, IMMUTABLE), false)),
                                         new parameter_t (dn, "0return", typed_value_t::make_value (bool_type, typed_value_t::STACK, MUTABLE, MUTABLE), false)))
{ }

void
Readable::call (executor_base_t& exec, const ast_call_expr_t& node) const
{
  ast_t::const_iterator pos = node.args ()->begin ();
  runtime::evaluate_expr (exec, *pos);
  ::FileDescriptor* fd = static_cast< ::FileDescriptor*> (stack_frame_pop_pointer (exec.stack ()));

  struct pollfd pfd;
  pfd.fd = fd->fd ();
  pfd.events = POLLIN;

  int r = poll (&pfd, 1, 0);

  if (r < 0)
    {
      error (EXIT_FAILURE, errno, "poll");
    }

  exec.checkedForReadability (fd);

  stack_frame_push_tv (exec.stack (), typed_value_t (Bool::Instance (), pfd.revents & POLLIN));
}

Read::Read (ast_t* dn, const Type::Type* fd_type, const Type::Type* uint8_type)
  : BuiltinFunction ("read",
                     dn,
                     new Type::Function (Type::Function::FUNCTION, (new Signature ())
                                         ->Append (new parameter_t (dn, "fd", typed_value_t::make_value (fd_type, typed_value_t::STACK, MUTABLE, MUTABLE), false))
                                         ->Append (new parameter_t (dn, "buf", typed_value_t::make_value (uint8_type->GetSlice (), typed_value_t::STACK, MUTABLE, MUTABLE), false)),
                                         new parameter_t (dn, "0return", typed_value_t::make_value (Int::Instance (), typed_value_t::STACK, MUTABLE, MUTABLE), false)))
{ }

void
Read::call (executor_base_t& exec, const ast_call_expr_t& node) const
{
  typed_value_t buf_tv = type_->GetParameter ("buf")->value;

  ast_t::const_iterator pos = node.args ()->begin ();
  runtime::evaluate_expr (exec, *pos++);
  ::FileDescriptor* fd = static_cast< ::FileDescriptor*> (stack_frame_pop_pointer (exec.stack ()));
  runtime::evaluate_expr (exec, *pos++);
  stack_frame_pop_tv (exec.stack (), buf_tv);
  Slice::ValueType slice = buf_tv.slice_value ();
  int r = read (fd->fd (), slice.ptr, slice.length);
  typed_value_t retval (Int::Instance (), r);
  stack_frame_push_tv (exec.stack (), retval);
}

Writable::Writable (ast_t* dn, const Type::Type* fd_type, const Type::Type* bool_type)
  : BuiltinFunction ("writable",
                     dn,
                     new Type::Function (Type::Function::FUNCTION, (new Signature ())
                                         ->Append (new parameter_t (dn, "fd", typed_value_t::make_value (fd_type, typed_value_t::STACK, MUTABLE, IMMUTABLE), false)),
                                         new parameter_t (dn, "0return", typed_value_t::make_value (bool_type, typed_value_t::STACK, MUTABLE, MUTABLE), false)))
{ }

void
Writable::call (executor_base_t& exec, const ast_call_expr_t& node) const
{
  ast_t::const_iterator pos = node.args ()->begin ();
  runtime::evaluate_expr (exec, *pos);
  ::FileDescriptor* fd = static_cast< ::FileDescriptor*> (stack_frame_pop_pointer (exec.stack ()));

  struct pollfd pfd;
  pfd.fd = fd->fd ();
  pfd.events = POLLOUT;

  int r = poll (&pfd, 1, 0);

  if (r < 0)
    {
      error (EXIT_FAILURE, errno, "poll");
    }

  exec.checkedForWritability (fd);

  stack_frame_push_tv (exec.stack (), typed_value_t (Bool::Instance (), pfd.revents & POLLOUT));
}

TimerfdCreate::TimerfdCreate (ast_t* dn, const Type::Type* fd_type)
  : BuiltinFunction ("timerfd_create",
                     dn,
                     new Type::Function (Type::Function::FUNCTION, new Signature (),
                                         new parameter_t (dn, "0return", typed_value_t::make_value (fd_type, typed_value_t::STACK, MUTABLE, MUTABLE), false)))
{ }

void
TimerfdCreate::call (executor_base_t& exec, const ast_call_expr_t& node) const
{
  int fd = timerfd_create (CLOCK_MONOTONIC, TFD_NONBLOCK);
  if (fd != -1)
    {
      ::FileDescriptor* thefd = exec.allocateFileDescriptor (fd);
      stack_frame_push_pointer (exec.stack (), thefd);
    }
  else
    {
      stack_frame_push_pointer (exec.stack (), NULL);
    }
}

TimerfdSettime::TimerfdSettime (ast_t* dn, const Type::Type* fd_type, const Type::Type* int_type, const Type::Type* uint64_type)
  : BuiltinFunction ("timerfd_settime",
                     dn,
                     new Type::Function (Type::Function::FUNCTION, (new Signature ())
                                         ->Append (new parameter_t (dn, "fd", typed_value_t::make_value (fd_type, typed_value_t::STACK, MUTABLE, MUTABLE), false))
                                         ->Append (new parameter_t (dn, "s", typed_value_t::make_value (uint64_type, typed_value_t::STACK, MUTABLE, MUTABLE), false)),
                                         new parameter_t (dn, "0return", typed_value_t::make_value (int_type, typed_value_t::STACK, MUTABLE, MUTABLE), false)))
{ }

void
TimerfdSettime::call (executor_base_t& exec, const ast_call_expr_t& node) const
{
  ast_t::const_iterator pos = node.args ()->begin ();
  runtime::evaluate_expr (exec, *pos);
  ::FileDescriptor* fd = static_cast< ::FileDescriptor*> (stack_frame_pop_pointer (exec.stack ()));
  ++pos;
  runtime::evaluate_expr (exec, *pos);
  typed_value_t tv (Uint64::Instance (), 0);
  stack_frame_pop_tv (exec.stack (), tv);

  struct itimerspec spec;
  spec.it_interval.tv_sec = tv.value.ref (*Uint64::Instance ());
  spec.it_interval.tv_nsec = 0;
  spec.it_value.tv_sec = tv.value.ref (*Uint64::Instance ());
  spec.it_value.tv_nsec = 0;
  int retval = timerfd_settime (fd->fd (), 0, &spec, NULL);

  stack_frame_push_tv (exec.stack (), typed_value_t (Int::Instance (), retval));
}

UdpSocket::UdpSocket (ast_t* dn, const Type::Type* fd_type)
  : BuiltinFunction ("udp_socket",
                     dn,
                     new Type::Function (Type::Function::FUNCTION, new Signature (),
                                         new parameter_t (dn, "0return", typed_value_t::make_value (fd_type, typed_value_t::STACK, MUTABLE, MUTABLE), false)))
{ }

void
UdpSocket::call (executor_base_t& exec, const ast_call_expr_t& node) const
{
  int fd = socket (AF_INET, SOCK_DGRAM, 0);
  if (fd == -1)
    {
      stack_frame_push_pointer (exec.stack (), NULL);
      return;
    }

  int s = fcntl (fd, F_SETFL, O_NONBLOCK);
  if (s == -1)
    {
      stack_frame_push_pointer (exec.stack (), NULL);
      return;
    }

  ::FileDescriptor* thefd = exec.allocateFileDescriptor (fd);
  stack_frame_push_pointer (exec.stack (), thefd);
}

Sendto::Sendto (ast_t* dn, const Type::Type* fd_type, const Type::Type* uint8_type, const Type::Type* uint16_type)
  : BuiltinFunction ("sendto",
                     dn,
                     new Type::Function (Type::Function::FUNCTION, (new Signature ())
                                         ->Append (new parameter_t (dn, "fd", typed_value_t::make_value (fd_type, typed_value_t::STACK, MUTABLE, MUTABLE), false))
                                         ->Append (new parameter_t (dn, "host", typed_value_t::make_value (uint8_type->GetSlice (), typed_value_t::STACK, MUTABLE, IMMUTABLE), false))
                                         ->Append (new parameter_t (dn, "port", typed_value_t::make_value (uint16_type, typed_value_t::STACK, MUTABLE, IMMUTABLE), false))
                                         ->Append (new parameter_t (dn, "buf", typed_value_t::make_value (uint8_type->GetSlice (), typed_value_t::STACK, MUTABLE, IMMUTABLE), false)),
                                         new parameter_t (dn, "0return", typed_value_t::make_value (Int::Instance (), typed_value_t::STACK, MUTABLE, MUTABLE), false)))
{ }

void
Sendto::call (executor_base_t& exec, const ast_call_expr_t& node) const
{
  typed_value_t host_tv = type_->GetParameter ("host")->value;
  typed_value_t port_tv = type_->GetParameter ("port")->value;
  typed_value_t buf_tv = type_->GetParameter ("buf")->value;

  ast_t::const_iterator pos = node.args ()->begin ();
  runtime::evaluate_expr (exec, *pos++);
  ::FileDescriptor* fd = static_cast< ::FileDescriptor*> (stack_frame_pop_pointer (exec.stack ()));
  runtime::evaluate_expr (exec, *pos++);
  stack_frame_pop_tv (exec.stack (), host_tv);
  runtime::evaluate_expr (exec, *pos++);
  stack_frame_pop_tv (exec.stack (), port_tv);
  runtime::evaluate_expr (exec, *pos++);
  stack_frame_pop_tv (exec.stack (), buf_tv);

  Slice::ValueType host_slice = host_tv.slice_value ();
  std::string host (static_cast<const char*> (host_slice.ptr), host_slice.length);
  std::stringstream port;
  port << port_tv.value.ref (*Uint16::Instance ());
  Slice::ValueType buf_slice = buf_tv.slice_value ();

  struct addrinfo* info;
  struct addrinfo hints;
  memset (&hints, 0, sizeof (struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG | AI_NUMERICSERV;
  int r = getaddrinfo (host.c_str (), port.str ().c_str (), &hints, &info);
  if (r != 0)
    {
      unimplemented;
    }

  ssize_t s = sendto (fd->fd (), buf_slice.ptr, buf_slice.length, 0, info->ai_addr, info->ai_addrlen);
  if (s != static_cast<ssize_t> (buf_slice.length))
    {
      unimplemented;
    }

  freeaddrinfo (info);
}
