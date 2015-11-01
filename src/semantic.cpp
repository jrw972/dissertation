#include "semantic.hpp"
#include "debug.hpp"
#include "ast.hpp"
#include <error.h>
#include "action.hpp"
#include "Type.hpp"
#include "parameter.hpp"
#include "instance.hpp"
#include "Activation.hpp"
#include "field.hpp"
#include "Symbol.hpp"
#include "MemoryModel.hpp"
#include "bind.hpp"
#include "Callable.hpp"

// TODO:  Replace interacting with type_t* with typed_value_t.

static void
allocate_symbol (MemoryModel& memory_model,
                 Symbol* symbol)
{
  struct visitor : public SymbolVisitor
  {
    MemoryModel& memory_model;

    visitor (MemoryModel& mm) : memory_model (mm) { }

    void defineAction (Symbol& symbol)
    {
      not_reached;
    }

    void visit (ParameterSymbol& symbol)
    {
      switch (symbol.kind)
        {
        case ParameterSymbol::Ordinary:
        case ParameterSymbol::Receiver:
        case ParameterSymbol::Return:
        {
          const Type::Type* type = symbol.value.type;
          memory_model.ArgumentsPush (type->Size ());
          static_cast<Symbol&> (symbol).offset (memory_model.ArgumentsOffset ());
        }
        break;
        case ParameterSymbol::ReceiverDuplicate:
        case ParameterSymbol::OrdinaryDuplicate:
          break;
        }
    }

    void visit (TypedConstantSymbol& symbol)
    {
      // No need to allocate.
    }

    void visit (VariableSymbol& symbol)
    {
      const Type::Type* type = symbol.value.type;
      static_cast<Symbol&>(symbol).offset (memory_model.LocalsOffset ());
      memory_model.LocalsPush (type->Size ());
    }

    void visit (HiddenSymbol& symbol)
    {
      // Do nothing.
    }
  };

  visitor v (memory_model);
  symbol->accept (v);
}

static void
allocate_symtab (ast_t* node, MemoryModel& memory_model)
{
  // Allocate the parameters.
  for (ast_t::SymbolsType::const_iterator pos = node->symbols.begin (), limit = node->symbols.end ();
       pos != limit;
       ++pos)
    {
      allocate_symbol (memory_model, *pos);
    }
}

static void
allocate_statement_stack_variables (ast_t* node, MemoryModel& memory_model)
{
  struct visitor : public ast_visitor_t
  {
    MemoryModel& memory_model;

    visitor (MemoryModel& m) : memory_model (m) { }

    void default_action (ast_t& node)
    {
      ast_not_reached (node);
    }

    void visit (ast_const_t& node)
    {
      // Do nothing.
    }

    void visit (ast_empty_statement_t& node)
    { }

    void visit (ast_for_iota_statement_t& node)
    {
      ptrdiff_t offset_before = memory_model.LocalsOffset ();
      allocate_symtab (&node, memory_model);
      ptrdiff_t offset_after = memory_model.LocalsOffset ();
      allocate_statement_stack_variables (node.body (), memory_model);
      memory_model.LocalsPop (offset_after - offset_before);
      assert (memory_model.LocalsOffset () == offset_before);
    }

    void visit (ast_bind_push_port_statement_t& node)
    {
      // Do nothing.
    }

    void visit (ast_bind_push_port_param_statement_t& node)
    {
      // Do nothing.
    }

    void visit (ast_bind_pull_port_statement_t& node)
    {
      // Do nothing.
    }

    void visit (ast_assign_statement_t& node)
    {
      // Do nothing.
    }

    void visit (ast_change_statement_t& node)
    {
      ptrdiff_t offset_before = memory_model.LocalsOffset ();
      allocate_symtab (&node, memory_model);
      ptrdiff_t offset_after = memory_model.LocalsOffset ();
      allocate_statement_stack_variables (node.body (), memory_model);
      memory_model.LocalsPop (offset_after - offset_before);
      assert (memory_model.LocalsOffset () == offset_before);
    }

    void visit (ast_expression_statement_t& node)
    {
      // Do nothing.
    }

    void visit (ast_if_statement_t& node)
    {
      allocate_statement_stack_variables (node.true_branch (), memory_model);
      allocate_statement_stack_variables (node.false_branch (), memory_model);
    }

    void visit (ast_while_statement_t& node)
    {
      allocate_statement_stack_variables (node.body (), memory_model);
    }

    void visit (ast_add_assign_statement_t& node)
    {
      // Do nothing.
    }

    void visit (ast_subtract_assign_statement_t& node)
    {
      // Do nothing.
    }

    void visit (ast_list_statement_t& node)
    {
      ptrdiff_t offset_before = memory_model.LocalsOffset ();
      allocate_symtab (&node, memory_model);
      ptrdiff_t offset_after = memory_model.LocalsOffset ();
      for (ast_t::const_iterator pos = node.begin (), limit = node.end ();
           pos != limit;
           ++pos)
        {
          allocate_statement_stack_variables (*pos, memory_model);
        }
      memory_model.LocalsPop (offset_after - offset_before);
      assert (memory_model.LocalsOffset () == offset_before);
    }

    void visit (ast_return_statement_t& node)
    {
      // Do nothing.
    }

    void visit (ast_increment_statement_t& node)
    {
      // Do nothing.
    }

    void visit (ast_activate_statement_t& node)
    {
      ptrdiff_t offset_before = memory_model.LocalsOffset ();
      allocate_symtab (&node, memory_model);
      ptrdiff_t offset_after = memory_model.LocalsOffset ();
      allocate_statement_stack_variables (node.body (), memory_model);
      memory_model.LocalsPop (offset_after - offset_before);
      assert (memory_model.LocalsOffset () == offset_before);
    }

    void visit (ast_var_statement_t& node)
    {
      // Do nothing.  The variables are allocated in the StmtList containing this.
    }
  };
  visitor v (memory_model);
  node->accept (v);
}

static void
allocate_parameter (MemoryModel& memory_model,
                    ast_t::SymbolsType::const_iterator pos,
                    ast_t::SymbolsType::const_iterator limit)
{
  if (pos != limit)
    {
      allocate_parameter (memory_model, pos + 1, limit);
      allocate_symbol (memory_model, *pos);
    }
}

void
allocate_stack_variables (ast_t* node)
{
  struct visitor : public ast_visitor_t
  {
    void visit (ast_action_t& node)
    {
      node.action->memory_model = allocate_stack_variables_helper (&node, node.body ());
    }

    void visit (ast_dimensioned_action_t& node)
    {
      node.action->memory_model = allocate_stack_variables_helper (&node, node.body ());
    }

    void visit (ast_bind_t& node)
    {
      node.bind->memory_model = allocate_stack_variables_helper (&node, node.body ());
    }

    void visit (ast_function_t& node)
    {
      node.function->memoryModel = allocate_stack_variables_helper (&node, node.body ());
    }

    void visit (ast_method_t& node)
    {
      node.method->memoryModel = allocate_stack_variables_helper (&node, node.body ());
    }

    void visit (ast_initializer_t& node)
    {
      node.initializer->memoryModel = allocate_stack_variables_helper (&node, node.body ());
    }

    void visit (ast_getter_t& node)
    {
      node.getter->memoryModel = allocate_stack_variables_helper (&node, node.body ());
    }

    void visit (ast_reaction_t& node)
    {
      node.reaction->memory_model = allocate_stack_variables_helper (&node, node.body ());
    }

    void visit (ast_dimensioned_reaction_t& node)
    {
      node.reaction->memory_model = allocate_stack_variables_helper (&node, node.body ());
    }

    void visit (ast_top_level_list_t& node)
    {
      node.visit_children (*this);
    }

    // Return the size of the locals.
    MemoryModel
    allocate_stack_variables_helper (ast_t* node,
                                     ast_t* child)
    {
      MemoryModel memory_model;
      // Allocate the parameters.
      ast_t::SymbolsType::const_iterator pos = node->symbols.begin ();
      ast_t::SymbolsType::const_iterator limit = node->symbols.end ();
      allocate_parameter (memory_model, pos, limit);
      // Allocate the locals.
      allocate_statement_stack_variables (child, memory_model);
      assert (memory_model.LocalsEmpty ());
      return memory_model;
    }

  };

  visitor v;
  node->accept (v);
}
