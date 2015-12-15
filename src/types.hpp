#ifndef rc_src_types_hpp
#define rc_src_types_hpp

#include <stddef.h>
#include <stdint.h>

#include <map>
#include <vector>

#include "debug.hpp"

enum ExpressionKind
{
  kValue,
  kVariable,
  kType
};

namespace decl
{
class Action;
}

namespace ast
{
class ast_receiver_t;
class ast_activate_statement_t;
class Node;
class ast_call_expr_t;
class ast_function_t;
class ast_getter_t;
class ast_initializer_t;
class ast_instance_t;
class ast_method_t;
class ast_list_expr_t;

class Visitor;
class ConstVisitor;
}

namespace runtime
{
class Operation;
}

namespace Type
{
class Type;
class NamedType;
class Signature;
}

typedef std::vector<const Type::Type*> TypeList;

namespace Composition
{
class Composer;
class Instance;
}

class typed_value_t;
class MemoryModel;
class Template;
class ParameterSymbol;
class InstanceSymbol;
class ConstantSymbol;
class TypeSymbol;
class bind_t;
class Callable;
class Template;
class component_t;
class executor_base_t;
class field_t;
class Function;
class Getter;
class Initializer;
class Method;
typedef struct heap_t heap_t;
class port_t;
class reaction_t;
class scheduler_t;
class Symbol;
class value_t;
class VariableSymbol;

namespace runtime
{
class Stack;
}

// A reference is either mutable, immutable, or foreign.
// Ordered by strictness for <.
enum Mutability
{
  MUTABLE,
  IMMUTABLE,
  FOREIGN,
};

enum ReceiverAccess
{
  AccessNone,
  AccessRead,
  AccessWrite,
};

struct pull_port_t
{
  component_t* instance;
  Getter* getter;
};

enum UnaryArithmetic
{
  LogicNot,
  Negate,
};

inline const char* unary_arithmetic_symbol (UnaryArithmetic ua)
{
  switch (ua)
    {
    case LogicNot:
      return "!";
    case Negate:
      return "-";
    }

  not_reached;
}

enum BinaryArithmetic
{
  Multiply,
  Divide,
  Modulus,
  LeftShift,
  RightShift,
  BitAnd,
  BitAndNot,

  Add,
  Subtract,
  BitOr,
  BitXor,

  Equal,
  NotEqual,
  LessThan,
  LessEqual,
  MoreThan,
  MoreEqual,

  LogicOr,

  LogicAnd,
};

inline const char* binary_arithmetic_symbol (BinaryArithmetic ba)
{
  switch (ba)
    {
    case Multiply:
      return "*";
    case Divide:
      return "/";
    case Modulus:
      return "%";
    case LeftShift:
      return "<<";
    case RightShift:
      return ">>";
    case BitAnd:
      return "&";
    case BitAndNot:
      return "&^";

    case Add:
      return "+";
    case Subtract:
      return "-";
    case BitOr:
      return "|";
    case BitXor:
      return "^";

    case Equal:
      return "==";
    case NotEqual:
      return "!=";
    case LessThan:
      return "<";
    case LessEqual:
      return "<=";
    case MoreThan:
      return ">";
    case MoreEqual:
      return ">=";

    case LogicOr:
      return "||";

    case LogicAnd:
      return "&&";
    }

  not_reached;
}

#endif // rc_src_types_hpp
