#pragma once

#include <Arduino.h>
#include "etl_array.h"
#include "etl_vector.h"
#include "etl_utility.h"

namespace etl {

// for_each
template<typename Iterator, typename Function>
Function for_each(Iterator first, Iterator last, Function fn) {
    for (; first != last; ++first) {
        fn(*first);
    }
    return fn;
}

template<typename Container, typename Function>
Function for_each(Container& c, Function fn) {
    return for_each(etl::begin(c), etl::end(c), fn);
}

// find_if
template<typename Iterator, typename Predicate>
Iterator find_if(Iterator first, Iterator last, Predicate pred) {
    for (; first != last; ++first) {
        if (pred(*first)) {
            return first;
        }
    }
    return last;
}

template<typename Container, typename Predicate>
auto find_if(Container& c, Predicate pred) -> decltype(c.begin()) {
    return find_if(etl::begin(c), etl::end(c), pred);
}

// accumulate
template<typename Iterator, typename T>
T accumulate(Iterator first, Iterator last, T init) {
    for (; first != last; ++first) {
        init = init + *first;
    }
    return init;
}

template<typename Iterator, typename T, typename BinaryOperation>
T accumulate(Iterator first, Iterator last, T init, BinaryOperation op) {
    for (; first != last; ++first) {
        init = op(init, *first);
    }
    return init;
}

template<typename Container, typename T>
T accumulate(Container& c, T init) {
    return accumulate(etl::begin(c), etl::end(c), init);
}

template<typename Container, typename T, typename BinaryOperation>
T accumulate(Container& c, T init, BinaryOperation op) {
    return accumulate(etl::begin(c), etl::end(c), init, op);
}

// copy
template<typename InputIterator, typename OutputIterator>
OutputIterator copy(InputIterator first, InputIterator last, OutputIterator result) {
    for (; first != last; ++first, ++result) {
        *result = *first;
    }
    return result;
}

// fill
template<typename Iterator, typename T>
void fill(Iterator first, Iterator last, const T& value) {
    for (; first != last; ++first) {
        *first = value;
    }
}

template<typename Container, typename T>
void fill(Container& c, const T& value) {
    fill(etl::begin(c), etl::end(c), value);
}

// ==================== ОПЕРАТОРЫ СРАВНЕНИЯ ====================
// template<typename T, size_t N>
// bool operator==(const array<T, N>& lhs, const array<T, N>& rhs) {
//     return etl::equal(lhs.begin(), lhs.end(), rhs.begin());
// }

// template<typename T, size_t N>
// bool operator!=(const array<T, N>& lhs, const array<T, N>& rhs) {
//     return !(lhs == rhs);
// }

// equal алгоритм
template<typename InputIterator1, typename InputIterator2>
bool equal(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2) {
    for (; first1 != last1; ++first1, ++first2) {
        if (*first1 != *first2) return false;
    }
    return true;
}

} // namespace etl