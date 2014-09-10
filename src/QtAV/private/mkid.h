/******************************************************************************
    mkid: map 5 chars to int
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

struct lower_letter_t;
struct upper_letter_t;
struct number_t;
struct underline_t;
struct dot_t;
template<int x, typename T> struct map64_helper {};
template<int x> struct map64_helper<x, number_t> { enum { value = x-48}; };
template<int x> struct map64_helper<x, upper_letter_t> { enum { value = x-65+10}; };
template<int x> struct map64_helper<x, lower_letter_t> { enum { value = x-97+10+26}; };
template<int x> struct map64_helper<x, underline_t> { enum { value = 62}; };
template<int x> struct map64_helper<x, dot_t> { enum { value = 63}; };

template<bool b, typename T1, typename T2> struct if_then_else;
template<typename T1, typename T2> struct if_then_else<true, T1, T2> { typedef T1 Type;};
template<typename T1, typename T2> struct if_then_else<false, T1, T2> { typedef T2 Type;};
template<int c>
struct is_lower_letter { enum { value = c >= 97 && c <= 122 };};
template<int c>
struct is_upper_letter { enum { value = c >= 65 && c <= 90 };};
template<int c>
struct is_num { enum { value = c >= 48 && c <= 57 };};
template<int c>
struct is_underline { enum { value = c == 95 }; };
template<int c>
struct is_dot { enum { value = c == 46 };};
struct invalid_char_must_be_number_26letters_underline_dot;
template<int base, int x> struct map_base;
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
template<int x, typename T> struct map36_helper {};
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

template<int base, int a0>
struct mkid32_1 { enum { value = map_base<base, a0>::value};};
template<int base, int a0, int a1>
struct mkid32_2 { enum { value = mkid32_1<base, a0>::value*base + map_base<base, a1>::value};};
template<int base, int a0, int a1, int a2>
struct mkid32_3 { enum { value = mkid32_2<base, a0, a1>::value*base + map_base<base, a2>::value};};
template<int base, int a0, int a1, int a2, int a3>
struct mkid32_4 { enum { value = mkid32_3<base, a0, a1, a2>::value*base + map_base<base, a3>::value};};
template<int base, int a0, int a1, int a2, int a3, int a4>
struct mkid32_5 { enum { value = mkid32_4<base, a0, a1, a2, a3>::value*base + map_base<base, a4>::value};};
template<int base, int a0, int a1, int a2, int a3, int a4, int a5>
struct mkid32_6 { enum { value = mkid32_5<base, a0, a1, a2, a3, a4>::value*base + map_base<base, a5>::value};};

// a0~a4: '0'~'9', 'A'~'Z', 'a'~'z', '_', '.' ==>> [0,63]
template<int a0>
struct mkid32base64_1 : public mkid32_1<64, a0>{};
template<int a0, int a1>
struct mkid32base64_2 : public mkid32_2<64, a0, a1>{};
template<int a0, int a1, int a2>
struct mkid32base64_3 : public mkid32_3<64, a0, a1, a2>{};
template<int a0, int a1, int a2, int a3>
struct mkid32base64_4 : public mkid32_4<64, a0, a1, a2, a3>{};
template<int a0, int a1, int a2, int a3, int a4>
struct mkid32base64_5 : public mkid32_5<64, a0, a1, a2, a3, a4>{};

// a0~a5: '0'~'9', 'A'~'Z', 'a'~'z' ==>> [0,63], upper and lower letters are equal
template<int a0>
struct mkid32base36_1 : public mkid32_1<36, a0>{};
template<int a0, int a1>
struct mkid32base36_2 : public mkid32_2<36, a0, a1>{};
template<int a0, int a1, int a2>
struct mkid32base36_3 : public mkid32_3<36, a0, a1, a2>{};
template<int a0, int a1, int a2, int a3>
struct mkid32base36_4 : public mkid32_4<36, a0, a1, a2, a3>{};
template<int a0, int a1, int a2, int a3, int a4>
struct mkid32base36_5 : public mkid32_5<36, a0, a1, a2, a3, a4>{};
template<int a0, int a1, int a2, int a3, int a4, int a5>
struct mkid32base36_6 : public mkid32_6<36, a0, a1, a2, a3, a4, a5>{};

#endif // MKID_H
