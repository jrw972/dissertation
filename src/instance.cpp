#include "instance.hpp"
#include "util.hpp"
#include "type.hpp"
#include "debug.hpp"
#include <error.h>
#include "action.hpp"
#include "trigger.hpp"
#include <vector>
#include <set>
#include "ast.hpp"
#include "symbol.hpp"
#include "bind.hpp"
#include "field.hpp"

instance_table_t *
instance_table_make (void)
{
  instance_table_t *t = new instance_table_t;
  return t;
}

instance_t *
instance_table_insert (instance_table_t * table, instance_t* parent, const named_type_t * type, method_t* method, size_t address)
{
  instance_t *i = new instance_t (parent, address, type, method);
  table->instances.insert (std::make_pair (address, i));
  return i;
}

void
instance_table_insert_port (instance_table_t* table,
                            size_t address,
                            instance_t* output_instance)
{
  table->ports[address] = instance_table_t::PortValueType (output_instance);
}

static typed_value_t evaluate_static_rvalue (ast_t* node, size_t receiver_address);

static size_t evaluate_static_lvalue (ast_t* node, const size_t receiver_address)
{
  struct visitor : public ast_const_visitor_t
  {
    const size_t receiver_address;
    size_t address;

    visitor (size_t ra) : receiver_address (ra) { }

    void default_action (const ast_t& node)
    {
      not_reached;
    }

    void visit (const ast_select_expr_t& node)
    {
      node.base ()->accept (*this);

      if (node.field)
        {

          address += field_offset (node.field);
          return;
        }

      typed_value_t tv = node.get_type ();
      if (!tv.has_value)
        {
          error_at_line (-1, 0, node.file, node.line,
                         "non-static expression");

        }

      struct visitor : public const_type_visitor_t
      {
        const ast_select_expr_t& node;

        visitor (const ast_select_expr_t& n) : node (n) { }

        void default_action (const type_t& type)
        {
          error_at_line (-1, 0, node.file, node.line,
                         "non-static expression");
        }

        void visit (const reaction_type_t& type)
        {
          // Do nothing.  Looking for address of base.
          // This is a hack that should be solved by adding implicit deref nodes to the tree.
        }
      };
      visitor v (node);
      tv.type->accept (v);
    }

    void visit (const ast_index_expr_t& node)
    {
      node.base ()->accept (*this);
      typed_value_t tv = evaluate_static_rvalue (node.index (), receiver_address);
      int64_t index = typed_value_to_index (tv);
      const array_type_t* array_type = dynamic_cast<const array_type_t*> (ast_get_typed_value (node.base ()).type);
      assert (array_type != NULL);
      address += index * array_type->element_size ();
    }

    void visit (const ast_dereference_expr_t& node)
    {
      ast_identifier_expr_t* id = dynamic_cast<ast_identifier_expr_t*> (node.child ());
      if (id)
        {
          symbol_t* symbol = id->symbol.symbol ();
          switch (symbol_kind (symbol))
            {
            case SymbolParameter:
              if (symbol_parameter_kind (symbol) == ParameterReceiver)
                {
                  address = receiver_address;
                }
              break;
            case SymbolFunction:
            case SymbolInstance:
            case SymbolType:
            case SymbolTypedConstant:
            case SymbolVariable:
              break;
            }
        }
    }
  };
  visitor v (receiver_address);
  node->accept (v);
  return v.address;
}

static typed_value_t evaluate_static_rvalue (ast_t* node, size_t receiver_address)
{
  struct visitor : public ast_const_visitor_t
  {
    const size_t receiver_address;
    typed_value_t tv;

    visitor (size_t ra) : receiver_address (ra) { }

    void default_action (const ast_t& node)
    {
      not_reached;
    }

    void visit (const ast_select_expr_t& node)
    {
      typed_value_t t = node.get_type ();
      if (!t.has_value)
        {
          error_at_line (-1, 0, node.file, node.line,
                         "non-static expression");
        }

      tv = t;
    }

    void visit (const ast_literal_expr_t& node)
    {
      tv = node.get_type ();
    }
  };
  visitor v (receiver_address);
  node->accept (v);
  return v.tv;
}

void
instance_table_enumerate_bindings (instance_table_t * table)
{
  // For each instance.
  for (instance_table_t::InstancesType::const_iterator instance_pos = table->instances.begin (),
         instance_limit = table->instances.end ();
       instance_pos != instance_limit;
       ++instance_pos)
  {
    instance_t *instance = instance_pos->second;
    const named_type_t *type = instance->type ();
    // Enumerate the bindings.
    for (named_type_t::BindsType::const_iterator bind_pos = type->binds_begin (),
           bind_limit = type->binds_end ();
         bind_pos != bind_limit;
	 ++bind_pos)
      {
        struct visitor : public ast_const_visitor_t
        {
          instance_table_t* table;
          const size_t receiver_address;

          visitor (instance_table_t* t, size_t ra) : table (t), receiver_address (ra) { }

          void default_action (const ast_t& node)
          {
            not_reached;
          }

          void visit (const ast_bind_t& node)
          {
            node.body ()->accept (*this);
          }

          void visit (const ast_bind_statement_list_t& node)
          {
            node.visit_children (*this);
          }

          void bind (ast_t* left, ast_t* right, int64_t param = 0)
          {
            size_t port_address = evaluate_static_lvalue (left, receiver_address);
            size_t input_address = evaluate_static_lvalue (right, receiver_address);
            typed_value_t reaction_value = evaluate_static_rvalue (right, receiver_address);

            assert (dynamic_cast<const reaction_type_t*> (reaction_value.type));
            assert (reaction_value.has_value);

            instance_table_t::InputType i (table->instances[input_address], reaction_value.reaction_value, param);
            table->ports[port_address].inputs.insert (i);
            table->reverse_ports[i].insert (port_address);
          }

          void visit (const ast_bind_statement_t& node)
          {
            bind (node.left (), node.right ());
          }

          void visit (const ast_bind_param_statement_t& node)
          {
            int64_t param = typed_value_to_index (ast_get_typed_value (node.param ()));
            bind (node.left (), node.right (), param);
          }

        };
        visitor v (table, instance_pos->first);
        (*bind_pos)->node ()->accept (v);
      }
  }
}

struct action_set_t
{
  // Instances implied by the immutable phase.
  // The TriggerAction should always be READ.
  instance_set_t immutable_phase;
  // Instances implied by the mutable phase.
  // The TriggerAction may be READ or WRITE.
  instance_set_t mutable_phase;

  void merge_with_upgrade (const action_set_t& other)
  {
    // Immutable merges are straight union.
    immutable_phase.insert (other.immutable_phase.begin (), other.immutable_phase.end ());
    // Mutable phase is where the upgrade happens.
    for (instance_set_t::const_iterator pos = other.mutable_phase.begin (),
           limit = other.mutable_phase.end ();
         pos != limit;
         ++pos)
      {
        std::pair<instance_set_t::iterator, bool> x = mutable_phase.insert (*pos);
        if (!x.second && pos->second > x.first->second)
          {
            // Upgrade.
            x.first->second = pos->second;
          }
      }
  }

  bool merge_no_upgrade (const action_set_t& other)
  {
    // Immutable merges are straight union.
    immutable_phase.insert (other.immutable_phase.begin (), other.immutable_phase.end ());
    // Mutable phase is where the upgrade happens.
    for (instance_set_t::const_iterator pos = other.mutable_phase.begin (),
           limit = other.mutable_phase.end ();
         pos != limit;
         ++pos)
      {
        std::pair<instance_set_t::iterator, bool> x = mutable_phase.insert (*pos);
        if (!x.second && (pos->second == TRIGGER_WRITE || x.first->second == TRIGGER_WRITE))
          {
            // Two writers for same instance.
            return true;
          }
      }

    return false;
  }

};

/*
  Given an <instance, action> pair, compute the action_set_t checking
  for determinism.  The system is non-deterministic if two different
  actions attempt to mutate the same instance.
 */
static action_set_t
transitive_closure (const instance_table_t * table,
		    instance_t * instance,
                    const action_reaction_base_t * action,
                    size_t iota = 0)
{
  struct port_call_visitor : public ast_const_visitor_t
  {
    action_set_t set;
    const instance_table_t* table;
    const size_t address;
    size_t iota;
    bool have_port_index;
    ssize_t port_index;

    port_call_visitor (const instance_table_t* t, size_t a, size_t i) : table (t), address (a), iota (i) { }

    void default_action (const ast_t& node)
    {
      not_reached;
    }

    void visit (const ast_trigger_statement_t& node)
    {
      node.expr_list ()->accept (*this);
    }

    void visit (const ast_list_expr_t& node)
    {
      node.visit_children (*this);
    }

    void visit (const ast_port_call_expr_t& node)
    {
      size_t port = address + field_offset (node.field);
      // Find what is bound to this port.
      instance_table_t::PortsType::const_iterator port_pos = table->ports.find (port);
      assert (port_pos != table->ports.end ());

      instance_table_t::InputsType::const_iterator pos = port_pos->second.inputs.begin ();
      if (pos != table->ports.find (port)->second.inputs.end ())
        {
          assert (port_pos->second.inputs.size () == 1);
          instance_t* inst = pos->instance;
          // Merge the sets.
          if (set.merge_no_upgrade (transitive_closure (table, inst, pos->reaction)))
            {
              error_at_line (-1, 0, node.file, node.line,
                             "system is non-deterministic");

            }
        }
    }

    void visit (const ast_indexed_port_call_expr_t& node)
    {
      have_port_index = false;
      node.index ()->accept (*this);

      if (!have_port_index)
        {
          error_at_line (-1, 0, node.file, node.line,
                         "port index is not constant");

        }

      if (port_index < 0)
        {
          error_at_line (-1, 0, node.file, node.line,
                         "port index is negative");
        }

      if (static_cast<size_t> (port_index) >= node.array_type->dimension ())
        {
          error_at_line (-1, 0, node.file, node.line,
                         "port index out of bounds");
        }

      size_t port = address + field_offset (node.field) + port_index * node.array_type->element_size ();

      // Find what is bound to this port.
      instance_table_t::PortsType::const_iterator port_pos = table->ports.find (port);
      assert (port_pos != table->ports.end ());

      instance_table_t::InputsType::const_iterator pos = port_pos->second.inputs.begin ();
      if (pos != table->ports.find (port)->second.inputs.end ())
        {
          assert (port_pos->second.inputs.size () == 1);
          instance_t* inst = pos->instance;
          // Merge the sets.
          if (set.merge_no_upgrade (transitive_closure (table, inst, pos->reaction)))
            {
              error_at_line (-1, 0, node.file, node.line,
                             "system is non-deterministic");

            }
        }
    }

    void visit (const ast_identifier_expr_t& node)
    {
      string_t id = ast_get_identifier (node.child ());
      if (streq (id, enter ("IOTA")))
        {
          have_port_index = true;
          port_index = iota;
        }
    }
  };

  struct offset_visitor : public ast_const_visitor_t
  {
    const size_t receiver_address;
    size_t computed_address;

    offset_visitor (size_t ra) : receiver_address (ra), computed_address (-1) { }

    void default_action (const ast_t& node)
    {
      not_reached;
    }

    void visit (const ast_address_of_expr_t& node)
    {
      computed_address = -1;
      node.visit_children (*this);
    }

    void visit (const ast_select_expr_t& node)
    {
      computed_address = -1;
      node.base ()->accept (*this);
      if (computed_address != static_cast<size_t> (-1))
        {
          computed_address += field_offset (node.field);
        }
    }

    void visit (const ast_dereference_expr_t& node)
    {
      ast_identifier_expr_t* id = dynamic_cast<ast_identifier_expr_t*> (node.child ());
      if (id)
        {
          symbol_t* symbol = id->symbol.symbol ();
          switch (symbol_kind (symbol))
            {
            case SymbolParameter:
              if (symbol_parameter_kind (symbol) == ParameterReceiver)
                {
                  computed_address = receiver_address;
                }
              break;
            case SymbolFunction:
            case SymbolInstance:
            case SymbolType:
            case SymbolTypedConstant:
            case SymbolVariable:
              break;
            }
        }
    }

  };

  struct immutable_phase_visitor : public ast_const_visitor_t
  {
    const instance_table_t * table;
    size_t receiver_address;
    action_set_t& set;

    immutable_phase_visitor (const instance_table_t * t, size_t ra, action_set_t& s) : table (t), receiver_address (ra), set (s) { }

    void default_action (const ast_t& node)
    {
      not_reached;
    }

    void visit (const ast_action_t& node)
    {
      node.precondition ()->accept (*this);
      node.body ()->accept (*this);
    }

    void visit (const ast_dimensioned_action_t& node)
    {
      node.precondition ()->accept (*this);
      node.body ()->accept (*this);
    }

    void visit (const ast_reaction_t& node)
    {
      node.body ()->accept (*this);
    }

    void visit (const ast_dimensioned_reaction_t& node)
    {
      node.body ()->accept (*this);
    }

    void visit (const ast_list_statement_t& node)
    {
      node.visit_children (*this);
    }

    void visit (const ast_trigger_statement_t& node)
    {
      node.expr_list ()->accept (*this);
    }

    void visit (const ast_var_statement_t& node)
    {
    }

    void visit (const ast_assign_statement_t& node)
    {
      node.visit_children (*this);
    }

    void visit (const ast_change_statement_t& node)
    {
      node.expr ()->accept (*this);
      node.body ()->accept (*this);
    }

    void visit (const ast_new_expr_t& node)
    {
    }

    void visit (const ast_move_expr_t& node)
    {
      node.visit_children (*this);
    }

    void visit (const ast_merge_expr_t& node)
    {
      node.visit_children (*this);
    }

    void visit (const ast_list_expr_t& node)
    {
      node.visit_children (*this);
    }

    void visit (const ast_logic_not_expr_t& node)
    {
      node.visit_children (*this);
    }

    void visit (const ast_not_equal_expr_t& node)
    {
      node.visit_children (*this);
    }

    void visit (const ast_select_expr_t& node)
    {
      node.base ()->accept (*this);
    }

    void visit (const ast_dereference_expr_t& node)
    {
      node.visit_children (*this);
    }

    void visit (const ast_identifier_expr_t& node)
    { }

    void visit (const ast_literal_expr_t& node)
    { }

    void visit (const ast_port_call_expr_t& node)
    {
      node.args ()->accept (*this);
    }

    void visit (const ast_indexed_port_call_expr_t& node)
    {
      node.index ()->accept (*this);
      node.args ()->accept (*this);
    }

    void visit (const ast_println_statement_t& node)
    {
      node.visit_children (*this);
    }

    void visit (const ast_index_expr_t& node)
    {
      node.visit_children (*this);
    }

    void visit (const ast_logic_and_expr_t& node)
    {
      node.visit_children (*this);
    }

    void visit (const ast_add_expr_t& node)
    {
      node.visit_children (*this);
    }

    void visit (const ast_call_expr_t& node)
    {
      // See if the first argument is a component.
      if (!node.args ()->children.empty ())
        {
          offset_visitor v (receiver_address);
          node.args ()->children[0]->accept (v);
          if (v.computed_address != static_cast<size_t> (-1))
            {
              instance_table_t::InstancesType::const_iterator pos = table->instances.find (v.computed_address);
              if (pos != table->instances.end ())
                {
                  set.immutable_phase.insert (std::make_pair (pos->second, TRIGGER_READ));
                }
            }
        }
    }
  };

  action_set_t set;
  TriggerAction local_action = TRIGGER_READ;

  // For each trigger in the action.
  for (action_reaction_base_t::TriggersType::const_iterator pos = action->begin (),
         limit = action->end ();
       pos != limit;
       ++pos)
    {
      // Determine the set for this trigger.
      port_call_visitor v (table, instance->address (), iota);
      (*pos)->node.accept (v);
      // Merge the sets from this trigger.
      set.merge_with_upgrade (v.set);
      // Determine if this action mutates the receiver.
      local_action = std::max (local_action, (*pos)->action);
    }

  // Add this action.
  if (!set.mutable_phase.insert (std::make_pair (instance, local_action)).second)
    {
      if (local_action == TRIGGER_WRITE)
        {
          // We are not the first writer of this instance.
          error (-1, 0, "system is non-deterministic");
        }
    }

  // Add instances accessed in the immutable phase.
  immutable_phase_visitor v (table, instance->address (), set);
  action->node ()->accept (v);

  return set;
}

void
instance_table_analyze_composition (const instance_table_t * table)
{
  // Check that no reaction is bound more than once.
  for (instance_table_t::ReversePortsType::const_iterator pos1 = table->reverse_ports.begin (),
         limit = table->reverse_ports.end ();
       pos1 != limit;
       ++pos1)
    {
      if (pos1->second.size () > 1)
        {
          error (-1, 0, "reaction bound more than once");
        }
    }

  // For each instance.
  for (instance_table_t::InstancesType::const_iterator instance_pos = table->instances.begin (),
         instance_limit = table->instances.end ();
       instance_pos != instance_limit;
       ++instance_pos)
  {
    instance_t *instance = instance_pos->second;
    const named_type_t *type = instance->type ();
    // For each action in the type.
    for (named_type_t::ActionsType::const_iterator action_pos = type->actions_begin (),
           action_end = type->actions_end ();
         action_pos != action_end;
	 ++action_pos)
      {
        action_t* action = *action_pos;
        if (action->has_dimension ())
          {
            for (size_t iota = 0; iota != action->dimension (); ++iota)
              {
                action_set_t set = transitive_closure (table, instance, action, iota);
                // Combine the immutable and mutable sets.
                set.mutable_phase.insert (set.immutable_phase.begin (), set.immutable_phase.end ());
                instance->instance_sets.push_back (instance_t::ConcreteAction (action, set.mutable_phase, iota));
              }
          }
        else
          {
            action_set_t set = transitive_closure (table, instance, action);
            // Combine the immutable and mutable sets.
            set.mutable_phase.insert (set.immutable_phase.begin (), set.immutable_phase.end ());
            instance->instance_sets.push_back (instance_t::ConcreteAction (action, set.mutable_phase));
          }
      }
  }
}

void
instance_table_dump (const instance_table_t * table)
{
  {
    printf ("instances\n");
    printf ("%s\t%s\n", "address", "type");
    for (instance_table_t::InstancesType::const_iterator pos = table->instances.begin (),
           limit = table->instances.end ();
         pos != limit;
         ++pos)
    {
      printf ("%zd\t%s\n", pos->first, pos->second->type ()->to_string ().c_str ());
    }
    printf ("\n");
  }

  {
    printf ("ports\n");
    printf ("%s\t%s\n", "address", "inputs");
    for (instance_table_t::PortsType::const_iterator pos = table->ports.begin (),
           limit = table->ports.end ();
         pos != limit;
         ++pos)
    {
      printf ("%zd\t", pos->first);

      for (instance_table_t::InputsType::const_iterator p = pos->second.inputs.begin (),
             l = pos->second.inputs.end ();
           p != l;
           ++p)
        {
          printf ("(%zd,%s,%ld) ", p->instance->address (), get (p->reaction->name ()), p->parameter);
        }
      printf ("\n");
    }
    printf ("\n");
  }
}