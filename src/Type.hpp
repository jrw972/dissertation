#ifndef Type_hpp
#define Type_hpp

#include "types.hpp"
#include "debug.hpp"
#include "util.hpp"
#include <vector>
#include <string>
#include <sstream>
#include <cstring>

#define type_not_reached(type) do { std::cerr << '\n' << type << std::endl; not_reached; } while (0);

#define StringReturner(name, string) struct name { const char* operator() () const { return string; } }

namespace Type
{

  class Visitor;
  class Pointer;
  class Slice;
  class Array;
  class Heap;

  typedef int64_t IntValueType;

  struct Type
  {
    enum TypeLevel
    {
      UNTYPED, // Represent untyped literals.
      UNNAMED, // Types constructed through type literals and the "builtin" types.
      NAMED,   // Types named with a type declaration.
    };
    Type () : pointer_ (NULL), slice_ (NULL), heap_ (NULL) { }
    virtual ~Type () { }
    virtual void Accept (Visitor& visitor) const = 0;
    virtual std::string ToString () const = 0;
    virtual size_t Alignment () const = 0;
    virtual size_t Size () const = 0;
    // When give the choice between two types, use the one with lower level.
    virtual TypeLevel Level () const = 0;
    virtual const Type* UnderlyingType () const
    {
      return this;
    }
    virtual const Type* DefaultType () const
    {
      return this;
    }
    bool IsUntyped () const
    {
      return Level () == UNTYPED;
    }
    virtual bool IsNumeric () const
    {
      return false;
    }
    virtual bool IsFloatingPoint () const
    {
      return false;
    }
    virtual bool IsInteger () const
    {
      return false;
    }
    virtual bool IsString () const;
    virtual bool IsComplex () const;
    virtual bool IsSliceOfBytes () const;
    virtual bool IsSliceOfRunes () const;
    const Pointer* GetPointer () const;
    const Slice* GetSlice () const;
    const Array* GetArray (IntValueType dimension) const;
    const Heap* GetHeap () const;
  private:
    const Pointer* pointer_;
    const Slice* slice_;
    typedef std::map<IntValueType, const Array*> ArraysType;
    ArraysType arrays_;
    const Heap* heap_;
  };

  std::ostream&
  operator<< (std::ostream& o, const Type& type);

  class NamedType : public Type
  {
  public:
    typedef std::vector<action_t*> ActionsType;
    typedef std::vector<reaction_t*> ReactionsType;
    typedef std::vector<bind_t*> BindsType;

    NamedType (const std::string& name)
      : name_ (name)
      , underlyingType_ (NULL)
    { }

    NamedType (const std::string& name,
               const Type* underlyingType);

    void Accept (Visitor& visitor) const;
    std::string ToString () const
    {
      return name_;
    }
    void UnderlyingType (const Type* u)
    {
      underlyingType_ = u;
    }
    const Type* UnderlyingType () const
    {
      return underlyingType_;
    }
    virtual size_t Alignment () const
    {
      return underlyingType_->Alignment ();
    }
    virtual size_t Size () const
    {
      return underlyingType_->Size ();
    }
    virtual TypeLevel Level () const
    {
      return NAMED;
    }
    virtual bool IsNumeric () const
    {
      return underlyingType_->IsNumeric ();
    }
    virtual bool IsFloatingPoint () const
    {
      return underlyingType_->IsFloatingPoint ();
    }
    virtual bool IsInteger () const
    {
      return underlyingType_->IsInteger ();
    }
    virtual bool IsString () const
    {
      return underlyingType_->IsString ();
    }
    void Add (Method* method)
    {
      methods_.push_back (method);
    }
    Method* GetMethod (const std::string& identifier) const;
    void Add (Initializer* initializer)
    {
      initializers_.push_back (initializer);
    }
    Initializer* GetInitializer (const std::string& identifier) const;
    void Add (Getter* getter)
    {
      getters_.push_back (getter);
    }
    Getter* GetGetter (const std::string& identifier) const;
    void Add (action_t* action)
    {
      actions_.push_back (action);
    }
    action_t* GetAction (const std::string& identifier) const;
    ActionsType::const_iterator ActionsBegin () const
    {
      return actions_.begin ();
    }
    ActionsType::const_iterator ActionsEnd () const
    {
      return actions_.end ();
    }
    void Add (reaction_t* reaction)
    {
      reactions_.push_back (reaction);
    }
    reaction_t* GetReaction (const std::string& identifier) const;
    void Add (bind_t* bind)
    {
      binds_.push_back (bind);
    }
    bind_t* GetBind (const std::string& identifier) const;
    BindsType::const_iterator BindsBegin () const
    {
      return binds_.begin ();
    }
    BindsType::const_iterator BindsEnd () const
    {
      return binds_.end ();
    }

  private:
    std::string const name_;
    const Type* underlyingType_;
    std::vector<Method*> methods_;
    std::vector<Initializer*> initializers_;
    std::vector<Getter*> getters_;
    ActionsType actions_;
    ReactionsType reactions_;
    BindsType binds_;
  };

  class Void : public Type
  {
  public:
    void Accept (Visitor& visitor) const;
    std::string ToString () const
    {
      return "<void>";
    }
    size_t Alignment () const
    {
      not_reached;
    }
    size_t Size () const
    {
      return 0;
    }
    virtual TypeLevel Level () const
    {
      return UNNAMED;
    }
    static const Void* Instance ();
  private:
    Void () { }
  };

  template <typename T, typename S, bool Numeric, bool FloatingPoint, bool Integer>
  class Scalar : public Type
  {
  public:
    typedef T ValueType;
    void Accept (Visitor& visitor) const;
    std::string ToString () const
    {
      return S () ();
    }
    size_t Alignment () const
    {
      return sizeof (T);
    }
    size_t Size () const
    {
      return sizeof (T);
    }
    virtual TypeLevel Level () const
    {
      return UNNAMED;
    }
    virtual bool IsNumeric () const
    {
      return Numeric;
    }
    virtual bool IsFloatingPoint () const
    {
      return FloatingPoint;
    }
    virtual bool IsInteger () const
    {
      return Integer;
    }
    static const Scalar<T, S, Numeric, FloatingPoint, Integer>* Instance ()
    {
      static Scalar<T, S, Numeric, FloatingPoint, Integer>* instance_ = new Scalar<T, S, Numeric, FloatingPoint, Integer> ();
      return instance_;
    }
  private:
    Scalar<T, S, Numeric, FloatingPoint, Integer> () { }
  };

  StringReturner(EnumString, "<enum>");
  typedef Scalar<size_t, EnumString, false, false, false> Enum;

  StringReturner(BoolString, "<bool>");
  typedef Scalar<bool, BoolString, false, false, false> Bool;

  StringReturner(Uint8String, "<uint8>");
  typedef Scalar<uint8_t, Uint8String, true, false, true> Uint8;

  StringReturner(Uint16String, "<uint16>");
  typedef Scalar<uint16_t, Uint16String, true, false, true> Uint16;

  StringReturner(Uint32String, "<uint32>");
  typedef Scalar<uint32_t, Uint32String, true, false, true> Uint32;

  StringReturner(Uint64String, "<uint64>");
  typedef Scalar<uint64_t, Uint64String, true, false, true> Uint64;

  StringReturner(Int8String, "<int8>");
  typedef Scalar<int8_t, Int8String, true, false, true> Int8;

  StringReturner(Int16String, "<int16>");
  typedef Scalar<int16_t, Int16String, true, false, true> Int16;

  StringReturner(Int32String, "<int32>");
  typedef Scalar<int32_t, Int32String, true, false, true> Int32;

  StringReturner(Int64String, "<int64>");
  typedef Scalar<int64_t, Int64String, true, false, true> Int64;

  StringReturner(Float32String, "<float32>");
  typedef Scalar<float, Float32String, true, true, false> Float32;

  StringReturner(Float64String, "<float64>");
  typedef Scalar<double, Float64String, true, true, false> Float64;

  StringReturner(Complex64String, "<complex64>");
  struct C64
  {
    float real;
    float imag;

    bool operator== (const C64& other) const
    {
      return this->real == other.real && this->imag == other.imag;
    }

    operator double() const
    {
      unimplemented;
    }

    C64& operator= (const Int64::ValueType& x)
    {
      this->real = x;
      this->imag = 0;
      return *this;
    }
  };
  typedef Scalar<C64, Complex64String, true, false, false> Complex64;

  StringReturner(Complex128String, "<complex128>");
  struct C128
  {
    double real;
    double imag;

    bool operator== (const C128& other) const
    {
      return this->real == other.real && this->imag == other.imag;
    }

    operator double() const
    {
      unimplemented;
    }

    C128& operator= (const Int64::ValueType& x)
    {
      this->real = x;
      this->imag = 0;
      return *this;
    }
  };
  typedef Scalar<C128, Complex128String, true, false, false> Complex128;

  StringReturner(UintString, "<uint>");
  typedef Scalar<uint64_t, UintString, true, false, true> Uint;

  StringReturner(IntString, "<int>");
  typedef Scalar<IntValueType, IntString, true, false, true> Int;

  StringReturner(UintptrString, "<uintptr>");
  typedef Scalar<ptrdiff_t, UintptrString, true, false, true> Uintptr;

  StringReturner(StringUString, "<string>");
  struct StringRep
  {
    void* ptr;
    size_t length;

    bool operator== (const StringRep& other) const
    {
      if (this->ptr == other.ptr &&
          this->length == other.length)
        {
          return true;
        }

      if (this->length != other.length)
        {
          return false;
        }

      return memcmp (this->ptr, other.ptr, this->length) == 0;
    }

    bool operator< (const StringRep& other) const
    {
      int x = memcmp (this->ptr, other.ptr, std::min (this->length, other.length));
      if (x < 0)
        {
          return true;
        }
      else if (x > 0)
        {
          return false;
        }
      else
        {
          return this->length < other.length;
        }
    }
  };
  typedef Scalar<StringRep, StringUString, false, false, false> StringU;

  // Helper class for types that have a base type.
  class BaseType
  {
  public:
    BaseType (const Type* base) : base_ (base) { }
    const Type* Base () const
    {
      return base_;
    }
  protected:
    const Type* const base_;
  };

  class Pointer : public Type, public BaseType
  {
  public:
    typedef void* ValueType;
    void Accept (Visitor& visitor) const;
    std::string ToString () const
    {
      return "*" + base_->ToString ();
    }
    size_t Alignment () const
    {
      return sizeof (ValueType);
    }
    size_t Size () const
    {
      return sizeof (ValueType);
    }
    virtual TypeLevel Level () const
    {
      return UNNAMED;
    }
  private:
    friend class Type;
    Pointer (const Type* base) : BaseType (base) { }
  };

  class Slice : public Type, public BaseType
  {
  public:
    struct ValueType
    {
      void* ptr;
      size_t length;
      size_t capacity;
    };
    virtual void Accept (Visitor& visitor) const;
    virtual std::string ToString () const
    {
      return "[]" + base_->ToString ();
    }
    virtual size_t Alignment () const
    {
      return sizeof (void*);
    }
    virtual size_t Size () const
    {
      return sizeof (ValueType);
    }
    virtual TypeLevel Level () const
    {
      return UNNAMED;
    }
  private:
    friend class Type;
    Slice (const Type* base) : BaseType (base) { }
  };

  class Array : public Type, public BaseType
  {
  public:
    void Accept (Visitor& visitor) const;
    std::string ToString () const;
    size_t Alignment () const
    {
      return base_->Alignment ();
    }
    size_t Size () const
    {
      return ElementSize () * dimension;
    }
    virtual TypeLevel Level () const
    {
      return UNNAMED;
    }
    size_t ElementSize () const
    {
      return util::AlignUp (base_->Size (), base_->Alignment ());
    }
    const Int::ValueType dimension;
  private:
    friend class Type;
    Array (Int::ValueType d, const Type* base) : BaseType (base), dimension (d) { }
  };

  struct Heap : public Type, public BaseType
  {
    void Accept (Visitor& visitor) const;
    std::string ToString () const
    {
      return "heap " + base_->ToString ();
    }
    size_t Alignment () const
    {
      not_reached;
    }
    size_t Size () const
    {
      not_reached;
    }
    virtual TypeLevel Level () const
    {
      return UNNAMED;
    }
  private:
    friend class Type;
    Heap (const Type* base) : BaseType (base) { }
  };

  class Struct : public Type
  {
  public:
    typedef std::vector<field_t*> FieldsType;
    typedef FieldsType::const_iterator const_iterator;
    Struct (bool insert_runtime = false);
    void Accept (Visitor& visitor) const;
    std::string ToString () const
    {
      unimplemented;
    }
    size_t Alignment () const
    {
      return alignment_;
    }
    size_t Size () const
    {
      return offset_;
    }
    virtual TypeLevel Level () const
    {
      return UNNAMED;
    }
    const_iterator Begin () const
    {
      return fields_.begin ();
    }
    const_iterator End () const
    {
      return fields_.end ();
    }
    void Append (const std::string& field_name, const Type* field_type);
    field_t* Find (const std::string& name) const;
  private:
    FieldsType fields_;
    ptrdiff_t offset_;
    size_t alignment_;
  };

  struct Component : public Struct
  {
    Component () : Struct (true) { }
    void Accept (Visitor& visitor) const;
    std::string ToString () const
    {
      unimplemented;
    }
  };

  class Signature : public Type
  {
  public:
    typedef std::vector<const parameter_t*> ParametersType;
    typedef ParametersType::const_iterator const_iterator;
    void Accept (Visitor& visitor) const;
    std::string ToString () const;
    size_t Alignment () const
    {
      not_reached;
    }
    size_t Size () const
    {
      not_reached;
    }
    virtual TypeLevel Level () const
    {
      return UNNAMED;
    }
    size_t Arity () const
    {
      return parameters_.size ();
    }
    const parameter_t* At (size_t idx) const
    {
      return parameters_.at (idx);
    }
    const_iterator Begin () const
    {
      return parameters_.begin ();
    }
    const_iterator End () const
    {
      return parameters_.end ();
    }
    const parameter_t * Find (const std::string& name) const;
    Signature* Append (const parameter_t* p)
    {
      parameters_.push_back (p);
      return this;
    }
  private:
    ParametersType parameters_;
  };

  class Function : public Type
  {
  public:
    enum Kind
    {
      FUNCTION,
      PUSH_PORT,
      PULL_PORT
    };
    Function (Kind k,
              const Signature * signature,
              const parameter_t * return_parameter)
      : kind (k)
      , signature_ (signature)
      , return_parameter_ (return_parameter)
    { }
    void Accept (Visitor& visitor) const;
    std::string ToString () const;
    size_t Alignment () const
    {
      return sizeof (void*);
    }
    size_t Size () const
    {
      return kind == PULL_PORT ? sizeof (pull_port_t) : sizeof (void*);
    }
    virtual TypeLevel Level () const
    {
      return UNNAMED;
    }
    const Signature* GetSignature () const
    {
      return signature_;
    }
    const parameter_t* GetParameter (const std::string& name) const
    {
      return signature_->Find (name);
    }
    const parameter_t* GetReturnParameter () const
    {
      return return_parameter_;
    }
    const Type* GetReturnType () const;
    Kind const kind;
  private:
    const Signature* const signature_;
    const parameter_t* const return_parameter_;
  };

  class Method : public Type
  {
  public:
    enum Kind
    {
      METHOD,
      INITIALIZER,
      GETTER,
      REACTION,
    };
    Method (Kind k,
            const NamedType* named_type_,
            const parameter_t* this_parameter_,
            const Signature * signature_,
            const parameter_t* return_parameter_);
    void Accept (Visitor& visitor) const;
    std::string ToString () const;
    size_t Alignment () const
    {
      return sizeof (void*);
    }
    size_t Size () const
    {
      return sizeof (void*);
    }
    virtual TypeLevel Level () const
    {
      return UNNAMED;
    }
    Kind const kind;
    const NamedType* const named_type;
    const Type* const receiver_type;
    const parameter_t* const this_parameter;
    const Function* const function_type;
    const Signature* const signature;
    const parameter_t* const return_parameter;
    const Type* return_type () const;
  private:
    static Function* make_function_type (const parameter_t* this_parameter,
                                         const Signature* signature,
                                         const parameter_t* return_parameter);
  };

  class Untyped : public Type
  {
  public:
    size_t Alignment () const
    {
      not_reached;
    }
    size_t Size () const
    {
      not_reached;
    }
    virtual TypeLevel Level () const
    {
      return UNTYPED;
    }
  };

  class Nil : public Untyped
  {
  public:
    void Accept (Visitor& visitor) const;
    std::string ToString () const
    {
      return "<<nil>>";
    }
    static const Nil* Instance ();
  private:
    Nil () { }
  };

  class Boolean : public Untyped
  {
  public:
    typedef bool ValueType;
    virtual const Type* DefaultType () const;
    void Accept (Visitor& visitor) const;
    std::string ToString () const
    {
      return "<<boolean>>";
    }
    static const Boolean* Instance ();
  private:
    Boolean () { }
  };

  class Rune : public Untyped
  {
  public:
    typedef int32_t ValueType;
    virtual const Type* DefaultType () const;
    virtual bool IsNumeric () const
    {
      return true;
    }
    virtual bool IsInteger () const
    {
      return true;
    }
    void Accept (Visitor& visitor) const;
    std::string ToString () const
    {
      return "<<rune>>";
    }
    static const Rune* Instance ();
  private:
    Rune () { }
  };

  class Integer : public Untyped
  {
  public:
    typedef long long ValueType;
    virtual const Type* DefaultType () const;
    virtual bool IsNumeric () const
    {
      return true;
    }
    virtual bool IsInteger () const
    {
      return true;
    }
    void Accept (Visitor& visitor) const;
    std::string ToString () const
    {
      return "<<integer>>";
    }
    static const Integer* Instance ();
  private:
    Integer () { }
  };

  class Float : public Untyped
  {
  public:
    typedef double ValueType;
    virtual const Type* DefaultType () const;
    virtual bool IsNumeric () const
    {
      return true;
    }
    virtual bool IsFloatingPoint () const
    {
      return true;
    }
    void Accept (Visitor& visitor) const;
    std::string ToString () const
    {
      return "<<float>>";
    }
    static const Float* Instance ();
  private:
    Float () { }
  };

  class Complex : public Untyped
  {
  public:
    struct ValueType
    {
      double real;
      double imag;
      static ValueType make (double r, double i)
      {
        ValueType retval;
        retval.real = r;
        retval.imag = i;
        return retval;
      }

      bool operator== (const ValueType& other) const
      {
        return this->real == other.real && this->imag == other.imag;
      }

      ValueType& operator= (const Int::ValueType& x)
      {
        this->real = x;
        this->imag = 0;
        return *this;
      }

      ValueType& operator= (const Complex64::ValueType& x)
      {
        this->real = x.real;
        this->imag = x.imag;
        return *this;
      }

      ValueType& operator= (const Complex128::ValueType& x)
      {
        this->real = x.real;
        this->imag = x.imag;
        return *this;
      }

      operator double() const
      {
        return this->real;
      }
    };
    virtual const Type* DefaultType () const;
    virtual bool IsNumeric () const
    {
      return true;
    }
    void Accept (Visitor& visitor) const;
    std::string ToString () const
    {
      return "<<complex>>";
    }
    static const Complex* Instance ();
  private:
    Complex () { }
  };

  class String : public Untyped
  {
  public:
    typedef StringRep ValueType;
    virtual const Type* DefaultType () const;
    void Accept (Visitor& visitor) const;
    std::string ToString () const
    {
      return "<<string>>";
    }
    static const String* Instance ();
  private:
    String () { }
  };

  class Template : public Type
  {
  public:
    void Accept (Visitor& visitor) const;
    virtual std::string ToString () const
    {
      return "<<template>>";
    }
    virtual size_t Alignment () const
    {
      not_reached;
    }
    virtual size_t Size () const
    {
      not_reached;
    }
    virtual TypeLevel Level () const
    {
      return UNTYPED;
    }
  };

  struct FileDescriptor : public Type
  {
    void Accept (Visitor& visitor) const;
    std::string ToString () const
    {
      return "<FileDescriptor>";
    }
    size_t Alignment () const
    {
      return sizeof (void*);
    }
    size_t Size () const
    {
      return sizeof (void*);
    }
    virtual TypeLevel Level () const
    {
      return UNNAMED;
    }
    static const FileDescriptor* Instance ();
  private:
    FileDescriptor () { }
  };

  struct Visitor
  {
    virtual ~Visitor () { }
    virtual void visit (const Array& type) = 0;
    virtual void visit (const Slice& type) = 0;
    virtual void visit (const Bool& type) = 0;
    virtual void visit (const Component& type) = 0;
    virtual void visit (const Enum& type) = 0;
    virtual void visit (const Function& type) = 0;
    virtual void visit (const Method& type) = 0;
    virtual void visit (const Heap& type) = 0;
    virtual void visit (const FileDescriptor& type) = 0;
    virtual void visit (const NamedType& type) = 0;
    virtual void visit (const Pointer& type) = 0;
    virtual void visit (const Signature& type) = 0;
    virtual void visit (const Struct& type) = 0;
    virtual void visit (const Int& type) = 0;
    virtual void visit (const Int8& type) = 0;
    virtual void visit (const Int16& type) = 0;
    virtual void visit (const Int32& type) = 0;
    virtual void visit (const Int64& type) = 0;
    virtual void visit (const Uint& type) = 0;
    virtual void visit (const Uint8& type) = 0;
    virtual void visit (const Uint16& type) = 0;
    virtual void visit (const Uint32& type) = 0;
    virtual void visit (const Uint64& type) = 0;
    virtual void visit (const Float32& type) = 0;
    virtual void visit (const Float64& type) = 0;
    virtual void visit (const Complex64& type) = 0;
    virtual void visit (const Complex128& type) = 0;
    virtual void visit (const StringU& type) = 0;
    virtual void visit (const Nil& type) = 0;
    virtual void visit (const Boolean& type) = 0;
    virtual void visit (const Rune& type) = 0;
    virtual void visit (const Integer& type) = 0;
    virtual void visit (const Float& type) = 0;
    virtual void visit (const Complex& type) = 0;
    virtual void visit (const String& type) = 0;
    virtual void visit (const Void& type) = 0;
    virtual void visit (const Template& type) = 0;
    virtual void visit (const Uintptr& type) = 0;
  };

  template <typename T>
  struct ComparableVisitor : public Visitor
  {
    typedef T DispatchType;

    T& t;
    ComparableVisitor (T& t_) : t (t_) { }

    virtual void visit (const Array& type)
    {
      t.NotComparable (type);
    }
    virtual void visit (const Slice& type)
    {
      t.NotComparable (type);
    }
    virtual void visit (const Bool& type)
    {
      t (type);
    }
    virtual void visit (const Component& type)
    {
      t.NotComparable (type);
    }
    virtual void visit (const Enum& type)
    {
      t (type);
    }
    virtual void visit (const Function& type)
    {
      t.NotComparable (type);
    }
    virtual void visit (const Method& type)
    {
      t.NotComparable (type);
    }
    virtual void visit (const Heap& type)
    {
      t.NotComparable (type);
    }
    virtual void visit (const FileDescriptor& type)
    {
      t.NotComparable (type);
    }
    virtual void visit (const NamedType& type)
    {
      type.UnderlyingType ()->Accept (*this);
    }
    virtual void visit (const Pointer& type)
    {
      t (type);
    }
    virtual void visit (const Signature& type)
    {
      t.NotComparable (type);
    }
    virtual void visit (const Struct& type)
    {
      t.NotComparable (type);
    }
    virtual void visit (const Int& type)
    {
      t (type);
    }
    virtual void visit (const Int8& type)
    {
      t (type);
    }
    virtual void visit (const Int16& type)
    {
      t (type);
    }
    virtual void visit (const Int32& type)
    {
      t (type);
    }
    virtual void visit (const Int64& type)
    {
      t (type);
    }
    virtual void visit (const Uint& type)
    {
      t (type);
    }
    virtual void visit (const Uint8& type)
    {
      t (type);
    }
    virtual void visit (const Uint16& type)
    {
      t (type);
    }
    virtual void visit (const Uint32& type)
    {
      t (type);
    }
    virtual void visit (const Uint64& type)
    {
      t (type);
    }
    virtual void visit (const Float32& type)
    {
      t (type);
    }
    virtual void visit (const Float64& type)
    {
      t (type);
    }
    virtual void visit (const Complex64& type)
    {
      t (type);
    }
    virtual void visit (const Complex128& type)
    {
      t (type);
    }
    virtual void visit (const StringU& type)
    {
      t (type);
    }
    virtual void visit (const Nil& type)
    {
      t.NotComparable (type);
    }
    virtual void visit (const Boolean& type)
    {
      t (type);
    }
    virtual void visit (const Rune& type)
    {
      t (type);
    }
    virtual void visit (const Integer& type)
    {
      t (type);
    }
    virtual void visit (const Float& type)
    {
      t (type);
    }
    virtual void visit (const Complex& type)
    {
      t (type);
    }
    virtual void visit (const String& type)
    {
      t (type);
    }
    virtual void visit (const Void& type)
    {
      t.NotComparable (type);
    }
    virtual void visit (const Template& type)
    {
      t.NotComparable (type);
    }
    virtual void visit (const Uintptr& type)
    {
      t (type);
    }
   };

  template <typename T>
  struct OrderableVisitor : public Visitor
  {
    typedef T DispatchType;

    T& t;
    OrderableVisitor (T& t_) : t (t_) { }

    virtual void visit (const Array& type)
    {
      t.NotOrderable (type);
    }
    virtual void visit (const Slice& type)
    {
      t.NotOrderable (type);
    }
    virtual void visit (const Bool& type)
    {
      t.NotOrderable (type);
    }
    virtual void visit (const Component& type)
    {
      t.NotOrderable (type);
    }
    virtual void visit (const Enum& type)
    {
      t.NotOrderable (type);
    }
    virtual void visit (const Function& type)
    {
      t.NotOrderable (type);
    }
    virtual void visit (const Method& type)
    {
      t.NotOrderable (type);
    }
    virtual void visit (const Heap& type)
    {
      t.NotOrderable (type);
    }
    virtual void visit (const FileDescriptor& type)
    {
      t.NotOrderable (type);
    }
    virtual void visit (const NamedType& type)
    {
      type.UnderlyingType ()->Accept (*this);
    }
    virtual void visit (const Pointer& type)
    {
      t.NotOrderable (type);
    }
    virtual void visit (const Signature& type)
    {
      t.NotOrderable (type);
    }
    virtual void visit (const Struct& type)
    {
      t.NotOrderable (type);
    }
    virtual void visit (const Int& type)
    {
      t (type);
    }
    virtual void visit (const Int8& type)
    {
      t (type);
    }
    virtual void visit (const Int16& type)
    {
      t (type);
    }
    virtual void visit (const Int32& type)
    {
      t (type);
    }
    virtual void visit (const Int64& type)
    {
      t (type);
    }
    virtual void visit (const Uint& type)
    {
      t (type);
    }
    virtual void visit (const Uint8& type)
    {
      t (type);
    }
    virtual void visit (const Uint16& type)
    {
      t (type);
    }
    virtual void visit (const Uint32& type)
    {
      t (type);
    }
    virtual void visit (const Uint64& type)
    {
      t (type);
    }
    virtual void visit (const Float32& type)
    {
      t (type);
    }
    virtual void visit (const Float64& type)
    {
      t (type);
    }
    virtual void visit (const Complex64& type)
    {
      t.NotOrderable (type);
    }
    virtual void visit (const Complex128& type)
    {
      t.NotOrderable (type);
    }
    virtual void visit (const StringU& type)
    {
      t (type);
    }
    virtual void visit (const Nil& type)
    {
      t.NotOrderable (type);
    }
    virtual void visit (const Boolean& type)
    {
      t.NotOrderable (type);
    }
    virtual void visit (const Rune& type)
    {
      t (type);
    }
    virtual void visit (const Integer& type)
    {
      t (type);
    }
    virtual void visit (const Float& type)
    {
      t (type);
    }
    virtual void visit (const Complex& type)
    {
      t.NotOrderable (type);
    }
    virtual void visit (const String& type)
    {
      t (type);
    }
    virtual void visit (const Void& type)
    {
      t.NotOrderable (type);
    }
    virtual void visit (const Template& type)
    {
      t.NotOrderable (type);
    }
    virtual void visit (const Uintptr& type)
    {
      t (type);
    }
  };

  template <typename T>
  struct ArithmeticVisitor : public Visitor
  {
    typedef T DispatchType;

    T& t;
    ArithmeticVisitor (T& t_) : t (t_) { }

    virtual void visit (const Array& type)
    {
      t.NotArithmetic (type);
    }
    virtual void visit (const Slice& type)
    {
      t.NotArithmetic (type);
    }
    virtual void visit (const Bool& type)
    {
      t.NotArithmetic (type);
    }
    virtual void visit (const Component& type)
    {
      t.NotArithmetic (type);
    }
    virtual void visit (const Enum& type)
    {
      t.NotArithmetic (type);
    }
    virtual void visit (const Function& type)
    {
      t.NotArithmetic (type);
    }
    virtual void visit (const Method& type)
    {
      t.NotArithmetic (type);
    }
    virtual void visit (const Heap& type)
    {
      t.NotArithmetic (type);
    }
    virtual void visit (const FileDescriptor& type)
    {
      t.NotArithmetic (type);
    }
    virtual void visit (const NamedType& type)
    {
      type.UnderlyingType ()->Accept (*this);
    }
    virtual void visit (const Pointer& type)
    {
      t.NotArithmetic (type);
    }
    virtual void visit (const Signature& type)
    {
      t.NotArithmetic (type);
    }
    virtual void visit (const Struct& type)
    {
      t.NotArithmetic (type);
    }
    virtual void visit (const Int& type)
    {
      t (type);
    }
    virtual void visit (const Int8& type)
    {
      t (type);
    }
    virtual void visit (const Int16& type)
    {
      t (type);
    }
    virtual void visit (const Int32& type)
    {
      t (type);
    }
    virtual void visit (const Int64& type)
    {
      t (type);
    }
    virtual void visit (const Uint& type)
    {
      t (type);
    }
    virtual void visit (const Uint8& type)
    {
      t (type);
    }
    virtual void visit (const Uint16& type)
    {
      t (type);
    }
    virtual void visit (const Uint32& type)
    {
      t (type);
    }
    virtual void visit (const Uint64& type)
    {
      t (type);
    }
    virtual void visit (const Float32& type)
    {
      t (type);
    }
    virtual void visit (const Float64& type)
    {
      t (type);
    }
    virtual void visit (const Complex64& type)
    {
      t (type);
    }
    virtual void visit (const Complex128& type)
    {
      t (type);
    }
    virtual void visit (const StringU& type)
    {
      t.NotArithmetic (type);
    }
    virtual void visit (const Nil& type)
    {
      t.NotArithmetic (type);
    }
    virtual void visit (const Boolean& type)
    {
      t.NotArithmetic (type);
    }
    virtual void visit (const Rune& type)
    {
      t (type);
    }
    virtual void visit (const Integer& type)
    {
      t (type);
    }
    virtual void visit (const Float& type)
    {
      t (type);
    }
    virtual void visit (const Complex& type)
    {
      t (type);
    }
    virtual void visit (const String& type)
    {
      t.NotArithmetic (type);
    }
    virtual void visit (const Void& type)
    {
      t.NotArithmetic (type);
    }
    virtual void visit (const Template& type)
    {
      t.NotArithmetic (type);
    }
    virtual void visit (const Uintptr& type)
    {
      t (type);
    }
  };

  template <typename T>
  struct IntegralVisitor : public Visitor
  {
    typedef T DispatchType;

    T& t;
    IntegralVisitor (T& t_) : t (t_) { }

    virtual void visit (const Array& type)
    {
      t.NotIntegral (type);
    }
    virtual void visit (const Slice& type)
    {
      t.NotIntegral (type);
    }
    virtual void visit (const Bool& type)
    {
      t.NotIntegral (type);
    }
    virtual void visit (const Component& type)
    {
      t.NotIntegral (type);
    }
    virtual void visit (const Enum& type)
    {
      t.NotIntegral (type);
    }
    virtual void visit (const Function& type)
    {
      t.NotIntegral (type);
    }
    virtual void visit (const Method& type)
    {
      t.NotIntegral (type);
    }
    virtual void visit (const Heap& type)
    {
      t.NotIntegral (type);
    }
    virtual void visit (const FileDescriptor& type)
    {
      t.NotIntegral (type);
    }
    virtual void visit (const NamedType& type)
    {
      type.UnderlyingType ()->Accept (*this);
    }
    virtual void visit (const Pointer& type)
    {
      t.NotIntegral (type);
    }
    virtual void visit (const Signature& type)
    {
      t.NotIntegral (type);
    }
    virtual void visit (const Struct& type)
    {
      t.NotIntegral (type);
    }
    virtual void visit (const Int& type)
    {
      t (type);
    }
    virtual void visit (const Int8& type)
    {
      t (type);
    }
    virtual void visit (const Int16& type)
    {
      t (type);
    }
    virtual void visit (const Int32& type)
    {
      t (type);
    }
    virtual void visit (const Int64& type)
    {
      t (type);
    }
    virtual void visit (const Uint& type)
    {
      t (type);
    }
    virtual void visit (const Uint8& type)
    {
      t (type);
    }
    virtual void visit (const Uint16& type)
    {
      t (type);
    }
    virtual void visit (const Uint32& type)
    {
      t (type);
    }
    virtual void visit (const Uint64& type)
    {
      t (type);
    }
    virtual void visit (const Float32& type)
    {
      t.NotIntegral (type);
    }
    virtual void visit (const Float64& type)
    {
      t.NotIntegral (type);
    }
    virtual void visit (const Complex64& type)
    {
      t.NotIntegral (type);
    }
    virtual void visit (const Complex128& type)
    {
      t.NotIntegral (type);
    }
    virtual void visit (const StringU& type)
    {
      t.NotIntegral (type);
    }
    virtual void visit (const Nil& type)
    {
      t.NotIntegral (type);
    }
    virtual void visit (const Boolean& type)
    {
      t.NotIntegral (type);
    }
    virtual void visit (const Rune& type)
    {
      t (type);
    }
    virtual void visit (const Integer& type)
    {
      t (type);
    }
    virtual void visit (const Float& type)
    {
      t.NotIntegral (type);
    }
    virtual void visit (const Complex& type)
    {
      t.NotIntegral (type);
    }
    virtual void visit (const String& type)
    {
      t.NotIntegral (type);
    }
    virtual void visit (const Void& type)
    {
      t.NotIntegral (type);
    }
    virtual void visit (const Template& type)
    {
      t.NotIntegral (type);
    }
    virtual void visit (const Uintptr& type)
    {
      t (type);
    }
  };

  template <typename T>
  struct LogicalVisitor : public Visitor
  {
    typedef T DispatchType;

    T& t;
    LogicalVisitor (T& t_) : t (t_) { }

    virtual void visit (const Array& type)
    {
      t.NotLogical (type);
    }
    virtual void visit (const Slice& type)
    {
      t.NotLogical (type);
    }
    virtual void visit (const Bool& type)
    {
      t (type);
    }
    virtual void visit (const Component& type)
    {
      t.NotLogical (type);
    }
    virtual void visit (const Enum& type)
    {
      t.NotLogical (type);
    }
    virtual void visit (const Function& type)
    {
      t.NotLogical (type);
    }
    virtual void visit (const Method& type)
    {
      t.NotLogical (type);
    }
    virtual void visit (const Heap& type)
    {
      t.NotLogical (type);
    }
    virtual void visit (const FileDescriptor& type)
    {
      t.NotLogical (type);
    }
    virtual void visit (const NamedType& type)
    {
      type.UnderlyingType ()->Accept (*this);
    }
    virtual void visit (const Pointer& type)
    {
      t.NotLogical (type);
    }
    virtual void visit (const Signature& type)
    {
      t.NotLogical (type);
    }
    virtual void visit (const Struct& type)
    {
      t.NotLogical (type);
    }
    virtual void visit (const Int& type)
    {
      t.NotLogical (type);
    }
    virtual void visit (const Int8& type)
    {
      t.NotLogical (type);
    }
    virtual void visit (const Int16& type)
    {
      t.NotLogical (type);
    }
    virtual void visit (const Int32& type)
    {
      t.NotLogical (type);
    }
    virtual void visit (const Int64& type)
    {
      t.NotLogical (type);
    }
    virtual void visit (const Uint& type)
    {
      t.NotLogical (type);
    }
    virtual void visit (const Uint8& type)
    {
      t.NotLogical (type);
    }
    virtual void visit (const Uint16& type)
    {
      t.NotLogical (type);
    }
    virtual void visit (const Uint32& type)
    {
      t.NotLogical (type);
    }
    virtual void visit (const Uint64& type)
    {
      t.NotLogical (type);
    }
    virtual void visit (const Float32& type)
    {
      t.NotLogical (type);
    }
    virtual void visit (const Float64& type)
    {
      t.NotLogical (type);
    }
    virtual void visit (const Complex64& type)
    {
      t.NotLogical (type);
    }
    virtual void visit (const Complex128& type)
    {
      t.NotLogical (type);
    }
    virtual void visit (const StringU& type)
    {
      t.NotLogical (type);
    }
    virtual void visit (const Nil& type)
    {
      t.NotLogical (type);
    }
    virtual void visit (const Boolean& type)
    {
      t (type);
    }
    virtual void visit (const Rune& type)
    {
      t.NotLogical (type);
    }
    virtual void visit (const Integer& type)
    {
      t.NotLogical (type);
    }
    virtual void visit (const Float& type)
    {
      t.NotLogical (type);
    }
    virtual void visit (const Complex& type)
    {
      t.NotLogical (type);
    }
    virtual void visit (const String& type)
    {
      t.NotLogical (type);
    }
    virtual void visit (const Void& type)
    {
      t.NotLogical (type);
    }
    virtual void visit (const Template& type)
    {
      t.NotLogical (type);
    }
    virtual void visit (const Uintptr& type)
    {
      t.NotLogical (type);
    }
  };

  struct DefaultVisitor : public Visitor
  {
    virtual void visit (const Array& type)
    {
      default_action (type);
    }
    virtual void visit (const Slice& type)
    {
      default_action (type);
    }
    virtual void visit (const Bool& type)
    {
      default_action (type);
    }
    virtual void visit (const Component& type)
    {
      default_action (type);
    }
    virtual void visit (const Enum& type)
    {
      default_action (type);
    }
    virtual void visit (const Function& type)
    {
      default_action (type);
    }
    virtual void visit (const Method& type)
    {
      default_action (type);
    }
    virtual void visit (const Heap& type)
    {
      default_action (type);
    }
    virtual void visit (const FileDescriptor& type)
    {
      default_action (type);
    }
    virtual void visit (const NamedType& type)
    {
      default_action (type);
    }
    virtual void visit (const Pointer& type)
    {
      default_action (type);
    }
    virtual void visit (const Signature& type)
    {
      default_action (type);
    }
    virtual void visit (const Struct& type)
    {
      default_action (type);
    }
    virtual void visit (const Int& type)
    {
      default_action (type);
    }
    virtual void visit (const Int8& type)
    {
      default_action (type);
    }
    virtual void visit (const Int16& type)
    {
      default_action (type);
    }
    virtual void visit (const Int32& type)
    {
      default_action (type);
    }
    virtual void visit (const Int64& type)
    {
      default_action (type);
    }
    virtual void visit (const Uint& type)
    {
      default_action (type);
    }
    virtual void visit (const Uint8& type)
    {
      default_action (type);
    }
    virtual void visit (const Uint16& type)
    {
      default_action (type);
    }
    virtual void visit (const Uint32& type)
    {
      default_action (type);
    }
    virtual void visit (const Uint64& type)
    {
      default_action (type);
    }
    virtual void visit (const Float32& type)
    {
      default_action (type);
    }
    virtual void visit (const Float64& type)
    {
      default_action (type);
    }
    virtual void visit (const Complex64& type)
    {
      default_action (type);
    }
    virtual void visit (const Complex128& type)
    {
      default_action (type);
    }
    virtual void visit (const StringU& type)
    {
      default_action (type);
    }
    virtual void visit (const Nil& type)
    {
      default_action (type);
    }
    virtual void visit (const Boolean& type)
    {
      default_action (type);
    }
    virtual void visit (const Rune& type)
    {
      default_action (type);
    }
    virtual void visit (const Integer& type)
    {
      default_action (type);
    }
    virtual void visit (const Float& type)
    {
      default_action (type);
    }
    virtual void visit (const Complex& type)
    {
      default_action (type);
    }
    virtual void visit (const String& type)
    {
      default_action (type);
    }
    virtual void visit (const Void& type)
    {
      default_action (type);
    }
    virtual void visit (const Template& type)
    {
      default_action (type);
    }
    virtual void visit (const Uintptr& type)
    {
      default_action (type);
    }

    virtual void default_action (const Type& type) { }
  };

  template <typename T, typename T1>
  struct visitor2 : public DefaultVisitor
  {
    const T1& type1;
    T& t;

    visitor2 (const T1& t1, T& t_) : type1 (t1), t (t_) { }

    void default_action (const Type& type)
    {
      type_not_reached (type);
    }

    void visit (const Bool& type2)
    {
      t (type1, type2);
    }

    void visit (const Int& type2)
    {
      t (type1, type2);
    }
    void visit (const Int8& type2)
    {
      t (type1, type2);
    }
    void visit (const Int16& type2)
    {
      t (type1, type2);
    }
    void visit (const Int32& type2)
    {
      t (type1, type2);
    }
    void visit (const Int64& type2)
    {
      t (type1, type2);
    }

    void visit (const Uint& type2)
    {
      t (type1, type2);
    }
    void visit (const Uint8& type2)
    {
      t (type1, type2);
    }
    void visit (const Uint16& type2)
    {
      t (type1, type2);
    }
    void visit (const Uint32& type2)
    {
      t (type1, type2);
    }
    void visit (const Uint64& type2)
    {
      t (type1, type2);
    }

    void visit (const Float32& type2)
    {
      t (type1, type2);
    }
    void visit (const Float64& type2)
    {
      t (type1, type2);
    }

    void visit (const Complex64& type2)
    {
      t (type1, type2);
    }
    void visit (const Complex128& type2)
    {
      t (type1, type2);
    }

    void visit (const Pointer& type2)
    {
      t (type1, type2);
    }

    void visit (const Slice& type2)
    {
      t (type1, type2);
    }

    void visit (const StringU& type2)
    {
      t (type1, type2);
    }

    void visit (const Boolean& type2)
    {
      t (type1, type2);
    }
    void visit (const Rune& type2)
    {
      t (type1, type2);
    }
    void visit (const Integer& type2)
    {
      t (type1, type2);
    }
    void visit (const Float& type2)
    {
      t (type1, type2);
    }

    void visit (const String& type2)
    {
      t (type1, type2);
    }

    void visit (const Nil& type2)
    {
      t (type1, type2);
    }
  };

  template <typename T, typename T1>
  static void doubleDispatchHelper (const T1& type1, const Type* type2, T& t)
  {
    visitor2<T, T1> v (type1, t);
    type2->Accept (v);
  }

  template <typename T>
  struct visitor1 : public DefaultVisitor
  {
    const Type* type2;
    T& t;
    visitor1 (const Type* t2, T& t_) : type2 (t2), t (t_) { }

    void default_action (const Type& type)
    {
      type_not_reached (type);
    }

    void visit (const Bool& type)
    {
      doubleDispatchHelper (type, type2, t);
    }

    void visit (const Int& type)
    {
      doubleDispatchHelper (type, type2, t);
    }
    void visit (const Int8& type)
    {
      doubleDispatchHelper (type, type2, t);
    }
    void visit (const Int16& type)
    {
      doubleDispatchHelper (type, type2, t);
    }
    void visit (const Int32& type)
    {
      doubleDispatchHelper (type, type2, t);
    }
    void visit (const Int64& type)
    {
      doubleDispatchHelper (type, type2, t);
    }

    void visit (const Uint& type)
    {
      doubleDispatchHelper (type, type2, t);
    }
    void visit (const Uint8& type)
    {
      doubleDispatchHelper (type, type2, t);
    }
    void visit (const Uint16& type)
    {
      doubleDispatchHelper (type, type2, t);
    }
    void visit (const Uint32& type)
    {
      doubleDispatchHelper (type, type2, t);
    }
    void visit (const Uint64& type)
    {
      doubleDispatchHelper (type, type2, t);
    }
    void visit (const Float32& type)
    {
      doubleDispatchHelper (type, type2, t);
    }
    void visit (const Float64& type)
    {
      doubleDispatchHelper (type, type2, t);
    }
    void visit (const Complex64& type)
    {
      doubleDispatchHelper (type, type2, t);
    }
    void visit (const Complex128& type)
    {
      doubleDispatchHelper (type, type2, t);
    }

    void visit (const Pointer& type)
    {
      doubleDispatchHelper (type, type2, t);
    }

    void visit (const StringU& type)
    {
      doubleDispatchHelper (type, type2, t);
    }

    void visit (const Slice& type)
    {
      doubleDispatchHelper (type, type2, t);
    }

    void visit (const Boolean& type)
    {
      doubleDispatchHelper (type, type2, t);
    }

    void visit (const Rune& type)
    {
      doubleDispatchHelper (type, type2, t);
    }
    void visit (const Integer& type)
    {
      doubleDispatchHelper (type, type2, t);
    }
    void visit (const Float& type)
    {
      doubleDispatchHelper (type, type2, t);
    }

    void visit (const String& type)
    {
      doubleDispatchHelper (type, type2, t);
    }

    void visit (const Nil& type)
    {
      doubleDispatchHelper (type, type2, t);
    }
  };

  template <typename T>
  static void DoubleDispatch (const Type* type1, const Type* type2, T& t)
  {
    visitor1<T> v (type2, t);
    type1->Accept (v);
  }

  // Select the appropriate object.
  field_t*
  type_select_field (const Type* type, const std::string& identifier);

  ::Method*
  type_select_method (const Type* type, const std::string& identifier);

  Initializer*
  type_select_initializer (const Type* type, const std::string& identifier);

  Getter*
  type_select_getter (const Type* type, const std::string& identifier);

  reaction_t*
  type_select_reaction (const Type* type, const std::string& identifier);

  action_t*
  type_select_action (const Type* type, const std::string& identifier);

  bind_t*
  type_select_bind (const Type* type, const std::string& identifier);

  // Return type of selected field, method, or reaction.
  const Type*
  type_select (const Type* type, const std::string& identifier);

  // Return the type of indexing into the other type.
  const Type*
  type_index (const Type* base, const Type* index);

  // Return type after applying dereference or NULL.
  const Type*
  type_dereference (const Type* type);

  // Return type after applying move or NULL.
  const Type*
  type_move (const Type* type);

  // Return type after applying merge or NULL.
  const Type*
  type_merge (const Type* type);

  // Return type after applying change or NULL.
  const Type*
  type_change (const Type* type);

  // True if the types are equal (strict).
  bool
  type_is_equal (const Type* x, const Type* y);

  bool
  Identitical (const Type* x, const Type* y);

  // True if any pointer is accessible.
  bool
  type_contains_pointer (const Type* type);

  // True if boolean operators can be applied to values of this type.
  bool
  type_is_boolean (const Type* type);

  // True if type is an integer.
  bool
  type_is_integral (const Type* type);

  // True if type is an unsigned integer.
  bool
  type_is_unsigned_integral (const Type* type);

  // True if type is floating-point.
  bool
  type_is_floating (const Type* type);

  // True if type is numeric.
  inline bool
  type_is_numeric (const Type* type)
  {
    return type_is_integral (type) || type_is_floating (type);
  }

  // True if == or != can be applied to values of this type.
  bool
  type_is_comparable (const Type* type);

  // True if <, <=, >, or >= can be applied to values of this type.
  bool
  type_is_orderable (const Type* type);

  // True if index is valid.
  bool
  type_is_index (const Type* type, Int::ValueType index);

  // True if x can be cast to y.
  bool
  type_is_castable (const Type* x, const Type* y);

  bool
  type_is_pointer_compare (const Type* left, const Type* right);

  // Remove a NamedType.
  const Type*
  type_strip (const Type* type);

  // Cast a type to a specific type.
  template<typename T>
  const T*
  type_cast (const Type * type)
  {
    if (type == NULL) return NULL;

    struct visitor : public DefaultVisitor
    {
      const T* retval;

      visitor () : retval (NULL) { }

      void visit (const T& type)
      {
        retval = &type;
      }
    };
    visitor v;
    type->Accept (v);
    return v.retval;
  }

  template<typename T>
  const T*
  type_strip_cast (const Type* type)
  {
    if (type == NULL) return NULL;
    return type_cast<T> (type_strip (type));
  }

  extern NamedType NamedBool;

  extern NamedType NamedUint8;
  extern NamedType NamedUint16;
  extern NamedType NamedUint32;
  extern NamedType NamedUint64;

  extern NamedType NamedInt8;
  extern NamedType NamedInt16;
  extern NamedType NamedInt32;
  extern NamedType NamedInt64;

  extern NamedType NamedFloat32;
  extern NamedType NamedFloat64;

  extern NamedType NamedComplex64;
  extern NamedType NamedComplex128;

  extern NamedType NamedUint;
  extern NamedType NamedInt;
  extern NamedType NamedUintptr;

  extern NamedType NamedRune;
  extern NamedType NamedByte;
  extern NamedType NamedString;

  extern NamedType NamedFileDescriptor;
}

#endif /* Type_hpp */