// Bit Packing Utilities
//
// Author: Alkis
//
// Features:
//          - pack (concatenate) integers into a value
//          - unpack (extract) integers from a value
//          - all functions are constexpr
//
// where integers is one of the following:
//          - a number of variables of any integer type
//          - a built-in array of any integer type
//          - a std::array of any integer type
//          - a pair of iterators that define a range of integers
//
//
// Some hacks used in this code:
//  - C++17 fold expressions in C++14:
//      Ex:
//        C++17:    ((array[i++] = pack), ...);
//
//        C++14:    [](auto...){}(array[i++] = pack ...); // wrong: the order of evaluation of function arguments is
//        undefined
//                  auto _ = { array[i++] = pack ... }; // ok: initializer lists behave as expected
//
//  - In some templates I want the first template argument to be optional with a default value that depends on other
//  template arguments:
//    In this case I use:
//      template <
//          class T = void,
//          /*more parameters*/,
//          class R = typename std::conditional_t<
//              std::is_void<T>::value,
//              /*something that depends on parameters*/,
//              T
//          >
//      >
//
// Support: email to alkiviadis.papadopoulos@cern.ch
//          email to mauro.dinardo@cern.ch

#ifndef BIT_PACKING_H
#define BIT_PACKING_H

#include <cstdint>
#include <iostream>
#include <tuple>

namespace bits
{
using std::size_t;

namespace local_detail
{
// if c++14 is not fully supported...
#if __cplusplus < 999999 // 201402

#define CONSTEXPR_

template <size_t... Is>
struct index_sequence
{
};

template <size_t N, size_t... Is>
struct make_index_sequence_impl
{
    using type = typename make_index_sequence_impl<N - 1, N - 1, Is...>::type;
};

template <size_t N, size_t... Is>
struct make_index_sequence_impl<N, 0, Is...>
{
    using type = index_sequence<0, Is...>;
};

template <size_t N>
using make_index_sequence = typename make_index_sequence_impl<N>::type;

template <bool Cond, class T>
using enable_if_t = typename std::enable_if<Cond, T>::type;

template <bool B, class T, class F>
using conditional_t = typename std::conditional<B, T, F>::type;

template <class Iter>
constexpr std::reverse_iterator<Iter> make_reverse_iterator(Iter i)
{
    return std::reverse_iterator<Iter>(i);
}

#else

#define CONSTEXPR_ constexpr

using std::conditional_t;
using std::enable_if_t;
using std::index_sequence;
using std::make_index_sequence;
using std::make_reverse_iterator;

#endif

template <size_t size, size_t... sizes>
struct size_sum
{
    static constexpr size_t value = size + size_sum<sizes...>::value;
};

template <size_t size>
struct size_sum<size>
{
    static constexpr size_t value = size;
};

template <size_t... Is>
auto make_reverse_impl(index_sequence<Is...>) -> index_sequence<sizeof...(Is) - 1 - Is...>
{
}

// used to declare a reveresed index_sequence
template <size_t N>
using make_reverse_sequence = decltype(make_reverse_impl(make_index_sequence<N>{}));

// choose uint that has N or more bits
template <size_t N, enable_if_t<(N <= 8), int> = 0>
auto uint_helper() -> uint8_t;

template <size_t N, enable_if_t<(N > 8 && N <= 16), int> = 0>
auto uint_helper() -> uint16_t;

template <size_t N, enable_if_t<(N > 16 && N <= 32), int> = 0>
auto uint_helper() -> uint32_t;

template <size_t N, enable_if_t<(N > 32), int> = 0>
auto uint_helper() -> uint64_t;

template <size_t N>
struct uint_of_size
{
    static_assert(N <= 64, "Cannot declare uint larger than 64 bits.");
    using type = decltype(uint_helper<N>());
};

template <size_t... Sizes>
using uint_t = typename uint_of_size<size_sum<Sizes...>::value>::type;

// tuple of references with constexpr assignement operator
// because std::tuple::operator= is not constexpr
// useful in case anyone wants to use Packer::unpack in constexpr
template <class... Ts>
struct ref_tuple
{
    constexpr ref_tuple(Ts&... args) : tup(args...) {}

    template <class Tuple>
    constexpr void operator=(const Tuple& source)
    {
        assign(source, make_index_sequence<sizeof...(Ts)>{});
    }

  private:
    template <class Tuple, size_t... Is>
    constexpr void assign(const Tuple& source, index_sequence<Is...>)
    {
        __attribute__((unused)) auto unused = {(std::get<Is>(tup) = std::get<Is>(source), 0)...};
    }

    std::tuple<Ts&...> tup;
};

} // namespace local_detail

// use this instead of std::tie in case you want to unpack in a constexpr context
template <class... Args>
constexpr auto make_ref(Args&... args)
{
    return local_detail::ref_tuple<Args...>(args...);
}

template <class T>
constexpr unsigned bit_count()
{
    return 8 * sizeof(T);
}

// returns a value with only the n least significant bits set.
constexpr size_t bit_mask(size_t n) { return n ? ~size_t{0} >> (bit_count<size_t>() - n) : 0; }

// Packer
// For a pack Sizes of length N:
// - pack:      returns the value that results from concatenating the bitstrings that occupy
//              the Sizes(i) least significant bits of each argument i.
// - unpack:    returns a std::tuple with the values of the N contiguous bitstrings of size
//              Sizes(i) that occupy the least sigificant bits of the given value.
template <size_t... Sizes>
struct Packer
{
    static_assert(sizeof...(Sizes) > 0, "Packer requires at least one template argument.");

    static constexpr size_t total_size = local_detail::size_sum<Sizes...>::value;

    template <class T = local_detail::uint_t<total_size>, class... Args>
    static CONSTEXPR_ T pack(Args... args)
    {
        static_assert(sizeof...(Sizes) == sizeof...(Args), "Invalid number of arguments to pack.");
        static_assert(sizeof(T) * 8 >= total_size, "T is too small.");
        T                            result = 0;
        size_t                       size   = total_size;
        __attribute__((unused)) auto unused = {result |= (args & bit_mask(Sizes)) << (size -= Sizes)...};
        return result;
    }

    template <class T>
    static CONSTEXPR_ auto unpack(T value)
    {
        return unpack_impl(value, local_detail::make_index_sequence<sizeof...(Sizes)>{});
    }

  private:
    template <class T, size_t...>
    using Identity = T;

    template <size_t... Is, class T>
    static CONSTEXPR_ auto unpack_impl(T value, local_detail::index_sequence<Is...>)
    {
        std::tuple<Identity<T, Is>...> result;
        size_t                         size   = total_size;
        __attribute__((unused)) auto   unused = {std::get<Is>(result) = (value >> (size -= Sizes)) & bit_mask(Sizes)...};
        return result;
    }
};

template <size_t... Sizes, class... Args>
CONSTEXPR_ auto pack(Args... args)
{
    return Packer<Sizes...>::pack(args...);
}

template <size_t... Sizes, class T>
CONSTEXPR_ auto unpack(T value)
{
    return Packer<Sizes...>::unpack(value);
}

// RangePacker
// For an array/range of size N:
// - pack:      returns the value that results from concatenating the bitstrings that
//              occupy the Size least significant bits of each element of the array/range.
// - unpack:    stores the values of the N contiguous bitstrings of size Size that occupy
//              the least significant bits of the given value into the corresponding
//              elements of the array/range.
// (pack/unpack) x (forward/reverse) x (range/std::array/array) = 12 functions
template <size_t Size>
struct RangePacker
{
    // unpack value into range
    template <class T, class It>
    static CONSTEXPR_ void unpack(T value, It begin, It end)
    {
        size_t total_size = std::distance(begin, end) * Size;
        for(auto it = begin; it != end; it++) { *it = (value >> (total_size -= Size)) & bit_mask(Size); }
    }

    // unpack value into reversed range
    template <class T, class It>
    static CONSTEXPR_ void unpack_reverse(T value, It begin, It end)
    {
        unpack(value, local_detail::make_reverse_iterator(end), local_detail::make_reverse_iterator(begin));
    }

    // unpack value into std::array
    template <size_t N, class T, class U>
    static CONSTEXPR_ void unpack(T value, std::array<U, N>& array)
    {
        unpack_impl(value, array, local_detail::make_index_sequence<N>{});
    }

    // unpack value into reversed std::array
    template <size_t N, class T, class U>
    static CONSTEXPR_ void unpack_reverse(T value, std::array<U, N>& array)
    {
        unpack_impl(value, array, local_detail::make_reverse_sequence<N>{});
    }

    // unpack value into bilt-in array
    template <size_t N, class T, class U>
    static CONSTEXPR_ void unpack(T value, U (&array)[N])
    {
        unpack_impl(value, array, local_detail::make_index_sequence<N>{});
    }

    // unpack value into reversed bilt-in array
    template <size_t N, class T, class U>
    static CONSTEXPR_ void unpack_reverse(T value, U (&array)[N])
    {
        unpack_impl(value, array, local_detail::make_reverse_sequence<N>{});
    }

    // pack range
    template <class T = uint64_t, class It>
    static CONSTEXPR_ T pack(const It& begin, const It& end)
    {
        T      result     = 0;
        size_t total_size = std::distance(begin, end) * Size;
        for(auto it = begin; it != end; it++) { result |= (*it & bit_mask(Size)) << (total_size -= Size); }
        return result;
    }

    // pack range reversed
    template <class T = uint64_t, class It>
    static CONSTEXPR_ T pack_reverse(const It& begin, const It& end)
    {
        return pack(local_detail::make_reverse_iterator(end), local_detail::make_reverse_iterator(begin));
    }

    // pack std::array
    template <class T = void, size_t N, class U, class R = typename local_detail::conditional_t<std::is_void<T>::value, local_detail::uint_t<N * Size>, T>>
    static CONSTEXPR_ R pack(const std::array<U, N>& array)
    {
        return pack_impl<R>(array, local_detail::make_index_sequence<N>{});
    }

    // pack std::array reversed
    template <class T = void, size_t N, class U, class R = typename local_detail::conditional_t<std::is_void<T>::value, local_detail::uint_t<N * Size>, T>>
    static CONSTEXPR_ R pack_reverse(const std::array<U, N>& array)
    {
        return pack_impl<R>(array, local_detail::make_reverse_sequence<N>{});
    }

    // pack bilt-in array
    template <class T = void, size_t N, class U, class R = typename local_detail::conditional_t<std::is_void<T>::value, local_detail::uint_t<N * Size>, T>>
    static CONSTEXPR_ R pack(const U (&array)[N])
    {
        return pack_impl<R>(array, local_detail::make_index_sequence<N>{});
    }

    // pack bilt-in array reversed
    template <class T = void, size_t N, class U, class R = typename local_detail::conditional_t<std::is_void<T>::value, local_detail::uint_t<N * Size>, T>>
    static CONSTEXPR_ R pack_reverse(const U (&array)[N])
    {
        return pack_impl<R>(array, local_detail::make_reverse_sequence<N>{});
    }

  private:
    template <size_t N, class T, class U, size_t... Is>
    static CONSTEXPR_ void unpack_impl(T value, std::array<U, N>& array, local_detail::index_sequence<Is...>)
    {
        size_t                       total_size = N * Size;
        __attribute__((unused)) auto unused     = {array[Is] = (value >> (total_size -= Size)) & bit_mask(Size)...};
    }

    template <size_t N, class T, class U, size_t... Is>
    static CONSTEXPR_ void unpack_impl(T value, U (&array)[N], local_detail::index_sequence<Is...>)
    {
        size_t                       total_size = N * Size;
        __attribute__((unused)) auto unused     = {array[Is] = (value >> (total_size -= Size)) & bit_mask(Size)...};
    }

    template <class T, size_t N, class U, size_t... Is>
    static CONSTEXPR_ T pack_impl(const std::array<U, N>& array, local_detail::index_sequence<Is...>)
    {
        return Packer<(Is - Is + Size)...>::template pack<T>(array[Is]...);
    }

    template <class T, size_t N, class U, size_t... Is>
    static CONSTEXPR_ T pack_impl(const U (&array)[N], local_detail::index_sequence<Is...>)
    {
        return Packer<(Is - Is + Size)...>::template pack<T>(array[Is]...);
    }
};

// unpack range into range
template <size_t Size, class InIt, class OutIt>
static CONSTEXPR_ void unpack_range(InIt in_first, InIt in_last, OutIt out_first)
{
    // using input_type = typename std::iterator_traits<OutIt>::value_type;
    using output_type           = typename std::iterator_traits<OutIt>::value_type;
    constexpr size_t input_size = bit_count<typename std::iterator_traits<InIt>::value_type>();
    static_assert(input_size > Size, "The size of the input range's value type is too small.");
    int         excess = input_size - Size;
    output_type value  = 0;
    while(in_first != in_last)
    {
        if(excess > 0)
        {
            *out_first = value | (bit_mask(Size) & (*in_first >> excess));
            value      = 0;
            ++out_first;
            excess -= Size;
        }
        else
        {
            value = bit_mask(Size) & (*in_first << (-excess));
            ++in_first;
            excess += input_size;
        }
    }
    *out_first = value;
}

} // namespace bits

#undef CONSTEXPR_

#endif
