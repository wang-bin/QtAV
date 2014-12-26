/******************************************************************************
    mkid: map chars to int at build time. Template based. A replacement of FourCC
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
******************************************************************************/

#ifndef MKID_H
#define MKID_H

/*!
 * Example:
 * int id1 = mkid::fourcc<'H', 'E', 'V', 'C'>::value;
 * int id2 = mkid::id32base64_5<'H', 'e', 'l', 'l', 'o'>::value;
 * int id3 = mkid::id32base36_6<'M', 'r', 'W', 'a', 'n', 'g'>::value;
 * For (u)int32 result, base 64 accepts at most 5 characters, while base 36 accepts at most 6 characters.
 */
namespace mkid {
namespace detail {
template<int base, int x> struct map_base;
} // namespace internal

template<int base, int a0>
struct id32_1 { enum { value = detail::map_base<base, a0>::value};};
template<int base, int a0, int a1>
struct id32_2 { enum { value = id32_1<base, a0>::value*base + detail::map_base<base, a1>::value};};
template<int base, int a0, int a1, int a2>
struct id32_3 { enum { value = id32_2<base, a0, a1>::value*base + detail::map_base<base, a2>::value};};
template<int base, int a0, int a1, int a2, int a3>
struct id32_4 { enum { value = id32_3<base, a0, a1, a2>::value*base + detail::map_base<base, a3>::value};};
template<int base, int a0, int a1, int a2, int a3, int a4>
struct id32_5 { enum { value = id32_4<base, a0, a1, a2, a3>::value*base + detail::map_base<base, a4>::value};};
template<int base, int a0, int a1, int a2, int a3, int a4, int a5>
struct id32_6 { enum { value = id32_5<base, a0, a1, a2, a3, a4>::value*base + detail::map_base<base, a5>::value};};

// template based FourCC
template<int a0, int a1, int a2, int a3>
struct fourcc : public id32_4<256, a0, a1, a2, a3>{};

// a0~a4: '0'~'9', 'A'~'Z', 'a'~'z', '_', '.' ==>> [0,63]
template<int a0>
struct id32base64_1 : public id32_1<64, a0>{};
template<int a0, int a1>
struct id32base64_2 : public id32_2<64, a0, a1>{};
template<int a0, int a1, int a2>
struct id32base64_3 : public id32_3<64, a0, a1, a2>{};
template<int a0, int a1, int a2, int a3>
struct id32base64_4 : public id32_4<64, a0, a1, a2, a3>{};
template<int a0, int a1, int a2, int a3, int a4>
struct id32base64_5 : public id32_5<64, a0, a1, a2, a3, a4>{};

// a0~a5: '0'~'9', 'A'~'Z', 'a'~'z' ==>> [0,63], upper and lower letters are equal
template<int a0>
struct id32base36_1 : public id32_1<36, a0>{};
template<int a0, int a1>
struct id32base36_2 : public id32_2<36, a0, a1>{};
template<int a0, int a1, int a2>
struct id32base36_3 : public id32_3<36, a0, a1, a2>{};
template<int a0, int a1, int a2, int a3>
struct id32base36_4 : public id32_4<36, a0, a1, a2, a3>{};
template<int a0, int a1, int a2, int a3, int a4>
struct id32base36_5 : public id32_5<36, a0, a1, a2, a3, a4>{};
template<int a0, int a1, int a2, int a3, int a4, int a5>
struct id32base36_6 : public id32_6<36, a0, a1, a2, a3, a4, a5>{};

namespace detail {
////////////////////////////Details////////////////////////////
template<bool b, typename T1, typename T2> struct if_then_else;
template<typename T1, typename T2> struct if_then_else<true, T1, T2> { typedef T1 Type;};
template<typename T1, typename T2> struct if_then_else<false, T1, T2> { typedef T2 Type;};
template<int c> struct is_lower_letter { enum { value = c >= 97 && c <= 122 };};
template<int c> struct is_upper_letter { enum { value = c >= 65 && c <= 90 };};
template<int c> struct is_num { enum { value = c >= 48 && c <= 57 };};
template<int c> struct is_underline { enum { value = c == 95 }; };
template<int c> struct is_dot { enum { value = c == 46 };};
struct lower_letter_t; struct upper_letter_t; struct number_t; struct underline_t; struct dot_t;
template<int x, typename T> struct map64_helper;
template<int x> struct map64_helper<x, number_t> { enum { value = x-48}; };
template<int x> struct map64_helper<x, upper_letter_t> { enum { value = x-65+10}; };
template<int x> struct map64_helper<x, lower_letter_t> { enum { value = x-97+10+26}; };
template<int x> struct map64_helper<x, underline_t> { enum { value = 62}; };
template<int x> struct map64_helper<x, dot_t> { enum { value = 63}; };
struct invalid_char_must_be_number_26letters_underline_dot;
template<int x> struct map_base<64, x> {
    enum {
        value = map64_helper<x,
        typename if_then_else<is_num<x>::value, number_t,
        typename if_then_else<is_upper_letter<x>::value, upper_letter_t,
        typename if_then_else<is_lower_letter<x>::value, lower_letter_t,
        typename if_then_else<is_underline<x>::value, underline_t,
        typename if_then_else<is_dot<x>::value, dot_t,
        invalid_char_must_be_number_26letters_underline_dot>::Type>::Type>::Type>::Type>::Type>::value
    };
};
template<int x, typename T> struct map36_helper;
template<int x> struct map36_helper<x, number_t> { enum { value = x-48}; };
template<int x> struct map36_helper<x, upper_letter_t> { enum { value = x-65+10}; };
template<int x> struct map36_helper<x, lower_letter_t> { enum { value = x-97+10}; };
struct invalid_char_must_be_number_26letters;
template<int x> struct map_base<36, x> {
    enum {
        value = map36_helper<x,
        typename if_then_else<is_num<x>::value, number_t,
        typename if_then_else<is_upper_letter<x>::value, upper_letter_t,
        typename if_then_else<is_lower_letter<x>::value, lower_letter_t,
        invalid_char_must_be_number_26letters>::Type>::Type>::Type>::value
    };
};
// for FourCC
template<int> struct out_of_lower_bound {};
template<int> struct out_of_upper_bound {};
struct map_identical_t;
template<int x, typename T> struct map_identical_helper;
template<int x> struct map_identical_helper<x,map_identical_t>{enum{value=x};};
template<int x> struct map_base<256, x> {
    enum {
        value = map_identical_helper<x,
        typename if_then_else<(x<0), out_of_lower_bound<0>,
        typename if_then_else<(x>255), out_of_upper_bound<255>,
        map_identical_t>::Type>::Type>::value
    };
};
} // namespace detail
} // namespace mkid
#endif // MKID_H
