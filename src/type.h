#ifndef type_h
#define type_h

#include "typed_value.h"
#include "strtab.h"
#include "types.h"

typedef enum
{
  TypeBool,
  TypeComponent,
  TypeFieldList,
  TypeForeign,
  TypeFunc,
  TypeImmutable,
  TypePointer,
  TypePort,
  TypeReaction,
  TypeSignature,
  TypeString,
  TypeStruct,
  TypeUint,
  TypeVoid,
  TypeUndefined,

  UntypedUndefined,
  UntypedNil,
  UntypedBool,
  UntypedInteger,
  UntypedString,
} TypeKind;

const char *type_to_string (const type_t * type);

/* Copy the guts of from to to and delete from. */
void type_move (type_t * to, type_t * from);

size_t type_size (const type_t * type);

const type_t *type_set_name (const type_t * type, string_t name);

string_t type_get_name (const type_t * type);

bool type_is_named (const type_t * type);

type_t *type_make_undefined (void);

bool type_is_undefined (const type_t * type);

type_t *type_make_bool (void);

type_t *type_pointer_base_type (const type_t * type);

type_t * type_make_void (void);

type_t *type_make_component (type_t * type);

type_t *type_make_pointer (type_t * base_type);

type_t *type_make_port (const type_t * signature);

bool type_is_port (const type_t * type);

bool type_is_component (const type_t * type);

action_t *type_component_add_action (type_t * type, ast_t* node);

action_t *type_component_add_reaction (type_t * component_type,
                                       ast_t* node,
				       string_t identifier,
				       type_t * signature);

method_t *type_add_method (type_t * component_type,
                       ast_t* node,
                       string_t identifier,
                       type_t * signature,
                       type_t * return_type);

TypeKind type_kind (const type_t * type);

const type_t *type_logic_not (const type_t * type);

const type_t *type_logic_and (const type_t * x, const type_t * y);

const type_t *type_logic_or (const type_t * x, const type_t * y);

type_t *type_select (const type_t * type, string_t identifier);

field_t* type_select_field (const type_t* type, string_t identifier);

action_t *type_component_get_reaction (const type_t * component_type,
				       string_t identifier);

method_t *type_get_method (const type_t * component_type,
                           string_t identifier);

bool type_is_bindable (const type_t * output, const type_t * input);

action_t **type_actions_begin (const type_t * type);

action_t **type_actions_end (const type_t * type);

action_t **type_actions_next (action_t ** action);

bool type_check_arg (const type_t * type, size_t idx, const type_t * arg);

const type_t *type_return_value (const type_t * type);

type_t *type_make_field_list (void);

field_t *type_field_list_find (const type_t * field_list,
			       string_t field_name);

void type_field_list_prepend (type_t* field_list,
                              string_t field_name, type_t* field_type);

void type_field_list_append (type_t * field_list,
			     string_t field_name, type_t * field_type);

bool type_is_pointer (const type_t * type);

bool type_convertible (const type_t * to, const type_t * from);

type_t *type_make_signature (void);

parameter_t *type_signature_find (const type_t * signature,
				  string_t parameter_name);

void type_signature_prepend (type_t * signature, string_t parameter_name,
                             type_t * parameter_type, bool is_receiver);

void type_signature_append (type_t * signature, string_t parameter_name,
			    type_t * parameter_type, bool is_receiver);

parameter_t **type_signature_begin (const type_t * signature);

parameter_t **type_signature_end (const type_t * signature);

parameter_t **type_signature_next (parameter_t **);

bool type_callable (const type_t * type);

size_t type_parameter_count (const type_t * type);

type_t *type_parameter_type (const type_t * type, size_t size);

type_t *type_return_type (const type_t * type);

bool type_is_reaction (const type_t * type);

bool type_is_signature (const type_t * type);

type_t *type_make_reaction (type_t * signature);

type_t *type_reaction_signature (const type_t * reaction);

bool type_compatible_port_reaction (const type_t * port,
				    const type_t * reaction);

size_t type_signature_arity (const type_t * signature);

type_t *type_make_func (const type_t * signature,
                          type_t * return_type);

const type_t *type_func_signature (const type_t * func);

type_t *type_func_return_type (const type_t * func);

field_t **type_field_list_begin (const type_t * field_list);

field_t **type_field_list_end (const type_t * field_list);

field_t **type_field_list_next (field_t ** field);

const type_t *type_component_field_list (const type_t * component);

void type_component_add_binding (type_t * component, binding_t * binding);

binding_t **type_component_bindings_begin (const type_t * component);

binding_t **type_component_bindings_end (const type_t * component);

binding_t **type_component_bindings_next (binding_t ** pos);

action_t **type_component_actions_begin (const type_t * component);

action_t **type_component_actions_end (const type_t * component);

action_t **type_component_actions_next (action_t ** pos);

void type_print_value (const type_t* type,
                       void* value);

type_t* type_make_string (void);

type_t* type_make_uint (void);

bool type_is_arithmetic (const type_t* type);

type_t* type_make_struct (type_t* field_list);

bool type_is_struct (const type_t* type);

type_t* type_make_untyped_nil (void);
type_t* type_make_untyped_bool (void);
type_t* type_make_untyped_string (void);
type_t* type_make_untyped_integer (void);

bool type_is_untyped (const type_t* type);

type_t* type_make_immutable (type_t* type);

bool type_is_immutable (const type_t* type);

type_t* type_immutable_base_type (const type_t* type);

bool type_leaks_mutable_pointers (const type_t* type);

bool type_leaks_mutable_or_immutable_pointers (const type_t* type);

type_t* type_make_foreign (type_t* type);

bool type_is_foreign (const type_t* type);

bool type_contains_foreign_pointers (const type_t* type);

#endif /* type_h */
