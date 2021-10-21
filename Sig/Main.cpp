#include <Sig/Sig.hpp>

#include <cstdio>
#include <intrin.h>

#define sig_assert(cond) if (!(cond)) { __int2c(); }


namespace
{
    const unsigned char g_arr[]
    {
        /* 00 */ '?', '?', '?', '?',
        /* 04 */ 1, 2, 2, 3, 3, 3, 4, 4, 4, 4,
        /* 14 */ 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
        /* 24 */ 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
        /* 30 */ 'r', 'r', 'r', 'r', 'r', 'r',
        /* 36 */ 't', 'e', 'x', 't',
        /* 40 */ 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        /* 48 */ 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
        /* 56 */ 0x0F, 0x05, 0xE9, '?', '?', '?', '?', 0xC3
    };

    namespace TemplateTests
    {
        template <typename Type, Type val>
        struct CustomCmp
        {
            static bool cmp(const void* const pos)
            {
                return *static_cast<const Type*>(pos) == (val / 2);
            }
        };

        template <typename Type, Type... vals>
        using Custom = Sig::Holder<Type, CustomCmp, vals...>;

        void runTests()
        {
            const void* found = nullptr;

            found = Sig::find<Sig::Byte<>>(g_arr, sizeof(g_arr));
            sig_assert(found == &g_arr[0]);

            found = Sig::find<Sig::Byte<>, Sig::Byte<>>(g_arr, sizeof(g_arr));
            sig_assert(found == &g_arr[0]);

            found = Sig::find<Sig::Byte<>>(g_arr, sizeof(unsigned char));
            sig_assert(found == &g_arr[0]);

            found = Sig::find<Sig::Byte<>>(g_arr, 0);
            sig_assert(found == nullptr);

            found = Sig::find<Sig::Dword<>>(g_arr, 0);
            sig_assert(found == nullptr);

            found = Sig::find<Sig::Dword<>>(g_arr, sizeof(unsigned char));
            sig_assert(found == nullptr);

            found = Sig::find<Sig::Dword<>>(g_arr, sizeof(unsigned long));
            sig_assert(found == &g_arr[0]);

            found = Sig::find<Sig::Byte<1>>(g_arr, sizeof(g_arr));
            sig_assert(found == &g_arr[4]);

            found = Sig::find<Sig::Byte<1, 2, 2, 3, 3, 3>>(g_arr, sizeof(g_arr));
            sig_assert(found == &g_arr[4]);

            found = Sig::find<Sig::Byte<1, 2, '?', 3, 3, 3>>(g_arr, sizeof(g_arr));
            sig_assert(found == nullptr);

            found = Sig::find<Sig::Byte<1, 2, 2, 3, 3, 3>>(g_arr, 0);
            sig_assert(found == nullptr);

            found = Sig::find<Sig::Byte<1, 2, 2, 3, 3, 3>>(g_arr, 4);
            sig_assert(found == nullptr);

            found = Sig::find<Sig::Byte<1, 2, 2, 3, 3, 3>>(g_arr, 9);
            sig_assert(found == nullptr);

            found = Sig::find<Sig::Byte<1, 2, 2, 3, 3, 3>>(g_arr, 10);
            sig_assert(found == &g_arr[4]);

            found = Sig::find<Sig::Char<'t', 'e', 'x', 't'>>(g_arr, sizeof(g_arr));
            sig_assert(found == &g_arr[36]);

            found = Sig::find<Sig::Rep<Sig::Char<'r'>, 3>>(g_arr, sizeof(g_arr));
            sig_assert(found == &g_arr[30]);

            found = Sig::find<Sig::Rep<Sig::Char<'r'>, 6>>(g_arr, sizeof(g_arr));
            sig_assert(found == &g_arr[30]);

            found = Sig::find<Sig::Rep<Sig::Char<'r'>, 7>>(g_arr, sizeof(g_arr));
            sig_assert(found == nullptr);

            found = Sig::find<Sig::Rep<Sig::Char<'r'>, 6>, Sig::Char<'t', 'e', 'x', 't'>>(g_arr, sizeof(g_arr));
            sig_assert(found == &g_arr[30]);

            found = Sig::find<Sig::Byte<0x10>, Sig::NotByte<0x00, 0x00, 0x00, 0x00>, Sig::Byte<0x15>>(g_arr, sizeof(g_arr));
            sig_assert(found == &g_arr[14]);

            found = Sig::find<Sig::Byte<0x10>, Sig::NotDword<0x00000000>, Sig::Byte<0x15>>(g_arr, sizeof(g_arr));
            sig_assert(found == &g_arr[14]);

            using Syscall = Sig::Compound<Sig::Byte<0x0F, 0x05>>;         // syscall
            using RelJump = Sig::Compound<Sig::Byte<0xE9>, Sig::Dword<>>; // jmp ?
            using Ret     = Sig::Compound<Sig::Byte<0xC3>>;               // ret

            using SamplePattern = Sig::Compound<Syscall, RelJump, Ret>;

            found = Sig::find<SamplePattern>(g_arr, sizeof(g_arr));
            sig_assert(found == &g_arr[56]);

            found = Sig::find<SamplePattern, Sig::Byte<>>(g_arr, sizeof(g_arr));
            sig_assert(found == nullptr);

            found = Sig::find<SamplePattern>(g_arr, sizeof(g_arr) - 1);
            sig_assert(found == nullptr);

            found = Sig::find<Custom<unsigned char, 2, 4, 4, 6, 6, 6>>(g_arr, sizeof(g_arr));
            sig_assert(found == &g_arr[4]);
        }
    }

    namespace MaskTests
    {
        void runTests()
        {
            const void* found = nullptr;

            found = Sig::find(g_arr, sizeof(g_arr), nullptr, nullptr);
            sig_assert(found == nullptr);

            found = Sig::find(g_arr, sizeof(g_arr), "", nullptr);
            sig_assert(found == nullptr);

            found = Sig::find(g_arr, sizeof(g_arr), nullptr, "");
            sig_assert(found == nullptr);

            found = Sig::find(g_arr, sizeof(g_arr), "123", nullptr);
            sig_assert(found == nullptr);

            found = Sig::find(g_arr, sizeof(g_arr), nullptr, "...");
            sig_assert(found == nullptr);

            found = Sig::find(g_arr, sizeof(g_arr), "", "");
            sig_assert(found == &g_arr[0]);

            found = Sig::find(g_arr, sizeof(g_arr), "????", "....");
            sig_assert(found == nullptr);

            found = Sig::find<Sig::Mask::Eq<'.'>, Sig::Mask::Any<'?'>>(g_arr, sizeof(g_arr), "????", "....");
            sig_assert(found == &g_arr[0]);

            found = Sig::find<Sig::Mask::Eq<'.'>, Sig::Mask::Any<'?'>>(g_arr, 0, "????", "....");
            sig_assert(found == nullptr);

            found = Sig::find<Sig::Mask::Eq<'.'>, Sig::Mask::Any<'?'>>(g_arr, sizeof(g_arr), "text", "....");
            sig_assert(found == &g_arr[36]);

            found = Sig::find<Sig::Mask::Eq<'.'>, Sig::Mask::Any<'?'>>(g_arr, sizeof(g_arr), "t_xt", ".?..");
            sig_assert(found == &g_arr[36]);

            found = Sig::find<Sig::Mask::Eq<'.'>, Sig::Mask::Any<'?'>>(g_arr, sizeof(g_arr), "\x01\x02\x00\x03\x03\x00\x00\x04\x04\x04", "..?..??...");
            sig_assert(found == &g_arr[4]);

            found = Sig::find<Sig::Mask::Eq<'.'>, Sig::Mask::Any<'?'>>(g_arr, sizeof(g_arr), "\x0F\x05\xE9xxxx\xC3", "...????.");
            sig_assert(found == &g_arr[56]);

            found = Sig::find<Sig::Mask::Eq<'.'>, Sig::Mask::Any<'?'>>(g_arr, sizeof(g_arr), "\x0F\x05\xE9xxxx\xC3?", "...????.?");
            sig_assert(found == nullptr);

            found = Sig::find<Sig::Mask::Eq<'.'>, Sig::Mask::Any<'?'>>(g_arr, sizeof(g_arr) - 1, "\x0F\x05\xE9xxxx\xC3", "...????.");
            sig_assert(found == nullptr);

            found = Sig::find<
                Sig::Mask::Eq<'.'>,
                Sig::Mask::Any<'?'>,
                Sig::Mask::Gr<'>'>,
                Sig::Mask::Le<'<'>,
                Sig::Mask::OneOf<'o'>,
                Sig::Mask::AllOf<'a'>
            >(g_arr, sizeof(g_arr), "\x01\x01\x00\x02\x03\x00\x00\x04\x04\x04", ".>?oa??...");
            sig_assert(found == &g_arr[4]);

            struct CustomCmp : Sig::Mask::CharHolder<'c'>
            {
                static bool cmp(const char data, const char pattern)
                {
                    return data > (pattern * 2);
                }
            };
            found = Sig::find<
                Sig::Mask::Eq<'.'>,
                CustomCmp
            >(g_arr, sizeof(g_arr), "\x01\x02\x02\x01\x03\x03", "...c..");
            sig_assert(found == &g_arr[4]);

            found = Sig::find<Sig::Mask::Eq<'.'>, Sig::Mask::Any<'?'>>(g_arr, sizeof(g_arr), "\x01\x02\x02\x03\x03\x03", "....u.");
            sig_assert(found == nullptr);
        }
    }

    namespace OneLineTests
    {
        void runTests()
        {
            const void* found = nullptr;

            found = Sig::find(g_arr, sizeof(g_arr), nullptr);
            sig_assert(found == nullptr);

            found = Sig::find(g_arr, sizeof(g_arr), "1 2 2 3 3 3 4 4 4 4");
            sig_assert(found == &g_arr[4]);

            found = Sig::find(g_arr, sizeof(g_arr), "AA bb cC Dd");
            sig_assert(found == &g_arr[50]);

            found = Sig::find(g_arr, sizeof(g_arr), "");
            sig_assert(found == nullptr);

            found = Sig::find(g_arr, sizeof(g_arr), "?");
            sig_assert(found == &g_arr[0]);

            found = Sig::find(g_arr, sizeof(g_arr), "x");
            sig_assert(found == nullptr);

            found = Sig::find(g_arr, sizeof(g_arr), "r");
            sig_assert(found == nullptr);

            found = Sig::find(g_arr, sizeof(g_arr), "0 11 ? ?? 44 55 66 77      88 99 aa bB     Cc dD   EE ?");
            sig_assert(found == &g_arr[40]);

            found = Sig::find(g_arr, sizeof(g_arr), "X 11 ? ?? 44 55 66 77");
            sig_assert(found == nullptr);

            found = Sig::find(g_arr, sizeof(g_arr), "F 5 E9");
            sig_assert(found == &g_arr[56]);

            found = Sig::find(g_arr, sizeof(g_arr), "       c3   ");
            sig_assert(found == &g_arr[63]);
        }
    }

    void runTests()
    {
        TemplateTests::runTests();
        MaskTests::runTests();
        OneLineTests::runTests();
    }
}

int main()
{
    runTests();
    return 0;
}
