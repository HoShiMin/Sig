# Sig
## The most powerful and customizable binary pattern scanner written on modern C++
### ✔ Capabilities:
* Support for all common pattern formats:
* * Pattern + mask: `"\x11\x22\x00\x44"` + `"..?."`
* * One-line pattern: `"11 22 ? AA BB ? ? EE FF"`
* * Bitmask + valuable bits mask: `"\x11\x13\x33"` + `"\xFF\x1F\xFF"`
* Support for template-based patterns:
* * `Sig::find<Sig::Byte<1, 2, 3>, Sig::Dword<>, Sig::StrEq<"text">>(buf, size);`
* Support for custom comparators.
* Extensible and customizable patterns.
* Header-only.
* Modern C++ (requires C++17 or above (and requires C++20 for `Sig::StrEq<"text">`)).
* Works in usermode and kernelmode on Windows, Linux and MacOS.
* Unit-tests.

### 🔍 Overview of pattern types:
```cpp
// Template-based:
Sig::find<Sig::Byte<0xAA, 0xBB>, Sig::Dword<>, Sig::Char<'t', 'e', 'x', 't'>>(buf, size);
Sig::find<Sig::CmpByte<Sig::NotEq, 1, 2, 3>>(buf, size);

// Pattern + Mask:
Sig::find<Sig::Mask::Eq<'.'>, Sig::Mask::Any<'?'>>(buf, size, "\x11\x22\x00\x44", "..?.");

// Pattern + Subpattern + Mask:
Sig::find<
    Sig::Mask::Eq<'.'>,
    Sig::Mask::Any<'?'>,
    Sig::Mask::BitMask<'m'>
>(buf, size, "\x11\x22\x19\x44", "\x00\x00\x1F\x00", "..m.?.");

// Bitmask + mask of meaningful bits in the bitmask:
Sig::bitmask(buf, size, "\x11\x22\x19", "\xFF\xFF\x1F");

// One line:
Sig::find(buf, size, "11 22 ? 44 ?? 66 AA bb cC Dd");
```

### 👾 Template-based patterns:
This type generates a comparing function in compile-time for each pattern:
```cpp
Sig::find<Tag1, Tag2, ...>(buf, size);
```

There are a lot of predefined tags, comparators and containers:
```cpp
// Tags:
Sig::Byte/Word/Dword/Qword/Int/UInt/Int64/UInt64/Char/WChar/Short/UShort/Long/ULong/LongLong/ULongLong<...>
Sig::Cmp[Byte/Word/...]<Comparator, values...> // Like Sig::CmpByte<...>, Sig::CmpDword<...>, etc.
Sig::[Byte/Word/Dword/Qword]Mask<value, mask> // To check whether the specified bits are equal to

// Comparators for Sig::Cmp***<Comparator, values...>:
Sig::Cmp::Eq       // Equal
Sig::Cmp::NotEq    // Not equal
Sig::Cmp::Gr       // Greater
Sig::Cmp::Le       // Less
Sig::Cmp::GrEq     // Greater or equal
Sig::Cmp::LeEq     // Less or equal
Sig::Cmp::OneOf    // One of a bits is setted ((value & mask) != 0)
Sig::Cmp::AllOf    // All of a bits are setted ((value & mask) == mask)
Sig::Cmp::BitMask<mask, mean>  // All of the specified bits are equal to the pattern ((val & mean) == (mask & mean))

// Special cases for strings (requires C++20 or above):
Sig::StrEq<"Sample text">        // Compare the string as-is (not including null-terminator)
Sig::StrEqNoCase<"SaMpLe TeXt">  // Case-insensitive comparation (only for English text)

// Containers:
Sig::Rep<Tag, count>  // Repeat a tag by a count times, e.g. Sig::Rep<Sig::Byte<0xFF>, 3> to compare with \xFF\xFF\xFF
Sig::Set<Tag1, Tag2, ...>  // Check an equality with one of defined tags, e.g. Sig::Set<Sig::Byte<1>, Sig::Dword<-1u>>
Sig::Range<Tag, from, to>  // Check whether a value is in a range [from, to], e.g. Sig::Range<Sig::Byte, 10, 20>
Sig::Compound<Tag1, Tag2, ...>  // A storage for creating user defined patterns


/* Examples */

// Find the sequence of bytes (0x01 0x02 0x03):
Sig::find<Sig::Byte<1, 2, 3>>(buf, size);

// Find the sequence with a couple of unknown bytes (0x01 0x02 ? 0x04):
Sig::find<Sig::Byte<1, 2>, Sig::Byte<>, Sig::Byte<4>>(buf, size);
//                                  ^ Any byte

// Find the null-terminated strings:
Sig::find<
    Sig::StrEq<"text">, Sig::Char<0x00>, // ASCII-compatible string
    Sig::StrEqNoCase<L"TeXt">, Sig::WChar<0x0000> // UTF-16 string
>(buf, size);

// Using different comparators:
Sig::find<
    Sig::Byte<0xAA>,                            // (pos[0] == 0xAA) &&
    Sig::CmpByte<Sig::Cmp::NotEq, 0xBB, 0xCC>,  // (pos[1] != 0xBB) && (pos[2] != 0xCC) &&
    Sig::CmpDword<Sig::Cmp::Gr, 0x1EE7C0DE>     // (*(dword*)&pos[3] > 0x1EE7C0DE)
>(buf, size);

// Find the matching bits:
Sig::find<
    Sig::Byte<1>,                     // (pos[0] == 1) &&
    Sig::ByteMask<0b110100, 0b111101> // (pos[2] == 0b??1101?0)
>(buf, size);

// Using the Sig::Rep (find the repeating pattern):
const uint8_t buf[]{ 1, 2, 3, 'r', 'r', 'r', 'r', 'r', 4, 5 };
Sig::find<Sig::Rep<Sig::Char<'r'>, 5>>(buf, sizeof(buf));

// Using the Sig::Set (find one of the tags):
const uint8_t buf[]{ '?', 0xDE, 0xC0, 0xE7, 0x1E, '!' };
Sig::find<Sig::Set<Sig::Byte<1>, Sig::Dword<0x1EE7C0DE>>, Sig::Char<'!'>>(buf, sizeof(buf));
// ^ It is either the Sig::Byte<1> or the Sig::Dword<0x1EE7C0DE>.
// The next position will be the current pos + size of matched tag.

// Using the Sig::Range (from..to):
const uint8_t buf[]{ 10, 20, 30, 40, 50 };
Sig::find<Sig::Range<Sig::Byte, 29, 31>>(buf, sizeof(buf));
// ^ The value must be in the range [29 >= x >= 31]

// Using the Sig::Compound (user-defined patterns):
const uint8_t buf[]{ '?', '?', 0xE9, 0x11, 0x22, 0x33, 0x44, '?', 0x0F, 0x05 };
using RelJump = Sig::Compound<Sig::Byte<0xE9>, Sig::Dword<>>; // E9 ?? ?? ?? ??  | jmp unknown_offset
using Syscall = Sig::Compound<Sig::Byte<0x0F, 0x05>>;         // 0F 05           | syscall
Sig::find<RelJump, Sig::Byte<>, Syscall>(buf, sizeof(buf));

// Using the comparator that doesn't depend on pattern:
template <typename Type>
struct IsOddCmp : Sig::RawCmp<Type>
{
    static bool cmp(const void* const pos)
    {
        return (*static_cast<const Type*>(pos) % 2) != 0;
    }
};
using IsOddByte = IsOddCmp<unsigned char>;

const uint8_t buf[]{ 2, 4, 6, 7, 8, 10, 12 };
Sig::find<IsOddByte>(buf, sizeof(buf));

// Using the comparator that depends on pattern:
template <typename Type, Type val>
struct IsDivisibleByCmp
{
    static bool cmp(const void* const pos)
    {
        return (*static_cast<const Type*>(pos) % val) == 0;
    }
};

template <unsigned char... values>
using IsDivisibleByByte = IsDivisibleByCmp<unsigned char, values...>;

const uint8_t buf[]{ 3, 4, 5, 10, 15, 17 };
Sig::find<IsDivisibleByByte<5, 10, 5>>(buf, sizeof(buf));

// Usng the custom comparator for the string:
template <Sig::String str>
struct StrCmpNoCase : Sig::Str<str>
{
    static bool cmp(const void* const pos)
    {
        using Char = typename decltype(str)::Type;

        const auto* const mem = static_cast<const Char*>(pos);
        for (size_t i = 0; i < decltype(str)::k_len; ++i)
        {
            const auto low = [](const Char ch) -> Char
            {
                return ((ch >= static_cast<Char>('A')) && (ch <= static_cast<Char>('Z')))
                    ? (ch + static_cast<Char>('a' - 'A'))
                    : (ch);
            };

            const auto left = low(mem[i]);
            const auto right = low(str.str.buf[i]);
            
            if (left != right)
            {
                return false;
            }
        }

        return true;
    }
};

Sig::find<StrCmpNoCase<"ASCII">, StrCmpNoCase<L"UTF-16">>(buf, size);
```

### 👻 Pattern + Mask:
It is the common type of pattern format: signature with mask that defines meaningful and meaningless bytes.  
E.g.: `"\x11\x22\x00\x44"` + `"..?."`.  
You can use this format with predefined comparators and define your custom comparators for each mask symbol.
```cpp
// Predefined comparators (are the same as for template-based patterns),
// they accept a corresponding char in a mask string:
Sig::Mask::Eq       // Equal
Sig::Mask::NotEq    // Not equal
Sig::Mask::Gr       // Greater
Sig::Mask::Le       // Less
Sig::Mask::GrEq     // Greater or equal
Sig::Mask::LeEq     // Less or equal
Sig::Mask::OneOf    // One of a bits is setted ((value & mask) != 0)
Sig::Mask::AllOf    // All of a bits are setted ((value & mask) == mask)
Sig::Mask::BitMask  // All of the specified bits are equal to the pattern ((val & mean) == (mask & mean))

// You must define comparators for all unique chars in a mask, otherwise Sig::find will return nullptr. 

// The simpliest example:
const uint8_t buf[]{ '?', 0x11, 0x22, 0xFF, 0x44 };
Sig::find<Sig::Mask::Eq<'.'>, Sig::Mask::Any<'?'>>(buf, sizeof(buf), "\x11\x22\x00\x44", "..?.");
//                       |                    ^- The '?' will mean any byte
//                       + The '.' will mean an exact byte


// The example above means that the char '.' in the mask is the equality comparator
// and the char '?' is an any byte.

// Using the custom basic comparator (Sig::MaskCmp):
template <char mask>
struct IsDivisibleBy : Sig::Mask::MaskCmp<mask>
{
    static bool cmp(const char data, const char pattern)
    {
        return (data % pattern) == 0;
    }
};

const uint8_t buf[]{ '?', 0x11, 0x22, 0xFF, 0x44, 6, 16, 100, '?' };
Sig::find<
    Sig::Mask::Eq<'.'>,
    Sig::Mask::NotEq<'!'>,
    Sig::Mask::Any<'?'>,
    IsDivisibleBy<'d'>,
>(buf, sizeof(buf), "\x10\x22\x00\x44\x02\x04\x0A", "!.?.ddd");

// Using subpatterns for additional data (e.g., bitmasks):
const uint8_t buf[]{ 0xFF, '?', 0b1101'0011, 0x00 };
Sig::find<
    Sig::Mask::Eq<'.'>,
    Sig::Mask::BitMask<'m'>
>(buf, sizeof(buf), "\xFF\x00\x13\x00", "\x00\x00\x1F\x00", ".?m.");
// The meaning:                |                   |
//   (pos[0] == 0xFF) &&       +-------+           +-+  Is equivalent to "val == ???1'0011"
//   (pos[1] == any) &&                V             V
//   ((pos[2] & 0b0001'1111) == (0b0001'0011 & 0b0001'1111)) &&
//   (pos[3] == 0x00)

// The subpattern uses only in extended comparators (e.g. in the Sig::Mask::Bitmask that requires additional info).

// Using the custom extended comparator (Sig::Mask::MaskCmpEx, that requires additional pattern):
template <char mask>
struct IsInRange : Sig::Mask::MaskCmpEx<char>
{
    static bool cmp(const char data, const char pattern, const char subpattern)
    {
        return (data >= pattern) && (data <= subpattern);
    }
};

const uint8_t buf[]{ 0x10, 0x20, 0x30, 0x40, 0x50 };
Sig::find<IsInRange<'r'>>(buf, sizeof(buf), "\x15\x25", "\x25\x35", "rr");

```

### 🧩 Bitmask:
It is the special case of the "pattern + mask" type that compares equality of the specified bits:
```cpp
Sig::bitmask(buf, size, "\x11\x13\x33", "\xFF\x1F\xFF", 3);
//                         ^ Bitmask       ^ Valuable bits in the bitmask
// This means:
//  (pos[0] & 0xFF == 0x11 & 0xFF) &&   | pos[0] == 0001'0001
//  (pos[1] & 0x1F == 0x13 & 0x1F) &&   | pos[1] == ???1'0011
//  (pos[2] & 0xFF == 0x33 & 0xFF)      | pos[2] == 0011'0011
```

### 😊 One-line patterns:
It is the friendly and easy-to-use type of pattern.  
It has no customizations, just one line pattern:
```cpp
const void* const found = Sig::find(buf, size, "AA BB ?? DD 1 2 ? 4 5 6");
```
Tokens `??` and `?` have the same meaning: any byte.

## Usage:
Just include the `./include/Sig/Sig.hpp` and you're good to go!
```cpp
#include <Sig/Sig.hpp>

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