#include "field.h"
#include "util.h"

struct field_t
{
  string_t name;
  type_t *type;
  ptrdiff_t offset;
};

field_t *
field_make (string_t name, type_t * type, ptrdiff_t offset)
{
  field_t *retval = xmalloc (sizeof (field_t));
  retval->name = name;
  retval->type = type;
  retval->offset = offset;
  return retval;
}

string_t
field_name (const field_t * field)
{
  return field->name;
}

type_t *
field_type (const field_t * field)
{
  return field->type;
}

ptrdiff_t
field_offset (const field_t * field)
{
  return field->offset;
}

void field_set_offset (field_t* field, ptrdiff_t offset)
{
  field->offset = offset;
}