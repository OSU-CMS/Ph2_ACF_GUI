/*!
  \file                  BitView.h
  \brief                 BitView implementation
  \author                Alkiviadis PAPADOPOULOS
  \version               1.0
  \date                  28/06/22
  Support:               email to mauro.dinardo@cern.ch
  Support:               email to alkiviadis.papadopoulos@cern.ch
*/

#ifndef BITMASTER_BIT_VIEW_H
#define BITMASTER_BIT_VIEW_H

#include <array>
#include <bitset>
#include <iostream>
#include <vector>

namespace BitViewDetails
{
template <class T>
T high_mask(int n)
{
    if(n <= 0 || n > int(sizeof(T) * 8)) return 0;
    return (T)~T{0} << uint8_t(sizeof(T) * 8 - n);
}

template <class T>
T low_mask(int n)
{
    if(n <= 0 || n > int(sizeof(T) * 8)) return 0;
    return (T)~T{0} >> uint8_t(sizeof(T) * 8 - n);
}

template <class T>
T mid_mask(int start, int end)
{
    return ~(high_mask<T>(start) | low_mask<T>(sizeof(T) * 8 - end));
}

} // namespace BitViewDetails

template <class BlockType>
class BitView
{
    static constexpr size_t block_size = 8 * sizeof(BlockType);

  public:
    BitView() : _data(nullptr), _start(0), _end(0) {}

    BitView(BlockType* data, size_t start, size_t end) : _data(data), _start(start), _end(end) {}
    BitView(std::vector<BlockType>& vec) : _data(vec.data()), _start(0), _end(block_size * vec.size()) {}

    explicit BitView(BlockType& val) : _data(&val), _start(0), _end(block_size) {}

    size_t start() const { return _start; }
    size_t size() const { return _end - _start; }

    auto data() const { return _data; }

    BitView slice(size_t start) const
    {
        if(start > size()) throw std::runtime_error("BitView::slice: out of range");
        return {_data, _start + start, _end};
    }

    BitView slice(size_t start, size_t end) const
    {
        if(start > end || end > size()) { throw std::runtime_error("BitView::slice: out of range"); }
        return {_data, _start + start, _start + end};
    }

    void skip(size_t n)
    {
        if(n > size()) throw std::runtime_error("BitView::skip: out of range");
        _start += n;
    }

    template <class T = size_t>
    T parse(size_t n) const
    {
        return parse(0, n);
    }

    template <class T = size_t>
    T parse(size_t offset, size_t n) const
    {
        if(offset + n > size()) throw std::runtime_error("BitView::parse: out of range");

        T result{0};

        int    block_offset = (_start + offset) / block_size;
        size_t bit_offset   = (_start + offset) % block_size;
        size_t bits_copied  = 0;

        while(true)
        {
            size_t bits_in_current_block = block_size - bit_offset;
            size_t bits_to_copy          = n - bits_copied;
            if(bits_in_current_block >= bits_to_copy)
            {
                size_t excess_bits_in_current_block = bits_in_current_block - bits_to_copy;
                result |= (_data[block_offset] >> excess_bits_in_current_block) & BitViewDetails::low_mask<BlockType>(bits_to_copy);
                break;
            }
            else
            {
                result |= (_data[block_offset] & BitViewDetails::low_mask<BlockType>(bits_in_current_block)) << (bits_to_copy - bits_in_current_block);
                ++block_offset;
                bit_offset = 0;
                bits_copied += bits_in_current_block;
            }
        }

        return result;
    }

    template <class T = size_t>
    T pop(size_t n)
    {
        T result = parse(n);
        _start += n;
        return result;
    }

    BitView pop_slice(size_t n)
    {
        if(n > size()) throw std::runtime_error("BitView::pop_slice: out of range");
        size_t start = _start;
        _start += n;
        return {_data, start, start + n};
    }

    template <class T>
    union U
    {
        T                                   val;
        std::array<std::uint8_t, sizeof(T)> raw;
    };

  private:
    template <typename T>
    struct type_tag
    {
    };

    template <class ByteIterator>
    void copy_into(ByteIterator begin, ByteIterator end) const
    {
        int    block_offset = _start / block_size;
        size_t bit_offset   = _start % block_size;

        while(begin != end)
        {
            if(bit_offset + 8 > block_size)
            {
                int left  = block_size - bit_offset;
                int right = 8 - left;
                *begin    = (_data[block_offset] & BitViewDetails::low_mask<BlockType>(left)) << right;
                *begin |= (_data[block_offset + 1] & BitViewDetails::high_mask<BlockType>(right)) >> (block_size - right);
                bit_offset += 8 - block_size;
                ++block_offset;
            }
            else
            {
                *begin = (_data[block_offset] & BitViewDetails::mid_mask<BlockType>(bit_offset, bit_offset + 8)) >> (block_size - 8 - bit_offset);
                bit_offset += 8;
            }
            ++begin;
        }
    }

    template <class T>
    T get(type_tag<T>, bool big_endian) const
    {
        U<T> result;
        if(big_endian)
            copy_into(std::rbegin(result.raw), std::rend(result.raw));
        else
            copy_into(std::begin(result.raw), std::end(result.raw));
        if(size() < 8 * sizeof(T))
        {
            if(big_endian)
                result.val >>= 8 * sizeof(T) - size();
            else
                result.val &= BitViewDetails::low_mask<T>(size());
        }
        return result.val;
    }

    // Specialization for bool
    bool get(type_tag<bool>, bool) const
    {
        int    block_offset = _start / block_size;
        size_t bit_offset   = _start % block_size;
        return _data[block_offset] & BitViewDetails::mid_mask<BlockType>(bit_offset, bit_offset + 1);
    }

  public:
    template <class T>
    T get(bool big_endian = true) const
    {
        return get(type_tag<T>{}, big_endian);
    }

    template <class T>
    void set(const T& value, bool big_endian = true)
    {
        if(size() == 0) return;

        int first_block = _start / block_size;
        int last_block  = (_end - 1) / block_size;

        size_t bit_start = _start % block_size;
        size_t bit_end   = (_end - 1) % block_size + 1;

        int n_bits;
        if(first_block == last_block) { n_bits = bit_end - bit_start; }
        else { n_bits = block_size - bit_start; }
        _data[first_block] &= ~BitViewDetails::mid_mask<BlockType>(bit_start, bit_start + n_bits);
        BlockType new_value = (value >> (size() - n_bits)) & BitViewDetails::low_mask<T>(n_bits);
        if(bit_start + n_bits < block_size) { new_value <<= block_size - (bit_start + n_bits); }
        _data[first_block] |= new_value;

        for(int i = first_block + 1; i <= last_block; ++i)
        {
            if(i < last_block) { _data[i] = (value >> (size() - i * block_size - n_bits)) & BitViewDetails::low_mask<T>(block_size); }
            else
            {
                _data[i] &= BitViewDetails::low_mask<BlockType>(block_size - bit_end);
                _data[i] |= (BlockType)(value & BitViewDetails::low_mask<T>(bit_end)) << (block_size - bit_end);
            }
        }
    }

    friend std::ostream& operator<<(std::ostream& os, const BitView& bit_view)
    {
        for(size_t i = 0; i < bit_view.size(); ++i) os << bit_view.slice(i, i + 1).template get<bool>();
        return os;
    };

  private:
    BlockType* _data;
    size_t     _start;
    size_t     _end;
};

template <class T>
BitView<T> bit_view(T* data, size_t start, size_t end)
{
    return {data, start, end};
}

template <class T>
BitView<T> bit_view(std::vector<T>& vec)
{
    return {vec};
}

template <class T>
BitView<const T> bit_view(const T* data, size_t start, size_t end)
{
    return {data, start, end};
}

template <class T>
BitView<const T> bit_view(const std::vector<T>& vec)
{
    return {vec.data(), 0, vec.size() * 8 * sizeof(T)};
}

#endif
