#pragma once

#include <cstring>


#if (__cplusplus >= 202002) || _HAS_CXX20
#   define sig_has_cxx20  (1)
#else
#   define sig_has_cxx20  (0)
#endif


struct Sig
{
    enum class Tag
    {
        val,
        any,
        pkg,
        str,
        raw,
        rep,
        set,
        range,
        compound
    };


    struct Cmp
    {
        template <typename Type, Type val>
        struct Eq
        {
            static bool cmp(const void* const pos)
            {
                return *static_cast<const Type*>(pos) == val;
            }
        };

        template <typename Type, Type val>
        struct Gr
        {
            static bool cmp(const void* const pos)
            {
                return *static_cast<const Type*>(pos) > val;
            }
        };

        template <typename Type, Type val>
        struct GrEq
        {
            static bool cmp(const void* const pos)
            {
                return *static_cast<const Type*>(pos) >= val;
            }
        };

        template <typename Type, Type val>
        struct Le
        {
            static bool cmp(const void* const pos)
            {
                return *static_cast<const Type*>(pos) < val;
            }
        };

        template <typename Type, Type val>
        struct LeEq
        {
            static bool cmp(const void* const pos)
            {
                return *static_cast<const Type*>(pos) <= val;
            }
        };

        template <typename Type, Type val>
        struct NotEq
        {
            static bool cmp(const void* const pos)
            {
                return *static_cast<const Type*>(pos) != val;
            }
        };

        template <typename Type, Type val>
        struct OneOf
        {
            static bool cmp(const void* const pos)
            {
                return (*static_cast<const Type*>(pos) & val) != 0;
            }
        };

        template <typename Type, Type val>
        struct AllOf
        {
            static bool cmp(const void* const pos)
            {
                return (*static_cast<const Type*>(pos) & val) == val;
            }
        };
    };

    template <typename... Type>
    struct PackageHolder
    {
    };


    template <typename Type, template <typename BaseType, BaseType val> typename Comparator, Type... values>
    struct Holder
    {
        using BaseType = Type;
        using Package = PackageHolder<Holder<Type, Comparator, values>...>;

        static constexpr auto k_tag = Tag::pkg;
        static constexpr auto k_count = sizeof...(values);
        static constexpr auto k_size = k_count * sizeof(Type);
    };
    static_assert(Holder<int, Cmp::Eq, 1, 2, 3>::k_tag == Tag::pkg);

    template <typename Type, template <typename BaseType, BaseType val> typename Comparator>
    struct Holder<Type, Comparator>
    {
        using BaseType = Type;

        static constexpr auto k_tag = Tag::any;
        static constexpr auto k_count = 1;
        static constexpr auto k_size = sizeof(Type);
    };
    static_assert(Holder<int, Cmp::Eq>::k_tag == Tag::any);

    template <typename Type, template <typename BaseType, BaseType val> typename Comparator, Type val>
    struct Holder<Type, Comparator, val>
    {
        using BaseType = Type;
        using Cmp = Comparator<Type, val>;

        static constexpr auto k_tag = Tag::val;
        static constexpr auto k_count = 1;
        static constexpr auto k_size = sizeof(Type);
    };
    static_assert(Holder<int, Cmp::Eq, 1>::k_tag == Tag::val);

    template <typename Type, size_t count = 1>
    struct RawCmp
    {
        using BaseType = Type;

        static constexpr auto k_tag = Tag::raw;
        static constexpr auto k_count = count;
        static constexpr auto k_size = sizeof(Type) * k_count;
    };

    template <typename Repeatable, size_t count>
    struct Rep
    {
        using Type = Repeatable;
        static constexpr auto k_tag = Tag::rep;
        static constexpr auto k_count = count;
        static constexpr auto k_size = k_count * Type::k_size;
    };

    template <typename... Entries>
    struct Set
    {
        template <auto val, auto... values>
        static constexpr auto maxof()
        {
            constexpr decltype(val) k_arr[sizeof...(values) + 1]{ val, values... };
            auto maxval = k_arr[0];
            for (size_t i = 1; i < sizeof...(values); ++i)
            {
                if (k_arr[i] > maxval)
                {
                    maxval = k_arr[i];
                }
            }
            return maxval;
        }

        static constexpr auto k_tag = Tag::set;
        static constexpr auto k_size = maxof<Entries::k_size...>();
        static constexpr auto k_count = sizeof...(Entries);
    };

    template <template <auto> typename Entry, auto from, auto to>
    struct Range
    {
        using BaseType = typename Entry<from>::BaseType;

        template <BaseType val>
        using EntryType = Entry<val>;

        static constexpr auto k_from = from;
        static constexpr auto k_to = to;

        static constexpr auto k_tag = Tag::range;
        static constexpr auto k_size = Entry<from>::k_size;
    };

    template <typename... Entries>
    struct Compound
    {
        static constexpr auto k_tag = Tag::compound;
        static constexpr auto k_size = (Entries::k_size + ...);
        static constexpr auto k_count = sizeof...(Entries);
    };


#if sig_has_cxx20
    template <typename T, size_t len>
    struct Arr
    {
        using Type = T;
        static constexpr auto k_len = len;
        Type buf[k_len]{};
    };

    template <typename T, size_t len>
    struct String
    {
        using Type = T;
        static constexpr auto k_len = len;

        Arr<Type, k_len> str;

        consteval String(const Type* const string)
        {
            for (size_t i = 0; i < k_len; ++i)
            {
                str.buf[i] = string[i];
            }
        }
    };

    template <typename Type, size_t len>
    String(const Type(&)[len])->String<Type, len - 1>;

    template <String string>
    struct Str
    {
        using Type = typename decltype(string)::Type;
        static constexpr auto k_tag = Tag::str;
        static constexpr auto k_size = sizeof(Type) * string.k_len;
        static constexpr auto k_count = string.k_len;
    };
#endif

    template <typename... Entries>
    struct Comparator;

    struct PackedCmp
    {
        template <template <typename...> typename Package, typename... Entries>
        static bool cmp(const Package<Entries...>&, const void* const pos)
        {
            return Comparator<Entries...>::cmp(pos);
        }
    };

    template <typename Rep>
    struct RepCmp
    {
        static bool cmp(const void* const pos)
        {
            for (size_t i = 0; i < Rep::k_count; ++i)
            {
                const bool matches = Comparator<typename Rep::Type>::cmp(&static_cast<const typename Rep::Type::BaseType*>(pos)[i]);
                if (!matches)
                {
                    return false;
                }
            }

            return true;
        }
    };

    template <typename... Entries>
    struct SetComparator;

    template <typename Entry, typename... Entries>
    struct SetComparator<Entry, Entries...>
    {
        static size_t equals(const void* const pos)
        {
            const bool matches = Comparator<Entry>::cmp(pos);
            if (matches)
            {
                return Entry::k_size;
            }
            return SetComparator<Entries...>::equals(pos);
        }
    };



    struct SetCmp
    {
        template <template <typename...> typename Set, typename... Entries>
        static size_t cmp(const Set<Entries...>&, const void* const pos)
        {
            return SetComparator<Entries...>::equals(pos);
        }
    };

    struct RangeCmp
    {
        template <template <auto> typename Entry, auto val, auto end>
        static bool equals(const void* const pos)
        {
            using Cmp = typename Entry<val>::Cmp;

            constexpr auto from = val;
            constexpr auto to = end;

            if constexpr (from < to)
            {
                const bool matches = Cmp::cmp(pos);
                return matches || equals<Entry, from + 1, to>(pos);
            }
            else
            {
                return Cmp::cmp(pos);
            }
        }

        template <typename Range>
        static bool cmp(const void* const pos)
        {
            const auto from = Range::k_from;
            const auto to = Range::k_to;
            return equals<Range::template EntryType, Range::k_from, Range::k_to>(pos);
        }
    };

    struct CompoundCmp
    {
        template <template <typename...> typename Compound, typename... Entries>
        static bool cmp(const Compound<Entries...>&, const void* const pos)
        {
            return Comparator<Entries...>::cmp(pos);
        }
    };

    template <typename Entry, typename... Entries>
    struct Comparator<Entry, Entries...>
    {
        static bool cmp(const void* const pos)
        {
            if constexpr ((Entry::k_tag == Tag::val) || (Entry::k_tag == Tag::raw) || (Entry::k_tag == Tag::str))
            {
                const bool matches = Comparator<Entry>::cmp(pos);
                return matches && Comparator<Entries...>::cmp(static_cast<const unsigned char*>(pos) + Entry::k_size);
            }
            else if constexpr (Entry::k_tag == Tag::pkg)
            {
                const bool matches = PackedCmp::cmp(typename Entry::Package{}, pos);
                return matches && Comparator<Entries...>::cmp(static_cast<const unsigned char*>(pos) + Entry::k_size);
            }
            else if constexpr (Entry::k_tag == Tag::rep)
            {
                const bool matches = RepCmp<Entry>::cmp(pos);
                return matches && Comparator<Entries...>::cmp(static_cast<const unsigned char*>(pos) + Entry::k_size);
            }
            else if constexpr (Entry::k_tag == Tag::set)
            {
                const size_t size = SetCmp::cmp(Entry{}, pos);
                return size && Comparator<Entries...>::cmp(static_cast<const unsigned char*>(pos) + size);
            }
            else if constexpr (Entry::k_tag == Tag::range)
            {
                const bool matches = RangeCmp::cmp<Entry>(pos);
                return matches && Comparator<Entries...>::cmp(static_cast<const unsigned char*>(pos) + Entry::k_size);
            }
            else if constexpr (Entry::k_tag == Tag::compound)
            {
                const bool matches = CompoundCmp::cmp(Entry{}, pos);
                return matches && Comparator<Entries...>::cmp(static_cast<const unsigned char*>(pos) + Entry::k_size);
            }
            else
            {
                return Comparator<Entries...>::cmp(static_cast<const unsigned char*>(pos) + Entry::k_size);
            }
        }
    };

    template <typename Entry>
    struct Comparator<Entry>
    {
        static bool cmp(const void* const pos)
        {
            if constexpr (Entry::k_tag == Tag::val)
            {
                using Cmp = typename Entry::Cmp;
                return Cmp::cmp(pos);
            }
            else if constexpr (Entry::k_tag == Tag::pkg)
            {
                return PackedCmp::cmp(typename Entry::Package{}, pos);
            }
            else if constexpr (Entry::k_tag == Tag::raw)
            {
                return Entry::cmp(pos);
            }
            else if constexpr (Entry::k_tag == Tag::str)
            {
                return Entry::cmp(pos);
            }
            else if constexpr (Entry::k_tag == Tag::rep)
            {
                return RepCmp<Entry>::cmp(pos);
            }
            else if constexpr (Entry::k_tag == Tag::set)
            {
                return SetCmp::cmp(Entry{}, pos) != 0;
            }
            else if constexpr (Entry::k_tag == Tag::range)
            {
                return RangeCmp::cmp<Entry>(pos);
            }
            else if constexpr (Entry::k_tag == Tag::compound)
            {
                return CompoundCmp::cmp(Entry{}, pos);
            }
            else if constexpr (Entry::k_tag == Tag::any)
            {
                return true;
            }
            else
            {
                static_assert("Invalid entry");
            }
        }
    };


    // Pattern format in a template way:
    // Sig::find<Sig::Byte<0x11, 0x22>, Sig::Char<'t', 'e', 'x', 't'>, Sig::Dword<>, Sig::Byte<0xFF>>(arr, sizeof(arr));
    template <typename... Entries>
    static const void* find(const void* const buf, const size_t size)
    {
        constexpr auto k_patternSize = (Entries::k_size + ...);

        const auto* pos = static_cast<const unsigned char*>(buf);
        const auto* const end = static_cast<const unsigned char*>(buf) + size - k_patternSize + 1;
        while (pos < end)
        {
            const bool equals = Comparator<Entries...>::cmp(pos);
            if (equals)
            {
                return pos;
            }

            ++pos;
        }

        return nullptr;
    }


#if sig_has_cxx20
    template <String str>
    struct StrEq : Str<str>
    {
        static bool cmp(const void* const pos)
        {
            const auto* const mem = static_cast<const typename decltype(str)::Type*>(pos);
            for (size_t i = 0; i < decltype(str)::k_len; ++i)
            {
                if (mem[i] != str.str.buf[i])
                {
                    return false;
                }
            }

            return true;
        }
    };

    template <String str>
    struct StrEqNoCase : Str<str>
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
#endif


    template <typename Type, Type value, Type mask>
    struct BitMask : RawCmp<Type>
    {
        static bool cmp(const void* const pos)
        {
            return (*static_cast<const Type*>(pos) & mask) == (value & mask);
        }
    };


    template <template <typename BaseType, BaseType val> typename Comparator, char... values>
    using CmpChar = Holder<char, Comparator, values...>;

    template <char... values>
    using Char = CmpChar<Cmp::Eq, values...>;


    template <template <typename BaseType, BaseType val> typename Comparator, wchar_t... values>
    using CmpWChar = Holder<wchar_t, Comparator, values...>;

    template <wchar_t... values>
    using WChar = CmpWChar<Cmp::Eq, values...>;


    template <template <typename BaseType, BaseType val> typename Comparator, unsigned char... values>
    using CmpByte = Holder<unsigned char, Comparator, values...>;

    template <unsigned char... values>
    using Byte = CmpByte<Cmp::Eq, values...>;


    template <template <typename BaseType, BaseType val> typename Comparator, unsigned short... values>
    using CmpWord = Holder<unsigned short, Comparator, values...>;

    template <unsigned short... values>
    using Word = CmpWord<Cmp::Eq, values...>;


    template <template <typename BaseType, BaseType val> typename Comparator, unsigned int... values>
    using CmpDword = Holder<unsigned int, Comparator, values...>;

    template <unsigned int... values>
    using Dword = CmpDword<Cmp::Eq, values...>;


    template <template <typename BaseType, BaseType val> typename Comparator, unsigned long long... values>
    using CmpQword = Holder<unsigned long long, Comparator, values...>;

    template <unsigned long long... values>
    using Qword = CmpQword<Cmp::Eq, values...>;


    template <template <typename BaseType, BaseType val> typename Comparator, short... values>
    using CmpShort = Holder<short, Comparator, values...>;

    template <short... values>
    using Short = CmpShort<Cmp::Eq, values...>;


    template <template <typename BaseType, BaseType val> typename Comparator, unsigned short... values>
    using CmpUShort = Holder<unsigned short, Comparator, values...>;

    template <unsigned short... values>
    using UShort = CmpUShort<Cmp::Eq, values...>;


    template <template <typename BaseType, BaseType val> typename Comparator, long... values>
    using CmpLong = Holder<long, Comparator, values...>;

    template <long... values>
    using Long = CmpLong<Cmp::Eq, values...>;


    template <template <typename BaseType, BaseType val> typename Comparator, unsigned long... values>
    using CmpULong = Holder<unsigned long, Comparator, values...>;

    template <unsigned long... values>
    using ULong = CmpULong<Cmp::Eq, values...>;


    template <template <typename BaseType, BaseType val> typename Comparator, long long... values>
    using CmpLongLong = Holder<long long, Comparator, values...>;

    template <long long... values>
    using LongLong = CmpLongLong<Cmp::Eq, values...>;


    template <template <typename BaseType, BaseType val> typename Comparator, unsigned long long... values>
    using CmpULongLong = Holder<unsigned long long, Comparator, values...>;

    template <unsigned long long... values>
    using ULongLong = CmpULongLong<Cmp::Eq, values...>;


    template <template <typename BaseType, BaseType val> typename Comparator, int... values>
    using CmpInt = Holder<int, Comparator, values...>;

    template <int... values>
    using Int = CmpInt<Cmp::Eq, values...>;


    template <template <typename BaseType, BaseType val> typename Comparator, unsigned int... values>
    using CmpUInt = Holder<unsigned int, Comparator, values...>;

    template <unsigned int... values>
    using UInt = CmpUInt<Cmp::Eq, values...>;


    template <template <typename BaseType, BaseType val> typename Comparator, long long... values>
    using CmpInt64 = Holder<long long, Comparator, values...>;

    template <long long... values>
    using Int64 = CmpInt64<Cmp::Eq, values...>;


    template <template <typename BaseType, BaseType val> typename Comparator, unsigned long long... values>
    using CmpUInt64 = Holder<unsigned long long, Comparator, values...>;

    template <unsigned long long... values>
    using UInt64 = CmpUInt64<Cmp::Eq, values...>;



    template <unsigned char value, unsigned char mask>
    using ByteMask = BitMask<unsigned char, value, mask>;

    template <unsigned short value, unsigned short mask>
    using WordMask = BitMask<unsigned short, value, mask>;

    template <unsigned int value, unsigned int mask>
    using DwordMask = BitMask<unsigned int, value, mask>;

    template <unsigned long long value, unsigned long long mask>
    using QwordMask = BitMask<unsigned long long, value, mask>;



    struct Mask
    {
        enum class CmpType
        {
            basic,
            extended
        };

        template <char ch>
        struct MaskCmp
        {
            static constexpr auto k_char = ch;
            static constexpr auto k_type = CmpType::basic;
        };

        template <char ch>
        struct MaskCmpEx
        {
            static constexpr auto k_char = ch;
            static constexpr auto k_type = CmpType::extended;
        };

        template <char ch>
        struct Eq : MaskCmp<ch>
        {
            static bool cmp(const char data, const char pattern)
            {
                return data == pattern;
            }
        };

        template <char ch>
        struct NotEq : MaskCmp<ch>
        {
            static bool cmp(const char data, const char pattern)
            {
                return data != pattern;
            }
        };

        template <char ch>
        struct Gr : MaskCmp<ch>
        {
            static bool cmp(const char data, const char pattern)
            {
                return data > pattern;
            }
        };

        template <char ch>
        struct GrEq : MaskCmp<ch>
        {
            static bool cmp(const char data, const char pattern)
            {
                return data >= pattern;
            }
        };

        template <char ch>
        struct Le : MaskCmp<ch>
        {
            static bool cmp(const char data, const char pattern)
            {
                return data < pattern;
            }
        };

        template <char ch>
        struct LeEq : MaskCmp<ch>
        {
            static bool cmp(const char data, const char pattern)
            {
                return data <= pattern;
            }
        };

        template <char ch>
        struct OneOf : MaskCmp<ch>
        {
            static bool cmp(const char data, const char pattern)
            {
                return (data & pattern) != 0;
            }
        };

        template <char ch>
        struct AllOf : MaskCmp<ch>
        {
            static bool cmp(const char data, const char pattern)
            {
                return (data & pattern) == pattern;
            }
        };

        template <char ch>
        struct BitMask : MaskCmpEx<ch>
        {
            static bool cmp(const char data, const char pattern, const char subpattern)
            {
                return (data & subpattern) == (pattern & subpattern);
            }
        };

        template <char ch>
        struct Any : MaskCmp<ch>
        {
            static bool cmp(const char, const char)
            {
                return true;
            }
        };
    };

    template <typename... Entries>
    struct MaskComparator
    {
    };

    template <typename Entry, typename... Entries>
    struct MaskComparator<Entry, Entries...>
    {
        static bool cmp(const char data, const char pattern, const char mask)
        {
            if (mask == Entry::k_char)
            {
                return Entry::cmp(data, pattern);
            }
            else
            {
                return MaskComparator<Entries...>::cmp(data, pattern, mask);
            }
        }

        static bool cmp(const char data, const char pattern, const char subpattern, const char mask)
        {
            if (mask == Entry::k_char)
            {
                if constexpr (Entry::k_type == Mask::CmpType::basic)
                {
                    return Entry::cmp(data, pattern);
                }
                else if constexpr (Entry::k_type == Mask::CmpType::extended)
                {
                    return Entry::cmp(data, pattern, subpattern);
                }
                else
                {
                    static_assert("Unknown type of comparator");
                }
            }
            else
            {
                return MaskComparator<Entries...>::cmp(data, pattern, subpattern, mask);
            }
        }
    };



    // Pattern format: "\x11\x2\x00text" + "..?....", meaning of mask chars is customizable by Sig:Mask::* types
    template <typename... Comparators>
    static const void* find(const void* const buf, const size_t size, const char* const sig, const char* const mask, const size_t sigsize)
    {
        if (!size || !sig || !mask)
        {
            return nullptr;
        }

        const auto* pos = static_cast<const unsigned char*>(buf);
        const auto* const end = static_cast<const unsigned char*>(buf) + size - sigsize + 1;
        while (pos < end)
        {
            bool result = true;
            for (size_t i = 0; i < sigsize; ++i)
            {
                const bool matches = MaskComparator<Comparators...>::cmp(pos[i], sig[i], mask[i]);
                result &= matches;
                if (!result)
                {
                    break;
                }
            }

            if (result)
            {
                return pos;
            }

            ++pos;
        }

        return nullptr;
    }

    // Pattern format: "\x11\x2\x00text" + "..?....", meaning of mask chars is customizable by Sig::Mask::* types
    template <typename... Comparators>
    static const void* find(const void* const buf, const size_t size, const char* const sig, const char* const mask)
    {
        if (!mask)
        {
            return nullptr;
        }

        return find<Comparators...>(buf, size, sig, mask, strlen(mask));
    }

    // Pattern format: "\x11\x2\x00text" + nullptr/"....\x1C\x03." + "..?.mm.", meaning of mask chars is customizable by Sig:Mask::* types
    template <typename... Comparators>
    static const void* find(const void* const buf, const size_t size, const char* const sig, const char* const subsig, const char* const mask, const size_t sigsize)
    {
        if (!sig || !subsig || !mask)
        {
            return nullptr;
        }

        const auto* pos = static_cast<const unsigned char*>(buf);
        const auto* const end = static_cast<const unsigned char*>(buf) + size - sigsize + 1;
        while (pos < end)
        {
            bool result = true;
            for (size_t i = 0; i < sigsize; ++i)
            {
                const bool matches = MaskComparator<Comparators...>::cmp(pos[i], sig[i], subsig[i], mask[i]);
                result &= matches;
                if (!result)
                {
                    break;
                }
            }

            if (result)
            {
                return pos;
            }

            ++pos;
        }

        return nullptr;
    }

    // Pattern format: "\x11\x2\x00text" + "..?....", meaning of mask chars is customizable by Sig::Mask::* types
    template <typename... Comparators>
    static const void* find(const void* const buf, const size_t size, const char* const sig, const char* const subsig, const char* const mask)
    {
        if (!mask)
        {
            return nullptr;
        }

        return find<Comparators...>(buf, size, sig, subsig, mask, strlen(mask));
    }

    // Pattern format: sig ("\x0D\xCB\xFF") + valuable bits in the sig that must match ("\x0D\xFF\x03")
    static const void* bitmask(const void* const buf, const size_t size, const void* const sig, const void* const mask, size_t sigsize)
    {
        if (!sig || !mask || !sigsize)
        {
            return nullptr;
        }

        const auto* const val = static_cast<const unsigned char*>(sig);
        const auto* const msk = static_cast<const unsigned char*>(mask);

        const auto* pos = static_cast<const unsigned char*>(buf);
        const auto* const end = static_cast<const unsigned char*>(buf) + size - sigsize + 1;
        while (pos < end)
        {
            bool result = true;
            for (size_t i = 0; i < sigsize; ++i)
            {
                const bool matches = ((pos[i] & msk[i]) == (val[i] & msk[i]));
                result &= matches;
                if (!result)
                {
                    break;
                }
            }

            if (result)
            {
                return pos;
            }

            ++pos;
        }

        return nullptr;
    }

    // Pattern format: "11 22 ? 44 ?? ?? 66 aa bB Cc DD ee FF" ('?' and '??' have the same meaning: any byte)
    static const void* find(const void* const buf, const size_t size, const char* const sig)
    {
        if (!sig)
        {
            return nullptr;
        }

        const auto skipSpace = [](const char* const str) -> const char*
        {
            const char* pos = str;
            while ((*pos == ' ') || (*pos == '\t'))
            {
                ++pos;
            }

            return pos;
        };

        const auto calcTokenLen = [](const char* const str) -> size_t
        {
            size_t size = 0;
            while ((str[size]) && (str[size] != ' ') && (str[size] != '\t'))
            {
                ++size;
            }
            return size;
        };

        const auto calcSigBytes = [&skipSpace, &calcTokenLen](const char* const sig) -> size_t
        {
            const char* pos = sig;
            size_t bytes = 0;

            const auto isHexChar = [](const char ch) -> bool
            {
                return ((ch >= '0') && (ch <= '9')) || ((ch >= 'A') && (ch <= 'F')) || ((ch >= 'a') && (ch <= 'f'));
            };

            while (*(pos = skipSpace(pos)) != '\0')
            {
                ++bytes;
                const size_t entryLen = calcTokenLen(pos);
                constexpr auto k_maxTokenSize = (sizeof("xx") - sizeof('\0'));
                if (entryLen > k_maxTokenSize)
                {
                    return 0;
                }

                if (entryLen == k_maxTokenSize)
                {
                    if ((pos[0] == '?') || (pos[1] == '?'))
                    {
                        if (pos[0] != pos[1])
                        {
                            return 0;
                        }
                    }
                    else
                    {
                        if (!isHexChar(pos[0]) || !isHexChar(pos[1]))
                        {
                            return 0;
                        }
                    }
                }
                else if (entryLen == sizeof('x'))
                {
                    if ((pos[0] != '?') && !isHexChar(pos[0]))
                    {
                        return 0;
                    }
                }

                pos += entryLen;
            }

            return bytes;
        };

        const auto testPattern = [&skipSpace, &calcTokenLen](const void* const buf, const char* const sig) -> bool
        {
            const char* sigPos = sig;
            const unsigned char* bufPos = static_cast<const unsigned char*>(buf);

            const auto tokenToByte = [](const char* const token, size_t size) -> unsigned char
            {
                const auto charToByte = [](const char ch) -> unsigned char
                {
                    if (ch >= '0' && ch <= '9')
                    {
                        return static_cast<unsigned char>(ch - '0');
                    }

                    if (ch >= 'A' && ch <= 'F')
                    {
                        return static_cast<unsigned char>(ch - 'A' + 10);
                    }

                    if (ch >= 'a' && ch <= 'f')
                    {
                        return static_cast<unsigned char>(ch - 'a' + 10);
                    }

                    return 0;
                };

                if (size == 2)
                {
                    return (charToByte(token[0]) << 4) | (charToByte(token[1]));
                }
                else if (size == 1)
                {
                    return charToByte(token[0]);
                }
                else
                {
                    return 0;
                }
            };

            while (*(sigPos = skipSpace(sigPos)) != '\0')
            {
                const size_t tokenSize = calcTokenLen(sigPos);

                if ((*sigPos) == '?')
                {
                    sigPos += tokenSize;
                    ++bufPos;
                    continue;
                }

                const auto tokenByte = tokenToByte(sigPos, tokenSize);
                if (*bufPos != tokenByte)
                {
                    return false;
                }

                sigPos += tokenSize;
                ++bufPos;
            }

            return true;
        };

        const size_t sigBytes = calcSigBytes(sig);
        if (!sigBytes)
        {
            return nullptr;
        }

        const auto* pos = static_cast<const unsigned char*>(buf);
        const auto* const end = static_cast<const unsigned char*>(buf) + size - sigBytes + 1;

        while (pos < end)
        {
            const bool matches = testPattern(pos, sig);
            if (matches)
            {
                return pos;
            }
            ++pos;
        }

        return nullptr;
    }
};

template <>
struct Sig::Comparator<>
{
    static bool cmp(const void* const)
    {
        return true;
    }
};


template <>
struct Sig::SetComparator<>
{
    static size_t equals(const void*)
    {
        return 0;
    }
};

template <>
struct Sig::MaskComparator<>
{
    static bool cmp(const char, const char, const char)
    {
        return false;
    }

    static bool cmp(const char, const char, const char, const char)
    {
        return false;
    }
};

#undef sig_has_cxx20