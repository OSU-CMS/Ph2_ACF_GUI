/*!
  \file                  GenericDataArray.h
  \brief                 Generic data array for DAQ
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef GenericDataArray_H
#define GenericDataArray_H

#include "ConsoleColor.h"

#include <array>
#include <iostream>

template <typename T, size_t N, size_t... S>
class GenericDataArray : public GenericDataArray<GenericDataArray<T, S...>, N>
{
};

template <typename T, size_t N>
class GenericDataArray<T, N> : public std::array<T, N>
{
  public:
    GenericDataArray()
    {
        for(size_t i = 0; i < N; ++i) this->at(i) = T();
    }
    ~GenericDataArray() {}

    T&       operator[](T pos)       = delete;
    const T& operator[](T pos) const = delete;

    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& theArchive, const unsigned int version)
    {
        for(size_t i = 0; i < N; ++i) theArchive& this->at(i);
    }
};

#endif
