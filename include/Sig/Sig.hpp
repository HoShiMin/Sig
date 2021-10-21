#pragma once

#include <string.h>

struct Sig
{
    enum class Tag
    {
        val,
        any,
        pkg,
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

    template <>
    struct SetComparator<>
    {
        static size_t equals(const void*)
        {
            return 0;
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
            if constexpr (Entry::k_tag == Tag::val)
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
                return typename Entry::Cmp::cmp(pos);
            }
            else if constexpr (Entry::k_tag == Tag::pkg)
            {
                return PackedCmp::cmp(typename Entry::Package{}, pos);
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

    template <>
    struct Comparator<>
    {
        static bool cmp(const void* const)
        {
            return true;
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

    template <typename Type, Type... values>
    using EntryType = Holder<Type, Cmp::Eq, values...>;

    template <typename Type, Type... values>
    using NotEntryType = Holder<Type, Cmp::NotEq, values...>;

    template <char... values>
    using Char = EntryType<char, values...>;

    template <wchar_t... values>
    using WChar = EntryType<wchar_t, values...>;

    template <unsigned char... values>
    using Byte = EntryType<unsigned char, values...>;

    template <unsigned short... values>
    using Word = EntryType<unsigned short, values...>;

    template <unsigned int... values>
    using Dword = EntryType<unsigned int, values...>;

    template <unsigned long long... values>
    using Qword = EntryType<unsigned long long, values...>;

    template <short... values>
    using Short = EntryType<short, values...>;

    template <unsigned short... values>
    using UShort = EntryType<unsigned short, values...>;

    template <long... values>
    using Long = EntryType<long, values...>;

    template <unsigned long... values>
    using ULong = EntryType<unsigned long, values...>;

    template <long long... values>
    using LongLong = EntryType<long long, values...>;

    template <unsigned long long... values>
    using ULongLong = EntryType<unsigned long long, values...>;

    template <int... values>
    using Int = EntryType<int, values...>;

    template <unsigned int... values>
    using UInt = EntryType<unsigned int, values...>;

    template <long long... values>
    using Int64 = EntryType<long long, values...>;

    template <unsigned long long... values>
    using UInt64 = EntryType<unsigned long long, values...>;



    template <char... values>
    using NotChar = NotEntryType<char, values...>;

    template <wchar_t... values>
    using NotWChar = NotEntryType<wchar_t, values...>;

    template <unsigned char... values>
    using NotByte = NotEntryType<unsigned char, values...>;

    template <unsigned short... values>
    using NotWord = NotEntryType<unsigned short, values...>;

    template <unsigned int... values>
    using NotDword = NotEntryType<unsigned int, values...>;

    template <unsigned long long... values>
    using NotQword = NotEntryType<unsigned long long, values...>;

    template <short... values>
    using NotShort = NotEntryType<short, values...>;

    template <unsigned short... values>
    using NotUShort = NotEntryType<unsigned short, values...>;

    template <long... values>
    using NotLong = NotEntryType<long, values...>;

    template <unsigned long... values>
    using NotULong = NotEntryType<unsigned long, values...>;

    template <long long... values>
    using NotLongLong = NotEntryType<long long, values...>;

    template <unsigned long long... values>
    using NotULongLong = NotEntryType<unsigned long long, values...>;

    template <int... values>
    using NotInt = NotEntryType<int, values...>;

    template <unsigned int... values>
    using NotUInt = NotEntryType<unsigned int, values...>;

    template <long long... values>
    using NotInt64 = NotEntryType<long long, values...>;

    template <unsigned long long... values>
    using NotUInt64 = NotEntryType<unsigned long long, values...>;





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
    };

    template <>
    struct MaskComparator<>
    {
        static bool cmp(const char, const char, const char)
        {
            return false;
        }
    };

    struct Mask
    {
        template <char ch>
        struct CharHolder
        {
            static constexpr auto k_char = ch;
        };

        template <char ch>
        struct Eq : CharHolder<ch>
        {
            static bool cmp(const char data, const char pattern)
            {
                return data == pattern;
            }
        };

        template <char ch>
        struct NotEq : CharHolder<ch>
        {
            static bool cmp(const char data, const char pattern)
            {
                return data != pattern;
            }
        };

        template <char ch>
        struct Gr : CharHolder<ch>
        {
            static bool cmp(const char data, const char pattern)
            {
                return data > pattern;
            }
        };

        template <char ch>
        struct GrEq : CharHolder<ch>
        {
            static bool cmp(const char data, const char pattern)
            {
                return data >= pattern;
            }
        };

        template <char ch>
        struct Le : CharHolder<ch>
        {
            static bool cmp(const char data, const char pattern)
            {
                return data < pattern;
            }
        };

        template <char ch>
        struct LeEq : CharHolder<ch>
        {
            static bool cmp(const char data, const char pattern)
            {
                return data <= pattern;
            }
        };

        template <char ch>
        struct OneOf : CharHolder<ch>
        {
            static bool cmp(const char data, const char pattern)
            {
                return (data & pattern) != 0;
            }
        };

        template <char ch>
        struct AllOf : CharHolder<ch>
        {
            static bool cmp(const char data, const char pattern)
            {
                return (data & pattern) == pattern;
            }
        };

        template <char ch>
        struct Any : CharHolder<ch>
        {
            static bool cmp(const char, const char)
            {
                return true;
            }
        };
    };

    // Pattern format: "\x11\x2\x00text" + "..?....", meaning of mask chars is customizable by Sig:Mask::* types
    template <typename... Comparators>
    static const void* find(const void* const buf, const size_t size, const char* const sig, const char* const mask, const size_t sigsize)
    {
        if (!sig || !mask)
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
