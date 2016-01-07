#ifndef RC_SRC_CALLABLE_HPP
#define RC_SRC_CALLABLE_HPP

#include "types.hpp"
#include "type.hpp"
#include "memory_model.hpp"
#include "symbol.hpp"

namespace decl
{

/*
 * Base class for things that can be called.
 */

class Callable
{
public:
  virtual ~Callable () { }
  virtual void call (runtime::executor_base_t& exec) const = 0;
  virtual const type::Signature* signature () const = 0;
  virtual const type::Type* type () const = 0;
  virtual size_t return_size () const = 0;
  virtual size_t receiver_size () const = 0;
  virtual size_t arguments_size () const = 0;
  virtual size_t locals_size () const = 0;
  virtual void check_types (ast::List* args) const;
  virtual void check_references (ast::List* args) const;
  virtual void check_mutability (ast::List* args) const;
  virtual void compute_receiver_access (ast::List* args, ReceiverAccess& receiver_access, bool& flag) const;
};
/*
 * TODO:  I debate whether or not the return symbols should be recorded here.
 */

struct Function : public Callable, public decl::Symbol
{
  // TODO:  Remove duplication with Symbol.
  Function (ast::Function& node, const type::Function* type);

  // Callable
  virtual void call (runtime::executor_base_t& exec) const;
  virtual const type::Type* type () const
  {
    return functionType_;
  }

  // Symbol
  virtual void accept (decl::SymbolVisitor& visitor);
  virtual void accept (decl::ConstSymbolVisitor& visitor) const;
  virtual const char* kindString () const
  {
    return "Function";
  }

  ast::Function& node;
  runtime::MemoryModel memoryModel;

  virtual size_t return_size () const
  {
    return functionType_->GetReturnType ()->Size ();
  }
  virtual size_t receiver_size () const
  {
    return 0;
  }
  virtual size_t arguments_size () const
  {
    return functionType_->GetSignature ()->Size ();
  }
  virtual size_t locals_size () const
  {
    return memoryModel.LocalsSize ();
  }
  virtual const type::Signature* signature () const
  {
    return functionType_->GetSignature ();
  }
  ParameterSymbol* return_parameter () const
  {
    return functionType_->GetReturnParameter ();
  }

private:
  const type::Function* functionType_;
};

struct Method : public Callable
{
  Method (ast::Method* n,
          const std::string& na,
          const type::Method* method_type_)
    : node (n)
    , name (na)
    , methodType (method_type_)
    , returnSize (method_type_->return_type ()->Size ())
  { }

  virtual void call (runtime::executor_base_t& exec) const;
  virtual const type::Signature* signature () const
  {
    return methodType->signature;
  }
  decl::ParameterSymbol* return_parameter () const
  {
    return methodType->return_parameter;
  }
  decl::ParameterSymbol* receiver_parameter () const
  {
    return methodType->receiver_parameter;
  }

  virtual const type::Type* type () const
  {
    return methodType;
  }

  virtual size_t return_size () const
  {
    return methodType->return_type ()->Size ();
  }
  virtual size_t receiver_size () const
  {
    return methodType->receiver_type ()->Size ();
  }
  virtual size_t arguments_size () const
  {
    return methodType->signature->Size ();
  }
  virtual size_t locals_size () const
  {
    return memoryModel.LocalsSize ();
  }

  ast::Method* const node;
  std::string const name;
  const type::Method * const methodType;
  size_t const returnSize;
  runtime::MemoryModel memoryModel;
};

struct Initializer : public Callable
{
  Initializer (ast::Initializer* n,
               const std::string& na,
               const type::Method* initializer_type_)
    : node (n)
    , name (na)
    , initializerType (initializer_type_)
    , returnSize (initializer_type_->return_type ()->Size ())
  { }

  virtual void call (runtime::executor_base_t& exec) const;
  virtual const type::Type* type () const
  {
    return initializerType;
  }

  virtual size_t return_size () const
  {
    return returnSize;
  }
  virtual size_t receiver_size () const
  {
    return initializerType->receiver_type ()->Size ();
  }
  virtual size_t arguments_size () const
  {
    return initializerType->signature->Size ();
  }
  virtual size_t locals_size () const
  {
    return memoryModel.LocalsSize ();
  }
  virtual const type::Signature* signature () const
  {
    return initializerType->signature;
  }
  decl::ParameterSymbol* return_parameter () const
  {
    return initializerType->return_parameter;
  }
  decl::ParameterSymbol* receiver_parameter () const
  {
    return initializerType->receiver_parameter;
  }
  ast::Initializer* const node;
  std::string const name;
  const type::Method * const initializerType;
  size_t const returnSize;
  runtime::MemoryModel memoryModel;
};

struct Getter : public Callable
{
  Getter (ast::Getter* n,
          const std::string& na,
          const type::Method* getter_type_)
    : node (n)
    , name (na)
    , getterType (getter_type_)
    , returnSize (getter_type_->return_type ()->Size ())
  { }

  virtual void call (runtime::executor_base_t& exec) const;

  virtual const type::Type* type () const
  {
    return getterType;
  }

  virtual size_t return_size () const
  {
    return getterType->return_type ()->Size ();
  }
  virtual size_t receiver_size () const
  {
    return getterType->receiver_type ()->Size ();
  }
  virtual size_t arguments_size () const
  {
    return getterType->signature->Size ();
  }
  virtual size_t locals_size () const
  {
    return memoryModel.LocalsSize ();
  }
  virtual const type::Signature* signature () const
  {
    return getterType->signature;
  }

  decl::ParameterSymbol* return_parameter () const
  {
    return getterType->return_parameter;
  }
  decl::ParameterSymbol* receiver_parameter () const
  {
    return getterType->receiver_parameter;
  }

  ast::Getter* const node;
  std::string const name;
  const type::Method * const getterType;
  size_t const returnSize;
  ReceiverAccess immutable_phase_access;
  runtime::MemoryModel memoryModel;
};

}

#endif // RC_SRC_CALLABLE_HPP
