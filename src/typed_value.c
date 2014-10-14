#include "typed_value.h"
#include <stdlib.h>
#include "debug.h"
#include "symtab.h"
#include "type.h"

bool
typed_value_is_undefined (typed_value_t t)
{
  return t.type == NULL;
}

typed_value_t
typed_value_make_bool (type_t * type, bool v)
{
typed_value_t retval = { type: type, bool_value:v };
  return retval;
}

type_t *
typed_value_get_type (typed_value_t t)
{
  return t.type;
}

typed_value_t
typed_value_from_untyped (untyped_value_t u, type_t * t)
{
  assert (type_can_represent (t, u));
  switch (type_kind (t))
    {
    case TypeUndefined:
      not_reached;
    case TypeVoid:
      unimplemented;
    case TypeBool:
      return typed_value_make_bool (t, untyped_value_bool_value (u));
    case TypeComponent:
      unimplemented;
    case TypePointer:
      unimplemented;
    case TypePort:
      unimplemented;
    case TypeReaction:
      unimplemented;
    case TypeFieldList:
      unimplemented;
    case TypeSignature:
      unimplemented;
    }
  bug ("unhandled case");
}
