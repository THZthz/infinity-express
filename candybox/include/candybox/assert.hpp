//
// Created by Amias on 2024/1/1.
//

#ifndef __CANDYBOX_ASSERT_HPP__
#define __CANDYBOX_ASSERT_HPP__

#ifndef NDEBUG
#  include <cassert>
#  define candybox_assert assert
#else
#  define candybox_assert
#endif // NDEBUG

#endif // __CANDYBOX_ASSERT_HPP__
