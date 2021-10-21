# Sig
## The most powerful and customizable binary pattern scanner written on modern C++
### ✔ Capabilities:
* Support for all common pattern formats
* Support for template-based patterns
* Support for custom comparators
* Extensible and customizable patterns
* Header-only
* Modern C++ (requires C++17 or above)
* Usermode and kernelmode support on Windows, Linux and MacOS
* Unit-tests

### 🔍 Overview of pattern types:
```cpp
// Template-based:
Sig::find<Sig::Byte<0xAA, 0xBB>, Sig::Dword<>, Sig::Char<'t', 'e', 'x', 't'>>(buf, size);

// Pattern + Mask:
Sig::find<Sig::Mask::Eq<'.'>, Sig::Mask::Any<'?'>>(buf, size, "\x11\x22\x00\x44", "..?.");

// One line:
Sig::find(buf, size, "11 22 ? 44 ?? 66 AA bb cC Dd");
```

### 👾 Template-based patterns:
This type generates a comparing function in compile-time for each pattern:
```cpp
Sig::find<PATTERN>(buf, size);
```
The 'PATTERN' is a set of tags with comparators inside.  
A tag format is:
```cpp
template <typename Type, Type value>
struct Comparator
{
    static bool cmp(const void* const pos)
    {
        return ...;
    }
};

template <typename Type, Type... values>
using Tag = Sig::Holder<Type, Comparator, values...>;
```
There are a lot of predefined tags, comparators and containers:
```cpp
// Tags:
Sig::Byte/Word/Dword/Qword/Int/UInt/Char/WChar/Short/UShort/Long/ULong/LongLong/ULongLong<...>
Sig::Not[All above]<...> // e.g. Sig::NotByte, Sig::NotDword, ...

// Comparators:
Sig::Cmp::Eq     // Equal
Sig::Cmp::NotEq  // Not equal
Sig::Cmp::Gr     // Greater
Sig::Cmp::Le     // Less
Sig::Cmp::GrEq   // Greater or equal
Sig::Cmp::LeEq   // Less or equal
Sig::Cmp::OneOf  // One of a bits is setted ((value & mask) != 0)
Sig::Cmp::AllOf  // All of a bits are setted ((value & mask) == mask)

// Containers:
Sig::Rep<Tag, count>  // Repeat a tag by a count times, e.g. Sig::Rep<Sig::Byte<0xFF>, 3> to compare with FF FF FF
Sig::Set<Tag1, Tag2, ...>  // Check an equality with one of defined tags, e.g. Sig::Set<Sig::Byte<1>, Sig::Dword<-1u>>
Sig::Range<Tag, from, to>  // Check whether a value is in a range [from, to], e.g. Sig::Range<Sig::Byte, 10, 20>
Sig::Compound<Tag1, Tag2, ...>  // A storage for creating user defined patterns


/* Examples */

template <typename Type, Type value>
struct CustomCmp
{
    static bool cmp(const void* const pos)
    {
        // Custom comparator:
        return *static_cast<const Type*>(pos) == (value / 2);
    }
};
template <unsigned char... values>
using CustomByte = Sig::Holder<unsigned char, CustomCmp, values...>;

template <typename Type, Type... values>
using Mask = Sig::Holder<Type, Sig::Cmp::AllOf, values...>;

template <unsigned char... values>
using ByteMask = Mask<unsigned char, values...>;

using RelJmp = Sig::Compound<Sig::Byte<0xE9>, Sig::Dword<>>;     // E9 ?? ?? ?? ??  | jmp any_addr
using Syscall = Sig::Compound<Sig::Byte<0x0F>, Sig::Byte<0x05>>; // 0F 05           | syscall

const void* const found = Sig::find<
    Sig::Byte<0xFF>, // Exact value
    Sig::Dword<>,    // Any dword
    Sig::Char<'t', 'e', 'x', 't'>,  // Sequence of values
    Sig::Rep<Sig::Byte<0xCC>, 3>,  // Is equivalent to "Sig::Byte<0xCC>, Sig::Byte<0xCC>, Sig::Byte<0xCC>"
    Sig::Range<Sig::Byte, 4, 7>, // The byte must be one of 4, 5, 6 or 7
    Sig::Set<Sig::Byte<10>, Sig::Dword<0x11223344>>, // The memory in this place must be one of 10 or 0x11223344
    ByteMask<0b1101011, 0b1010>,  // Each byte must match a corresponding mask
    CustomByte<2, 4, 6>, // Three bytes at this pos must match 1, 2 and 3 accordingly
    RelJump,  // -+
    Syscall   // -+-> Using of user-defined patterns
>(buf, size);
```

### 👻 Pattern + Mask:
It is the common type of pattern format: signature with mask that defines meaningful and meaningless bytes.  
E.g.: `"\x11\x22\x00\x44"` + `"..?."`.  
You can use this format with predefined comparators and define your custom comparators for each mask symbol.
```cpp
// Predefined comparators (are the same as for template-based patterns),
// they accept a corresponding char in a mask string:
Sig::Mask::Eq  // Equal
Sig::Mask::NotEq  // Not equal
Sig::Mask::Gr  // Greater
Sig::Mask::Le  // Less
Sig::Mask::GrEq  // Greater or equal
Sig::Mask::LeEq  // Less or equal
Sig::Mask::OneOf  // One of a bits is setted ((value & mask) != 0)
Sig::Mask::AllOf  // All of a bits are setted ((value & mask) == mask)

// You must define comparators for all unique chars in a mask.

// The simpliest example:
Sig::find<Sig::Mask::Eq<'.'>, Sig::Mask::Any<'?'>>(buf, size, "\x11\x22\x00\x44", "..?.");

// The example above means that the char '.' in the mask is the equality comparator
// and the char '?' is an any byte.


// More advanced example with the custom comparator:

struct CustomCmp : Sig::Mask::CharHolder<'c'>
{
    static bool cmp(const char data, const char pattern)
    {
        return data > (pattern * 2);
    }
};

const void* const found = Sig::find<
    Sig::Mask::Eq<'.'>,
    Sig::Mask::NotEq<'!'>,
    Sig::Mask::Any<'?'>,
    Sig::Mask::OneOf<'o'>,
    Sig::Mask::AllOf<'a'>,
    Sig::Mask::Le<'<'>,
    Sig::Mask::Gr<'>'>,
    CustomCmp,
>(buf, size, "\x11\x22\x00\x44\x55\x66\x77\x88", ".!?oa<>c");

```

### 😊 One-line patterns:
It is the friendly and easy-to-use type of pattern.  
It has no customizations, just one line pattern:
```cpp
const void* const found = Sig::find(buf, size, "AA BB ?? DD 1 2 ? 4 5 6");
```
Tokens `??` and `?` have the same meaning: any byte.

## Usage:
Just include the `Sig.hpp` from the `include` folder and you're good to go!
```cpp
#include <Sig.hpp>

#include <cassert>

static const unsigned char g_arr[]
{
    '?', '?', '?', '?',
    0x11, 0x22, 0x33, 0x44
};

int main()
{
    const void* const found1 = Sig::find<Sig::Dword<0x44332211>>(g_arr, sizeof(g_arr));
    
    const void* const found2 = Sig::find<
        Sig::Mask::Eq<'.'>,
        Sig::Mask::Any<'?'>
    >(g_arr, sizeof(g_arr), "\x11\x22\x00\x44", "..?.");

    const void* const found3 = Sig::find(g_arr, sizeof(g_arr), "11 22 ? 44");

    assert(found1 == &g_arr[4]);
    assert(found1 == found2);
    assert(found2 == found3);

    return 0;
}
```