/******************************************************************************
    ImageConverterTypes: type id and manually id register function
    Copyright (C) 2013 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV

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

#ifndef QTAV_IMAGECONVERTERTYPES_H
#define QTAV_IMAGECONVERTERTYPES_H

/*
 * This file can not be included by ImageConverterXXX.{h,cpp}
 * Usually you just include this and then you can use factory
 * e.g.
 *      ImageConverter* c = ImageConverter::create(ImageConverterId_FF);
 */

#include "ImageConverter.h"

namespace QtAV {

/*
 * When a new type is created, declare the type here, define the value in cpp
 */
//why can not be const for msvc?
extern Q_EXPORT ImageConverterId ImageConverterId_FF;
extern Q_EXPORT ImageConverterId ImageConverterId_IPP;

/*
 * This must be called manually in your program(outside this library) if your compiler does
 * not support automatically call a function before main(). And you should add functions there
 * to register a new type.
 * See prepost.h. Currently supports c++ compilers, and C compilers such as gnu compiler
 * (including llvm based compilers) and msvc
 * Another case is link this library as static library. Not all data will be pulled to you program
 * especially the automatically called functions.
 * Steps:
 *   1. add RegisterImageConverterXXX_Man() in ImageConverter_RegisterAll() in ImageConverter.cpp;
 *   2. define the RegisterImageConverterXXX_Man() In ImageConverterXXX.cpp
 */
Q_EXPORT void ImageConverter_RegisterAll();

} //namespace QtAV
#endif // QTAV_IMAGECONVERTERTYPES_H
