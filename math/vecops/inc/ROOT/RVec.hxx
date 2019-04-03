// Author: Enrico Guiraud, Enric Tejedor, Danilo Piparo CERN  01/2018

/*************************************************************************
 * Copyright (C) 1995-2018, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

/**
  \defgroup vecops VecOps
*/

#ifndef ROOT_TVEC
#define ROOT_TVEC

#ifdef _WIN32
#define _VECOPS_USE_EXTERN_TEMPLATES false
#else
#define _VECOPS_USE_EXTERN_TEMPLATES true
#endif

#include <ROOT/RAdoptAllocator.hxx>
#include <ROOT/RIntegerSequence.hxx>
#include <ROOT/TypeTraits.hxx>

#include <algorithm>
#include <cmath>
#include <numeric> // for inner_product
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include <utility>

#ifdef R__HAS_VDT
#include <vdt/vdtMath.h>
#endif

// TODO: Figure out best way to get pi on windows
// TODO: ROOT::Internal::VecOps::pi is set to float because of narrowing errors on windows.
#if defined(_MSC_VER)
#include <math.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846264338328
#endif

namespace ROOT {
namespace VecOps {
template <typename T>
class RVec;
}

namespace Detail {
namespace VecOps {

template<typename T>
using RVec = ROOT::VecOps::RVec<T>;

template <typename F, typename... T>
auto MapImpl(F &&f, const RVec<T> &... vs) -> RVec<decltype(f(vs[0]...))>
{
   constexpr const auto nArgs = sizeof...(T);
   const std::size_t sizes[] = {vs.size()...};
   if (nArgs > 1) {
      for (auto i = 1UL; i < nArgs; i++) {
         if (sizes[0] == sizes[i])
            continue;
         throw std::runtime_error("Map: input RVec instances have different lengths!");
      }
   }
   RVec<decltype(f(vs[0]...))> ret(sizes[0]);

   for (auto i = 0UL; i < sizes[0]; i++)
      ret[i] = f(vs[i]...);

   return ret;
}

template <typename Tuple_t, std::size_t... Is>
auto MapFromTuple(Tuple_t &&t, std::index_sequence<Is...>)
   -> decltype(MapImpl(std::get<std::tuple_size<Tuple_t>::value - 1>(t), std::get<Is>(t)...))
{
   constexpr const auto tupleSizeM1 = std::tuple_size<Tuple_t>::value - 1;
   return MapImpl(std::get<tupleSizeM1>(t), std::get<Is>(t)...);
}

}
}

namespace Internal {
namespace VecOps {

constexpr float pi = M_PI;

// We use this helper to workaround a limitation of compilers such as
// gcc 4.8 amd clang on osx 10.14 for which std::vector<bool>::emplace_back
// is not defined.
template <typename T, typename... Args>
void EmplaceBack(T &v, Args &&... args)
{
   v.emplace_back(std::forward<Args>(args)...);
}

template <typename... Args>
void EmplaceBack(std::vector<bool> &v, Args &&... args)
{
   v.push_back(std::forward<Args>(args)...);
}

// This template and specialisation allow to create vectors of size 0 and with T
// w/o a default constructor
template <typename RVec_t, std::size_t BufferSize = RVec_t::fgBufferSize>
class RStorageVectorFactory {
public:
   static typename RVec_t::Impl_t Get(typename RVec_t::pointer buf)
   {
      typename RVec_t::Impl_t v(BufferSize, typename RVec_t::value_type(),
                                ::ROOT::Detail::VecOps::MakeAdoptAllocator(buf, BufferSize));
      v.resize(0);
      return v;
   }
};

template <typename RVec_t>
class RStorageVectorFactory<RVec_t, 0> {
public:
   static typename RVec_t::Impl_t Get(typename RVec_t::value_type *) { return typename RVec_t::Impl_t(); }
};

} // namespace VecOps
} // namespace Internal

namespace VecOps {
// clang-format off
/**
\class ROOT::VecOps::RVec
\ingroup vecops
\brief A "std::vector"-like collection of values implementing handy operation to analyse them
\tparam T The type of the contained objects

A RVec is a container designed to make analysis of values' collections fast and easy.
Its storage is contiguous in memory and its interface is designed such to resemble to the one
of the stl vector. In addition the interface features methods and external functions to ease
the manipulation and analysis of the data in the RVec.
RVec implements for arithmetic types a small buffer optimisation, relying on the stack rather
than the heap to represent small collections.

\htmlonly
<a href="https://doi.org/10.5281/zenodo.1253756"><img src="https://zenodo.org/badge/DOI/10.5281/zenodo.1253756.svg" alt="DOI"></a>
\endhtmlonly

## Table of Contents
- [Example](#example)
- [Owning and adopting memory](#owningandadoptingmemory)
- [Sorting and manipulation of indices](#sorting)
- [Usage in combination with RDataFrame](#usagetdataframe)
- [Reference for the RVec class](#RVecdoxyref)

Also see the [reference for RVec helper functions](https://root.cern/doc/master/namespaceROOT_1_1VecOps.html).

## <a name="example"></a>Example
Suppose to have an event featuring a collection of muons with a certain pseudorapidity,
momentum and charge, e.g.:
~~~{.cpp}
std::vector<short> mu_charge {1, 1, -1, -1, -1, 1, 1, -1};
std::vector<float> mu_pt {56, 45, 32, 24, 12, 8, 7, 6.2};
std::vector<float> mu_eta {3.1, -.2, -1.1, 1, 4.1, 1.6, 2.4, -.5};
~~~
Suppose you want to extract the transverse momenta of the muons satisfying certain
criteria, for example consider only negatively charged muons with a pseudorapidity
smaller or equal to 2 and with a transverse momentum greater than 10 GeV.
Such a selection would require, among the other things, the management of an explicit
loop, for example:
~~~{.cpp}
std::vector<float> goodMuons_pt;
const auto size = mu_charge.size();
for (size_t i=0; i < size; ++i) {
   if (mu_pt[i] > 10 && abs(mu_eta[i]) <= 2. &&  mu_charge[i] == -1) {
      goodMuons_pt.emplace_back(mu_pt[i]);
   }
}
~~~
These operations become straightforward with RVec - we just need to *write what
we mean*:
~~~{.cpp}
auto goodMuons_pt = mu_pt[ (mu_pt > 10.f && abs(mu_eta) <= 2.f && mu_charge == -1) ]
~~~
Now the clean collection of transverse momenta can be used within the rest of the data analysis, for
example to fill a histogram.

## <a name="owningandadoptingmemory"></a>Owning and adopting memory
RVec has contiguous memory associated to it. It can own it or simply adopt it. In the latter case,
it can be constructed with the address of the memory associated to it and its length. For example:
~~~{.cpp}
std::vector<int> myStlVec {1,2,3};
RVec<int> myRVec(myStlVec.data(), myStlVec.size());
~~~
In this case, the memory associated to myStlVec and myRVec is the same, myRVec simply "adopted it".
If any method which implies a re-allocation is called, e.g. *emplace_back* or *resize*, the adopted
memory is released and new one is allocated. The previous content is copied in the new memory and
preserved.

## <a name="#sorting"></a>Sorting and manipulation of indices

### Sorting
RVec complies to the STL interfaces when it comes to iterations. As a result, standard algorithms
can be used, for example sorting:
~~~{.cpp}
RVec<double> v{6., 4., 5.};
std::sort(v.begin(), v.end());
~~~

For convinience, helpers are provided too:
~~~{.cpp}
auto sorted_v = Sort(v);
auto reversed_v = Reverse(v);
~~~

### Manipulation of indices

It is also possible to manipulated the RVecs acting on their indices. For example,
the following syntax
~~~{.cpp}
RVec<double> v0 {9., 7., 8.};
auto v1 = Take(v0, {1, 2, 0});
~~~
will yield a new RVec<double> the content of which is the first, second and zeroth element of
v0, i.e. `{7., 8., 9.}`.

The `Argsort` helper extracts the indices which order the content of a `RVec`. For example,
this snippet accomplish in a more expressive way what we just achieved:
~~~{.cpp}
auto v1_indices = Argsort(v0); // The content of v1_indices is {1, 2, 0}.
v1 = Take(v0, v1_indices);
~~~

The `Take` utility allows to extract portions of the `RVec`. The content to be *taken*
can be specified with an `RVec` of indices or an integer. If the integer is negative,
elements will be picked starting from the end of the container:
~~~{.cpp}
RVec<float> vf {1.f, 2.f, 3.f, 4.f};
auto vf_1 = Take(vf, {1, 3}); // The content is {2.f, 4.f}
auto vf_2 = Take(vf, 2); // The content is {1.f, 2.f}
auto vf_3 = Take(vf, -3); // The content is {2.f, 3.f, 4.f}
~~~

## <a name="usagetdataframe"></a>Usage in combination with RDataFrame
RDataFrame leverages internally RVecs. Suppose to have a dataset stored in a
TTree which holds these columns (here we choose C arrays to represent the
collections, they could be as well std::vector instances):
~~~{.bash}
  nPart            "nPart/I"            An integer representing the number of particles
  px               "px[nPart]/D"        The C array of the particles' x component of the momentum
  py               "py[nPart]/D"        The C array of the particles' y component of the momentum
  E                "E[nPart]/D"         The C array of the particles' Energy
~~~
Suppose you'd like to plot in a histogram the transverse momenta of all particles
for which the energy is greater than 200 MeV.
The code required would just be:
~~~{.cpp}
RDataFrame d("mytree", "myfile.root");
using doubles = RVec<double>;
auto cutPt = [](doubles &pxs, doubles &pys, doubles &Es) {
   auto all_pts = sqrt(pxs * pxs + pys * pys);
   auto good_pts = all_pts[Es > 200.];
   return good_pts;
   };

auto hpt = d.Define("pt", cutPt, {"px", "py", "E"})
            .Histo1D("pt");
hpt->Draw();
~~~
And if you'd like to express your selection as a string:
~~~{.cpp}
RDataFrame d("mytree", "myfile.root");
auto hpt = d.Define("pt", "sqrt(pxs * pxs + pys * pys)[E>200]")
            .Histo1D("pt");
hpt->Draw();
~~~
<a name="RVecdoxyref"></a>
**/
// clang-format on

template <typename T>
class RVec {
   // Here we check if T is a bool. This is done in order to decide what type
   // to use as a storage. If T is anything but bool, we use a vector<T, RAdoptAllocator<T>>.
   // If T is a bool, we opt for a plain vector<bool> otherwise we'll not be able
   // to write the data type given the shortcomings of TCollectionProxy design.
   static constexpr const auto fgIsVecBool = std::is_same<bool, T>::value;
   using Alloc_t =
      typename std::conditional<fgIsVecBool, std::allocator<T>, ::ROOT::Detail::VecOps::RAdoptAllocator<T>>::type;

public:
   using Impl_t = std::vector<T, Alloc_t>;
   using value_type = typename Impl_t::value_type;
   using size_type = typename Impl_t::size_type;
   using difference_type = typename Impl_t::difference_type;
   using reference = typename Impl_t::reference;
   using const_reference = typename Impl_t::const_reference;
   using pointer = typename Impl_t::pointer;
   using const_pointer = typename Impl_t::const_pointer;
   // The data_t and const_data_t types are chosen to be void in case T is a bool.
   // This way we can as elegantly as in the STL return void upon calling the data() method.
   using data_t = typename std::conditional<fgIsVecBool, void, typename Impl_t::pointer>::type;
   using const_data_t = typename std::conditional<fgIsVecBool, void, typename Impl_t::const_pointer>::type;
   using iterator = typename Impl_t::iterator;
   using const_iterator = typename Impl_t::const_iterator;
   using reverse_iterator = typename Impl_t::reverse_iterator;
   using const_reverse_iterator = typename Impl_t::const_reverse_iterator;
   static constexpr std::size_t fgBufferSize = std::is_arithmetic<T>::value && !fgIsVecBool ? 16 : 0;

private:
   // We need this class for the case where fgBufferSize is 0, otherwise array<NonCopiable, 0>
   // will not compile on macos.
   class RArrayPlaceHolder {
   public:
      std::size_t size() { return 0U; }
      T *data() { return nullptr; }
   };

   /// The type of the buffer the RVec keeps for small buffer optimisation.
   /// If the size of the buffer is 0, a place holder internal type is used, if not an std::array
   /// is used because its content resides on the stack. The RAdoptAllocator adopts if needed this
   /// memory.
   using Buffer_t = typename std::conditional<fgBufferSize == 0, RArrayPlaceHolder, std::array<T, fgBufferSize>>::type;

   /// The buffer used for avoiding heap allocations
   Buffer_t fBuffer;

   /// The allocator for the RVec. It is created through a free function since this could
   /// be an std::allocator<bool> and therefore a template for all T!=Bool and a no-op
   /// overload for bool are needed.
   Alloc_t fAlloc = ::ROOT::Detail::VecOps::MakeAdoptAllocator(fgBufferSize ? fBuffer.data() : nullptr, fgBufferSize);

   /// The default storage std::vector, initialised with the allocator
   Impl_t fData{fAlloc};

   bool CanUseBuffer(const RVec &v)
   {
      const auto thisBufSize = ::ROOT::Detail::VecOps::GetBufferSize(fAlloc);
      return thisBufSize && v.size() <= thisBufSize;
   }

public:
   // constructors

   // We need to use a sfinae here to allow for the case where a vector of size 0 is created
   // and T does not have a default constructor. We cannot even write T().
   RVec() : fData(::ROOT::Internal::VecOps::RStorageVectorFactory<RVec>::Get(fBuffer.data())) {}

   RVec(pointer p, size_type n) : fAlloc(::ROOT::Detail::VecOps::MakeAdoptAllocator(p)), fData(n, T(), fAlloc) {}

   // Here we initialise the internal vector to the size of the small buffer if count is smaller or equal
   // than such size to then resize back to count.
   // This step is carried out in order for the capacity of fData to be equal to the size of the
   // buffer.
   explicit RVec(size_type count) : fData(count, T(), fAlloc) {}

   RVec(size_type count, const T &value) : fData(count, value, fAlloc) {}

   RVec(const RVec &v) : fData(v.size() <= fgBufferSize ? fgBufferSize : 0, T(), fAlloc)
   {
      resize(v.size());
      std::copy(v.begin(), v.end(), begin());
   }

   RVec(const std::vector<T> &v) : fData(v.size() <= fgBufferSize ? fgBufferSize : 0, T(), fAlloc)
   {
      resize(v.size());
      std::copy(v.begin(), v.end(), fData.begin());
   }

   RVec(RVec<T> &&v)
   {
      if (CanUseBuffer(v)) {
         fData.assign(v.fData.begin(), v.fData.end());
      } else {
         fAlloc = std::move(v.fAlloc);
         fData = std::move(v.fData);
      }
   }

   template <class InputIt>
   RVec(InputIt first, InputIt last) : fData(first, last, fAlloc)
   {
   }

   RVec(std::initializer_list<T> ilist) : fData(ilist.size(), T(), fAlloc)
   {
      std::copy(ilist.begin(), ilist.end(), fData.begin());
   }

   // assignment
   RVec<T> &operator=(const RVec<T> &v)
   {
      fData.assign(v.fData.begin(), v.fData.end());
      return *this;
   }

   RVec<T> &operator=(RVec<T> &&v)
   {
      if (CanUseBuffer(v)) {
         fData.assign(v.fData.begin(), v.fData.end());
      } else {
         fAlloc = std::move(v.fAlloc);
         fData = std::move(v.fData);
      }
      return *this;
   }

   RVec<T> &operator=(std::initializer_list<T> ilist)
   {
      resize(ilist.size());
      std::copy(ilist.begin(), ilist.end(), begin());
      return *this;
   }

   // conversion
   template <typename U, typename = std::enable_if<std::is_convertible<T, U>::value>>
   operator RVec<U>() const
   {
      RVec<U> ret(size());
      std::copy(begin(), end(), ret.begin());
      return ret;
   }

   const Impl_t &AsVector() const { return fData; }
   Impl_t &AsVector() { return fData; }

   // accessors
   reference at(size_type pos) { return fData.at(pos); }
   const_reference at(size_type pos) const { return fData.at(pos); }
   /// No exception thrown. The user specifies the desired value in case the RVec is shorter than `pos`.
   value_type at(size_type pos, value_type fallback) { return pos < fData.size() ? fData[pos] : fallback; }
   /// No exception thrown. The user specifies the desired value in case the RVec is shorter than `pos`.
   value_type at(size_type pos, value_type fallback) const { return pos < fData.size() ? fData[pos] : fallback; }
   reference operator[](size_type pos) { return fData[pos]; }
   const_reference operator[](size_type pos) const { return fData[pos]; }

   template <typename V, typename = std::enable_if<std::is_convertible<V, bool>::value>>
   RVec operator[](const RVec<V> &conds) const
   {
      const size_type n = conds.size();

      if (n != size())
         throw std::runtime_error("Cannot index RVec with condition vector of different size");

      RVec<T> ret;
      ret.reserve(n);
      for (size_type i = 0; i < n; ++i)
         if (conds[i])
            ret.emplace_back(fData[i]);
      return ret;
   }

   reference front() { return fData.front(); }
   const_reference front() const { return fData.front(); }
   reference back() { return fData.back(); }
   const_reference back() const { return fData.back(); }
   data_t data() noexcept { return fData.data(); }
   const_data_t data() const noexcept { return fData.data(); }
   // iterators
   iterator begin() noexcept { return fData.begin(); }
   const_iterator begin() const noexcept { return fData.begin(); }
   const_iterator cbegin() const noexcept { return fData.cbegin(); }
   iterator end() noexcept { return fData.end(); }
   const_iterator end() const noexcept { return fData.end(); }
   const_iterator cend() const noexcept { return fData.cend(); }
   reverse_iterator rbegin() noexcept { return fData.rbegin(); }
   const_reverse_iterator rbegin() const noexcept { return fData.rbegin(); }
   const_reverse_iterator crbegin() const noexcept { return fData.crbegin(); }
   reverse_iterator rend() noexcept { return fData.rend(); }
   const_reverse_iterator rend() const noexcept { return fData.rend(); }
   const_reverse_iterator crend() const noexcept { return fData.crend(); }
   // capacity
   bool empty() const noexcept { return fData.empty(); }
   size_type size() const noexcept { return fData.size(); }
   size_type max_size() const noexcept { return fData.size(); }
   void reserve(size_type new_cap) { fData.reserve(new_cap); }
   size_type capacity() const noexcept { return fData.capacity(); }
   void shrink_to_fit() { fData.shrink_to_fit(); };
   // modifiers
   void clear() noexcept { fData.clear(); }
   iterator erase(iterator pos) { return fData.erase(pos); }
   iterator erase(iterator first, iterator last) { return fData.erase(first, last); }
   void push_back(T &&value) { fData.push_back(std::forward<T>(value)); }
   void push_back(const value_type &value) { fData.push_back(value); };
   template <class... Args>
   reference emplace_back(Args &&... args)
   {
      ::ROOT::Internal::VecOps::EmplaceBack(fData, std::forward<Args>(args)...);
      return fData.back();
   }
   /// This method is intended only for arithmetic types unlike the std::vector
   /// corresponding one which is generic.
   template <typename U = T, typename std::enable_if<std::is_arithmetic<U>::value, int> * = nullptr>
   iterator emplace(const_iterator pos, U value)
   {
      return fData.emplace(pos, value);
   }
   void pop_back() { fData.pop_back(); }
   void resize(size_type count) { fData.resize(count); }
   void resize(size_type count, const value_type &value) { fData.resize(count, value); }
};

///@name RVec Unary Arithmetic Operators
///@{

#define TVEC_UNARY_OPERATOR(OP)          \
   template <typename T>                 \
   RVec<T> operator OP(const RVec<T> &v) \
   {                                     \
      RVec<T> ret(v);                    \
      for (auto &x : ret)                \
         x = OP x;                       \
      return ret;                        \
   }

TVEC_UNARY_OPERATOR(+)
TVEC_UNARY_OPERATOR(-)
TVEC_UNARY_OPERATOR(~)
TVEC_UNARY_OPERATOR(!)
#undef TVEC_UNARY_OPERATOR

///@}
///@name RVec Binary Arithmetic Operators
///@{

#define ERROR_MESSAGE(OP) "Cannot call operator " #OP " on vectors of different sizes."

#define TVEC_BINARY_OPERATOR(OP)                                                            \
   template <typename T0, typename T1>                                                      \
   auto operator OP(const RVec<T0> &v, const T1 &y)->RVec<decltype(v[0] OP y)>              \
   {                                                                                        \
      RVec<decltype(v[0] OP y)> ret(v.size());                                              \
      auto op = [&y](const T0 &x) { return x OP y; };                                       \
      std::transform(v.begin(), v.end(), ret.begin(), op);                                  \
      return ret;                                                                           \
   }                                                                                        \
                                                                                            \
   template <typename T0, typename T1>                                                      \
   auto operator OP(const T0 &x, const RVec<T1> &v)->RVec<decltype(x OP v[0])>              \
   {                                                                                        \
      RVec<decltype(x OP v[0])> ret(v.size());                                              \
      auto op = [&x](const T1 &y) { return x OP y; };                                       \
      std::transform(v.begin(), v.end(), ret.begin(), op);                                  \
      return ret;                                                                           \
   }                                                                                        \
                                                                                            \
   template <typename T0, typename T1>                                                      \
   auto operator OP(const RVec<T0> &v0, const RVec<T1> &v1)->RVec<decltype(v0[0] OP v1[0])> \
   {                                                                                        \
      if (v0.size() != v1.size())                                                           \
         throw std::runtime_error(ERROR_MESSAGE(OP));                                       \
                                                                                            \
      RVec<decltype(v0[0] OP v1[0])> ret(v0.size());                                        \
      auto op = [](const T0 &x, const T1 &y) { return x OP y; };                            \
      std::transform(v0.begin(), v0.end(), v1.begin(), ret.begin(), op);                    \
      return ret;                                                                           \
   }

TVEC_BINARY_OPERATOR(+)
TVEC_BINARY_OPERATOR(-)
TVEC_BINARY_OPERATOR(*)
TVEC_BINARY_OPERATOR(/)
TVEC_BINARY_OPERATOR(%)
TVEC_BINARY_OPERATOR (^)
TVEC_BINARY_OPERATOR(|)
TVEC_BINARY_OPERATOR(&)
#undef TVEC_BINARY_OPERATOR

///@}
///@name RVec Assignment Arithmetic Operators
///@{

#define TVEC_ASSIGNMENT_OPERATOR(OP)                                    \
   template <typename T0, typename T1>                                  \
   RVec<T0> &operator OP(RVec<T0> &v, const T1 &y)                      \
   {                                                                    \
      auto op = [&y](T0 &x) { return x OP y; };                         \
      std::transform(v.begin(), v.end(), v.begin(), op);                \
      return v;                                                         \
   }                                                                    \
                                                                        \
   template <typename T0, typename T1>                                  \
   RVec<T0> &operator OP(RVec<T0> &v0, const RVec<T1> &v1)              \
   {                                                                    \
      if (v0.size() != v1.size())                                       \
         throw std::runtime_error(ERROR_MESSAGE(OP));                   \
                                                                        \
      auto op = [](T0 &x, const T1 &y) { return x OP y; };              \
      std::transform(v0.begin(), v0.end(), v1.begin(), v0.begin(), op); \
      return v0;                                                        \
   }

TVEC_ASSIGNMENT_OPERATOR(+=)
TVEC_ASSIGNMENT_OPERATOR(-=)
TVEC_ASSIGNMENT_OPERATOR(*=)
TVEC_ASSIGNMENT_OPERATOR(/=)
TVEC_ASSIGNMENT_OPERATOR(%=)
TVEC_ASSIGNMENT_OPERATOR(^=)
TVEC_ASSIGNMENT_OPERATOR(|=)
TVEC_ASSIGNMENT_OPERATOR(&=)
TVEC_ASSIGNMENT_OPERATOR(>>=)
TVEC_ASSIGNMENT_OPERATOR(<<=)
#undef TVEC_ASSIGNMENT_OPERATOR

///@}
///@name RVec Comparison and Logical Operators
///@{

#define TVEC_LOGICAL_OPERATOR(OP)                                                                    \
   template <typename T0, typename T1>                                                               \
   auto operator OP(const RVec<T0> &v, const T1 &y)->RVec<int> /* avoid std::vector<bool> */         \
   {                                                                                                 \
      RVec<int> ret(v.size());                                                                       \
      auto op = [y](const T0 &x) -> int { return x OP y; };                                          \
      std::transform(v.begin(), v.end(), ret.begin(), op);                                           \
      return ret;                                                                                    \
   }                                                                                                 \
                                                                                                     \
   template <typename T0, typename T1>                                                               \
   auto operator OP(const T0 &x, const RVec<T1> &v)->RVec<int> /* avoid std::vector<bool> */         \
   {                                                                                                 \
      RVec<int> ret(v.size());                                                                       \
      auto op = [x](const T1 &y) -> int { return x OP y; };                                          \
      std::transform(v.begin(), v.end(), ret.begin(), op);                                           \
      return ret;                                                                                    \
   }                                                                                                 \
                                                                                                     \
   template <typename T0, typename T1>                                                               \
   auto operator OP(const RVec<T0> &v0, const RVec<T1> &v1)->RVec<int> /* avoid std::vector<bool> */ \
   {                                                                                                 \
      if (v0.size() != v1.size())                                                                    \
         throw std::runtime_error(ERROR_MESSAGE(OP));                                                \
                                                                                                     \
      RVec<int> ret(v0.size());                                                                      \
      auto op = [](const T0 &x, const T1 &y) -> int { return x OP y; };                              \
      std::transform(v0.begin(), v0.end(), v1.begin(), ret.begin(), op);                             \
      return ret;                                                                                    \
   }

TVEC_LOGICAL_OPERATOR(<)
TVEC_LOGICAL_OPERATOR(>)
TVEC_LOGICAL_OPERATOR(==)
TVEC_LOGICAL_OPERATOR(!=)
TVEC_LOGICAL_OPERATOR(<=)
TVEC_LOGICAL_OPERATOR(>=)
TVEC_LOGICAL_OPERATOR(&&)
TVEC_LOGICAL_OPERATOR(||)
#undef TVEC_LOGICAL_OPERATOR

///@}
///@name RVec Standard Mathematical Functions
///@{

/// \cond
template <typename T>
struct PromoteTypeImpl;

template <>
struct PromoteTypeImpl<float> {
   using Type = float;
};
template <>
struct PromoteTypeImpl<double> {
   using Type = double;
};
template <>
struct PromoteTypeImpl<long double> {
   using Type = long double;
};

template <typename T>
struct PromoteTypeImpl {
   using Type = double;
};

template <typename T>
using PromoteType = typename PromoteTypeImpl<T>::Type;

template <typename U, typename V>
using PromoteTypes = decltype(PromoteType<U>() + PromoteType<V>());

/// \endcond

#define TVEC_UNARY_FUNCTION(NAME, FUNC)                   \
   template <typename T>                                  \
   RVec<PromoteType<T>> NAME(const RVec<T> &v)            \
   {                                                      \
      RVec<PromoteType<T>> ret(v.size());                 \
      auto f = [](const T &x) { return FUNC(x); };        \
      std::transform(v.begin(), v.end(), ret.begin(), f); \
      return ret;                                         \
   }

#define TVEC_BINARY_FUNCTION(NAME, FUNC)                                   \
   template <typename T0, typename T1>                                     \
   RVec<PromoteTypes<T0, T1>> NAME(const T0 &x, const RVec<T1> &v)         \
   {                                                                       \
      RVec<PromoteTypes<T0, T1>> ret(v.size());                            \
      auto f = [&x](const T1 &y) { return FUNC(x, y); };                   \
      std::transform(v.begin(), v.end(), ret.begin(), f);                  \
      return ret;                                                          \
   }                                                                       \
                                                                           \
   template <typename T0, typename T1>                                     \
   RVec<PromoteTypes<T0, T1>> NAME(const RVec<T0> &v, const T1 &y)         \
   {                                                                       \
      RVec<PromoteTypes<T0, T1>> ret(v.size());                            \
      auto f = [&y](const T1 &x) { return FUNC(x, y); };                   \
      std::transform(v.begin(), v.end(), ret.begin(), f);                  \
      return ret;                                                          \
   }                                                                       \
                                                                           \
   template <typename T0, typename T1>                                     \
   RVec<PromoteTypes<T0, T1>> NAME(const RVec<T0> &v0, const RVec<T1> &v1) \
   {                                                                       \
      if (v0.size() != v1.size())                                          \
         throw std::runtime_error(ERROR_MESSAGE(NAME));                    \
                                                                           \
      RVec<PromoteTypes<T0, T1>> ret(v0.size());                           \
      auto f = [](const T0 &x, const T1 &y) { return FUNC(x, y); };        \
      std::transform(v0.begin(), v0.end(), v1.begin(), ret.begin(), f);    \
      return ret;                                                          \
   }

#define TVEC_STD_UNARY_FUNCTION(F) TVEC_UNARY_FUNCTION(F, std::F)
#define TVEC_STD_BINARY_FUNCTION(F) TVEC_BINARY_FUNCTION(F, std::F)

TVEC_STD_UNARY_FUNCTION(abs)
TVEC_STD_BINARY_FUNCTION(fdim)
TVEC_STD_BINARY_FUNCTION(fmod)
TVEC_STD_BINARY_FUNCTION(remainder)

TVEC_STD_UNARY_FUNCTION(exp)
TVEC_STD_UNARY_FUNCTION(exp2)
TVEC_STD_UNARY_FUNCTION(expm1)

TVEC_STD_UNARY_FUNCTION(log)
TVEC_STD_UNARY_FUNCTION(log10)
TVEC_STD_UNARY_FUNCTION(log2)
TVEC_STD_UNARY_FUNCTION(log1p)

TVEC_STD_BINARY_FUNCTION(pow)
TVEC_STD_UNARY_FUNCTION(sqrt)
TVEC_STD_UNARY_FUNCTION(cbrt)
TVEC_STD_BINARY_FUNCTION(hypot)

TVEC_STD_UNARY_FUNCTION(sin)
TVEC_STD_UNARY_FUNCTION(cos)
TVEC_STD_UNARY_FUNCTION(tan)
TVEC_STD_UNARY_FUNCTION(asin)
TVEC_STD_UNARY_FUNCTION(acos)
TVEC_STD_UNARY_FUNCTION(atan)
TVEC_STD_BINARY_FUNCTION(atan2)

TVEC_STD_UNARY_FUNCTION(sinh)
TVEC_STD_UNARY_FUNCTION(cosh)
TVEC_STD_UNARY_FUNCTION(tanh)
TVEC_STD_UNARY_FUNCTION(asinh)
TVEC_STD_UNARY_FUNCTION(acosh)
TVEC_STD_UNARY_FUNCTION(atanh)

TVEC_STD_UNARY_FUNCTION(floor)
TVEC_STD_UNARY_FUNCTION(ceil)
TVEC_STD_UNARY_FUNCTION(trunc)
TVEC_STD_UNARY_FUNCTION(round)
TVEC_STD_UNARY_FUNCTION(lround)
TVEC_STD_UNARY_FUNCTION(llround)

TVEC_STD_UNARY_FUNCTION(erf)
TVEC_STD_UNARY_FUNCTION(erfc)
TVEC_STD_UNARY_FUNCTION(lgamma)
TVEC_STD_UNARY_FUNCTION(tgamma)
#undef TVEC_STD_UNARY_FUNCTION

///@}
///@name RVec Fast Mathematical Functions with Vdt
///@{

#ifdef R__HAS_VDT
#define TVEC_VDT_UNARY_FUNCTION(F) TVEC_UNARY_FUNCTION(F, vdt::F)

TVEC_VDT_UNARY_FUNCTION(fast_expf)
TVEC_VDT_UNARY_FUNCTION(fast_logf)
TVEC_VDT_UNARY_FUNCTION(fast_sinf)
TVEC_VDT_UNARY_FUNCTION(fast_cosf)
TVEC_VDT_UNARY_FUNCTION(fast_tanf)
TVEC_VDT_UNARY_FUNCTION(fast_asinf)
TVEC_VDT_UNARY_FUNCTION(fast_acosf)
TVEC_VDT_UNARY_FUNCTION(fast_atanf)

TVEC_VDT_UNARY_FUNCTION(fast_exp)
TVEC_VDT_UNARY_FUNCTION(fast_log)
TVEC_VDT_UNARY_FUNCTION(fast_sin)
TVEC_VDT_UNARY_FUNCTION(fast_cos)
TVEC_VDT_UNARY_FUNCTION(fast_tan)
TVEC_VDT_UNARY_FUNCTION(fast_asin)
TVEC_VDT_UNARY_FUNCTION(fast_acos)
TVEC_VDT_UNARY_FUNCTION(fast_atan)
#undef TVEC_VDT_UNARY_FUNCTION

#endif // R__HAS_VDT

#undef TVEC_UNARY_FUNCTION

///@}

/// Inner product
///
/// Example code, at the ROOT prompt:
/// ~~~{.cpp}
/// using namespace ROOT::VecOps;
/// RVec<float> v1 {1., 2., 3.};
/// RVec<float> v2 {4., 5., 6.};
/// auto v1_dot_v2 = Dot(v1, v2);
/// v1_dot_v2
/// // (float) 32.f
/// ~~~
template <typename T, typename V>
auto Dot(const RVec<T> &v0, const RVec<V> &v1) -> decltype(v0[0] * v1[0])
{
   if (v0.size() != v1.size())
      throw std::runtime_error("Cannot compute inner product of vectors of different sizes");
   return std::inner_product(v0.begin(), v0.end(), v1.begin(), decltype(v0[0] * v1[0])(0));
}

/// Sum elements of an RVec
///
/// Example code, at the ROOT prompt:
/// ~~~{.cpp}
/// using namespace ROOT::VecOps;
/// RVec<float> v {1.f, 2.f, 3.f};
/// auto v_sum = Sum(v);
/// v_sum
/// // (float) 6.f
/// ~~~
template <typename T>
T Sum(const RVec<T> &v)
{
   return std::accumulate(v.begin(), v.end(), T(0));
}

/// Get the mean of the elements of an RVec
///
/// The return type is a double precision floating point number.
/// Example code, at the ROOT prompt:
/// ~~~{.cpp}
/// using namespace ROOT::VecOps;
/// RVec<float> v {1.f, 2.f, 4.f};
/// auto v_mean = Mean(v);
/// v_mean
/// // (double) 2.3333333
/// ~~~
template <typename T>
double Mean(const RVec<T> &v)
{
   if (v.empty())
      return 0.;
   return double(Sum(v)) / v.size();
}

/// Get the greatest element of an RVec
///
/// Example code, at the ROOT prompt:
/// ~~~~{.cpp}
/// using namespace ROOT::VecOps;
/// RVec<float> v {1.f, 2.f, 4.f};
/// auto v_max = Max(v)
/// v_max
/// (float) 4.f
/// ~~~~
template <typename T>
T Max(const RVec<T> &v)
{
   return *std::max_element(v.begin(), v.end());
}

/// Get the smallest element of an RVec
///
/// Example code, at the ROOT prompt:
/// ~~~~{.cpp}
/// using namespace ROOT::VecOps;
/// RVec<float> v {1.f, 2.f, 4.f};
/// auto v_min = Min(v)
/// v_min
/// (float) 1.f
/// ~~~~
template <typename T>
T Min(const RVec<T> &v)
{
   return *std::min_element(v.begin(), v.end());
}

/// Get the index of the greatest element of an RVec
/// In case of multiple occurrences of the maximum values,
/// the index corresponding to the first occurrence is returned.
///
/// Example code, at the ROOT prompt:
/// ~~~~{.cpp}
/// using namespace ROOT::VecOps;
/// RVec<float> v {1.f, 2.f, 4.f};
/// auto v_argmax = ArgMax(v);
/// v_argmax
/// // (int) 2
/// ~~~~
template <typename T>
std::size_t ArgMax(const RVec<T> &v)
{
   return std::distance(v.begin(), std::max_element(v.begin(), v.end()));
}

/// Get the index of the smallest element of an RVec
/// In case of multiple occurrences of the minimum values,
/// the index corresponding to the first occurrence is returned.
///
/// Example code, at the ROOT prompt:
/// ~~~~{.cpp}
/// using namespace ROOT::VecOps;
/// RVec<float> v {1.f, 2.f, 4.f};
/// auto v_argmin = ArgMin(v);
/// v_argmin
/// // (int) 0
/// ~~~~
template <typename T>
std::size_t ArgMin(const RVec<T> &v)
{
   return std::distance(v.begin(), std::min_element(v.begin(), v.end()));
}

/// Get the variance of the elements of an RVec
///
/// The return type is a double precision floating point number.
/// Example code, at the ROOT prompt:
/// ~~~{.cpp}
/// using namespace ROOT::VecOps;
/// RVec<float> v {1.f, 2.f, 4.f};
/// auto v_var = Var(v);
/// v_var
/// // (double) 2.3333333
/// ~~~
template <typename T>
double Var(const RVec<T> &v)
{
   const std::size_t size = v.size();
   if (size < std::size_t(2))
      return 0.;
   T sum_squares(0), squared_sum(0);
   auto pred = [&sum_squares, &squared_sum](const T &x) {
      sum_squares += x * x;
      squared_sum += x;
   };
   std::for_each(v.begin(), v.end(), pred);
   squared_sum *= squared_sum;
   const auto dsize = (double)size;
   return 1. / (dsize - 1.) * (sum_squares - squared_sum / dsize);
}

/// Get the standard deviation of the elements of an RVec
///
/// The return type is a double precision floating point number.
/// Example code, at the ROOT prompt:
/// ~~~{.cpp}
/// using namespace ROOT::VecOps;
/// RVec<float> v {1.f, 2.f, 4.f};
/// auto v_sd = StdDev(v);
/// v_sd
/// // (double) 1.5275252
/// ~~~
template <typename T>
double StdDev(const RVec<T> &v)
{
   return std::sqrt(Var(v));
}

/// Create new collection applying a callable to the elements of the input collection
///
/// Example code, at the ROOT prompt:
/// ~~~{.cpp}
/// using namespace ROOT::VecOps;
/// RVec<float> v {1.f, 2.f, 4.f};
/// auto v_square = Map(v, [](float f){return f* 2.f;});
/// v_square
/// // (ROOT::VecOps::RVec<float> &) { 2.00000f, 4.00000f, 8.00000f }
///
/// RVec<float> x({1.f, 2.f, 3.f});
/// RVec<float> y({4.f, 5.f, 6.f});
/// RVec<float> z({7.f, 8.f, 9.f});
/// auto mod = [](float x, float y, float z) { return sqrt(x * x + y * y + z * z); };
/// auto v_mod = Map(x, y, z, mod);
/// v_mod
/// // (ROOT::VecOps::RVec<float> &) { 8.12404f, 9.64365f, 11.2250f }
/// ~~~
template <typename... Args>
auto Map(Args &&... args)
   -> decltype(ROOT::Detail::VecOps::MapFromTuple(std::forward_as_tuple(args...),
                                                  std::make_index_sequence<sizeof...(args) - 1>()))
{
   /*
   Here the strategy in order to generalise the previous implementation of Map, i.e.
   `RVec Map(RVec, F)`, here we need to move the last parameter of the pack in first
   position in order to be able to invoke the Map function with automatic type deduction.
   This is achieved in two steps:
   1. Forward as tuple the pack to MapFromTuple
   2. Invoke the MapImpl helper which has the signature `template<...T, F> RVec MapImpl(F &&f, RVec<T>...)`
   NOTA BENE: the signature is very heavy but it is one of the lightest ways to manage in C++11
   to build the return type based on the template args.
   */
   return ROOT::Detail::VecOps::MapFromTuple(std::forward_as_tuple(args...),
                                             std::make_index_sequence<sizeof...(args) - 1>());
}

/// Create a new collection with the elements passing the filter expressed by the predicate
///
/// Example code, at the ROOT prompt:
/// ~~~{.cpp}
/// using namespace ROOT::VecOps;
/// RVec<int> v {1, 2, 4};
/// auto v_even = Filter(v, [](int i){return 0 == i%2;});
/// v_even
/// // (ROOT::VecOps::RVec<int> &) { 2, 4 }
/// ~~~
template <typename T, typename F>
RVec<T> Filter(const RVec<T> &v, F &&f)
{
   const auto thisSize = v.size();
   RVec<T> w;
   w.reserve(thisSize);
   for (auto &&val : v) {
      if (f(val))
         w.emplace_back(val);
   }
   return w;
}

/// Return true if any of the elements equates to true, return false otherwise.
///
/// Example code, at the ROOT prompt:
/// ~~~{.cpp}
/// using namespace ROOT::VecOps;
/// RVec<int> v {0, 1, 0};
/// auto anyTrue = Any(v);
/// anyTrue
/// // (bool) true
/// ~~~
template <typename T>
auto Any(const RVec<T> &v) -> decltype(v[0] == true)
{
   for (auto &&e : v)
      if (e == true)
         return true;
   return false;
}

/// Return true if all of the elements equate to true, return false otherwise.
///
/// Example code, at the ROOT prompt:
/// ~~~{.cpp}
/// using namespace ROOT::VecOps;
/// RVec<int> v {0, 1, 0};
/// auto allTrue = All(v);
/// allTrue
/// // (bool) false
/// ~~~
template <typename T>
auto All(const RVec<T> &v) -> decltype(v[0] == false)
{
   for (auto &&e : v)
      if (e == false)
         return false;
   return true;
}

/// Return an RVec of indices that sort the input RVec
///
/// Example code, at the ROOT prompt:
/// ~~~{.cpp}
/// using namespace ROOT::VecOps;
/// RVec<double> v {2., 3., 1.};
/// auto sortIndices = Argsort(v);
/// sortIndices
/// // (ROOT::VecOps::RVec<unsigned long> &) { 2, 0, 1 }
/// ~~~
template <typename T>
RVec<typename RVec<T>::size_type> Argsort(const RVec<T> &v)
{
   using size_type = typename RVec<T>::size_type;
   RVec<size_type> i(v.size());
   std::iota(i.begin(), i.end(), 0);
   std::sort(i.begin(), i.end(), [&v](size_type i1, size_type i2) { return v[i1] < v[i2]; });
   return i;
}

/// Return elements of a vector at given indices
///
/// Example code, at the ROOT prompt:
/// ~~~{.cpp}
/// using namespace ROOT::VecOps;
/// RVec<double> v {2., 3., 1.};
/// auto vTaken = Take(v, {0,2});
/// vTaken
/// // (ROOT::VecOps::RVec<double>) { 2.0000000, 1.0000000 }
/// ~~~
template <typename T>
RVec<T> Take(const RVec<T> &v, const RVec<typename RVec<T>::size_type> &i)
{
   using size_type = typename RVec<T>::size_type;
   const size_type isize = i.size();
   RVec<T> r(isize);
   for (size_type k = 0; k < isize; k++)
      r[k] = v[i[k]];
   return r;
}

/// Return first or last `n` elements of an RVec
///
/// if `n > 0` and last elements if `n < 0`.
///
/// Example code, at the ROOT prompt:
/// ~~~{.cpp}
/// using namespace ROOT::VecOps;
/// RVec<double> v {2., 3., 1.};
/// auto firstTwo = Take(v, 2);
/// firstTwo
/// // (ROOT::VecOps::RVec<double>) { 2.0000000, 3.0000000 }
/// auto lastOne = Take(v, -1);
/// lastOne
/// // (ROOT::VecOps::RVec<double>) { 1.0000000 }
/// ~~~
template <typename T>
RVec<T> Take(const RVec<T> &v, const int n)
{
   using size_type = typename RVec<T>::size_type;
   const size_type size = v.size();
   const size_type absn = std::abs(n);
   if (absn > size) {
      std::stringstream ss;
      ss << "Try to take " << absn << " elements but vector has only size " << size << ".";
      throw std::runtime_error(ss.str());
   }
   RVec<T> r(absn);
   if (n < 0) {
      for (size_type k = 0; k < absn; k++)
         r[k] = v[size - absn + k];
   } else {
      for (size_type k = 0; k < absn; k++)
         r[k] = v[k];
   }
   return r;
}

/// Return copy of reversed vector
///
/// Example code, at the ROOT prompt:
/// ~~~{.cpp}
/// using namespace ROOT::VecOps;
/// RVec<double> v {2., 3., 1.};
/// auto v_reverse = Reverse(v);
/// v_reverse
/// // (ROOT::VecOps::RVec<double>) { 1.0000000, 3.0000000, 2.0000000 }
/// ~~~
template <typename T>
RVec<T> Reverse(const RVec<T> &v)
{
   RVec<T> r(v);
   std::reverse(r.begin(), r.end());
   return r;
}

/// Return copy of RVec with elements sorted in ascending order
///
/// This helper is different from ArgSort since it does not return an RVec of indices,
/// but an RVec of values.
///
/// Example code, at the ROOT prompt:
/// ~~~{.cpp}
/// using namespace ROOT::VecOps;
/// RVec<double> v {2., 3., 1.};
/// auto v_sorted = Sort(v);
/// v_sorted
/// // (ROOT::VecOps::RVec<double>) { 1.0000000, 2.0000000, 3.0000000 }
/// ~~~
template <typename T>
RVec<T> Sort(const RVec<T> &v)
{
   RVec<T> r(v);
   std::sort(r.begin(), r.end());
   return r;
}

/// Return copy of RVec with elements sorted based on a comparison operator
///
/// The comparison operator has to fullfill the same requirements of the
/// predicate of by std::sort.
///
///
/// This helper is different from ArgSort since it does not return an RVec of indices,
/// but an RVec of values.
///
/// Example code, at the ROOT prompt:
/// ~~~{.cpp}
/// using namespace ROOT::VecOps;
/// RVec<double> v {2., 3., 1.};
/// auto v_sorted = Sort(v, [](double x, double y) {return 1/x < 1/y;});
/// v_sorted
/// // (ROOT::VecOps::RVec<double>) { 3.0000000, 2.0000000, 1.0000000 }
/// ~~~
template <typename T, typename Compare>
RVec<T> Sort(const RVec<T> &v, Compare &&c)
{
   RVec<T> r(v);
   std::sort(r.begin(), r.end(), std::forward<Compare>(c));
   return r;
}

/// Return the indices that represent all combinations of the elements of two
/// RVecs.
///
/// The type of the return value is an RVec of two RVecs containing indices.
///
/// Example code, at the ROOT prompt:
/// ~~~{.cpp}
/// using namespace ROOT::VecOps;
/// auto comb_idx = Combinations(3, 2);
/// comb_idx
/// // (ROOT::VecOps::RVec<ROOT::VecOps::RVec<ROOT::VecOps::RVec<double>::size_type> >) { { 0, 0, 1, 1, 2, 2 }, { 0, 1,
/// 0, 1, 0, 1 } }
/// ~~~
inline RVec<RVec<std::size_t>> Combinations(const std::size_t s1, const std::size_t s2)
{
   using size_type = std::size_t;
   RVec<RVec<size_type>> r(2);
   r[0].resize(s1 * s2);
   r[1].resize(s1 * s2);
   size_type c = 0;
   for (size_type i = 0; i < s1; i++) {
      for (size_type j = 0; j < s2; j++) {
         r[0][c] = i;
         r[1][c] = j;
         c++;
      }
   }
   return r;
}

/// Return the indices that represent all combinations of the elements of two
/// RVecs.
///
/// The type of the return value is an RVec of two RVecs containing indices.
///
/// Example code, at the ROOT prompt:
/// ~~~{.cpp}
/// using namespace ROOT::VecOps;
/// RVec<double> v1 {1., 2., 3.};
/// RVec<double> v2 {-4., -5.};
/// auto comb_idx = Combinations(v1, v2);
/// comb_idx
/// // (ROOT::VecOps::RVec<ROOT::VecOps::RVec<ROOT::VecOps::RVec<double>::size_type> >) { { 0, 0, 1, 1, 2, 2 }, { 0, 1,
/// 0, 1, 0, 1 } }
/// ~~~
template <typename T1, typename T2>
RVec<RVec<typename RVec<T1>::size_type>> Combinations(const RVec<T1> &v1, const RVec<T2> &v2)
{
   return Combinations(v1.size(), v2.size());
}

/// Return the indices that represent all unique combinations of the
/// elements of a given RVec.
///
/// ~~~{.cpp}
/// using namespace ROOT::VecOps;
/// RVec<double> v {1., 2., 3., 4.};
/// auto v_1 = Combinations(v, 1);
/// v_1
/// (ROOT::VecOps::RVec<ROOT::VecOps::RVec<ROOT::VecOps::RVec<double>::size_type> >) { { 0, 1, 2, 3 } }
/// auto v_2 = Combinations(v, 2);
/// auto v_2
/// (ROOT::VecOps::RVec<ROOT::VecOps::RVec<ROOT::VecOps::RVec<double>::size_type> >) { { 0, 0, 0, 1, 1, 2 }, { 1, 2, 3,
/// 2, 3, 3 } } auto v_3 = Combinations(v, 3); v_3
/// (ROOT::VecOps::RVec<ROOT::VecOps::RVec<ROOT::VecOps::RVec<double>::size_type> >) { { 0, 0, 0, 1 }, { 1, 1, 2, 2 }, {
/// 2, 3, 3, 3 } } auto v_4 = Combinations(v, 4); v_4
/// (ROOT::VecOps::RVec<ROOT::VecOps::RVec<ROOT::VecOps::RVec<double>::size_type> >) { { 0 }, { 1 }, { 2 }, { 3 } }
/// ~~~
template <typename T>
RVec<RVec<typename RVec<T>::size_type>> Combinations(const RVec<T> &v, const typename RVec<T>::size_type n)
{
   using size_type = typename RVec<T>::size_type;
   const size_type s = v.size();
   if (n > s) {
      std::stringstream ss;
      ss << "Cannot make unique combinations of size " << n << " from vector of size " << s << ".";
      throw std::runtime_error(ss.str());
   }
   RVec<size_type> indices(s);
   for (size_type k = 0; k < s; k++)
      indices[k] = k;
   RVec<RVec<size_type>> c(n);
   for (size_type k = 0; k < n; k++)
      c[k].emplace_back(indices[k]);
   while (true) {
      bool run_through = true;
      long i = n - 1;
      for (; i >= 0; i--) {
         if (indices[i] != i + s - n) {
            run_through = false;
            break;
         }
      }
      if (run_through) {
         return c;
      }
      indices[i]++;
      for (long j = i + 1; j < (long)n; j++)
         indices[j] = indices[j - 1] + 1;
      for (size_type k = 0; k < n; k++)
         c[k].emplace_back(indices[k]);
   }
}

/// Return the indices of the elements which are not zero
///
/// Example code, at the ROOT prompt:
/// ~~~{.cpp}
/// using namespace ROOT::VecOps;
/// RVec<double> v {2., 0., 3., 0., 1.};
/// auto nonzero_idx = Nonzero(v);
/// nonzero_idx
/// // (ROOT::VecOps::RVec<ROOT::VecOps::RVec<double>::size_type>) { 0, 2, 4 }
/// ~~~
template <typename T>
RVec<typename RVec<T>::size_type> Nonzero(const RVec<T> &v)
{
   using size_type = typename RVec<T>::size_type;
   RVec<size_type> r;
   const auto size = v.size();
   r.reserve(size);
   for (size_type i = 0; i < size; i++) {
      if (v[i] != 0) {
         r.emplace_back(i);
      }
   }
   return r;
}

/// Return the intersection of elements of two RVecs.
///
/// Each element of v1 is looked up in v2 and added to the returned vector if
/// found. Following, the order of v1 is preserved. If v2 is already sorted, the
/// optional argument v2_is_sorted can be used to toggle of the internal sorting
/// step, therewith optimising runtime.
///
/// Example code, at the ROOT prompt:
/// ~~~{.cpp}
/// using namespace ROOT::VecOps;
/// RVec<double> v1 {1., 2., 3.};
/// RVec<double> v2 {-4., -5., 2., 1.};
/// auto v1_intersect_v2 = Intersect(v1, v2);
/// v1_intersect_v2
/// // (ROOT::VecOps::RVec<double>) { 1.0000000, 2.0000000 }
/// ~~~
template <typename T>
RVec<T> Intersect(const RVec<T> &v1, const RVec<T> &v2, bool v2_is_sorted = false)
{
   RVec<T> v2_sorted;
   if (!v2_is_sorted)
      v2_sorted = Sort(v2);
   const auto v2_begin = v2_is_sorted ? v2.begin() : v2_sorted.begin();
   const auto v2_end = v2_is_sorted ? v2.end() : v2_sorted.end();
   RVec<T> r;
   const auto size = v1.size();
   r.reserve(size);
   using size_type = typename RVec<T>::size_type;
   for (size_type i = 0; i < size; i++) {
      if (std::binary_search(v2_begin, v2_end, v1[i])) {
         r.emplace_back(v1[i]);
      }
   }
   return r;
}

/// Return the elements of v1 if the condition c is true and v2 if the
/// condition c is false.
///
/// Example code, at the ROOT prompt:
/// ~~~{.cpp}
/// using namespace ROOT::VecOps;
/// RVec<double> v1 {1., 2., 3.};
/// RVec<double> v2 {-1., -2., -3.};
/// auto c = v1 > 1;
/// c
/// // (ROOT::VecOps::RVec<int> &) { 0, 1, 1 }
/// auto if_c_v1_else_v2 = Where(c, v1, v2);
/// if_c_v1_else_v2
/// // (ROOT::VecOps::RVec<double> &) { -1.0000000, 2.0000000, 3.0000000 }
/// ~~~
template <typename T>
RVec<T> Where(const RVec<int> &c, const RVec<T> &v1, const RVec<T> &v2)
{
   using size_type = typename RVec<T>::size_type;
   const size_type size = c.size();
   RVec<T> r;
   r.reserve(size);
   for (size_type i = 0; i < size; i++) {
      r.emplace_back(c[i] != 0 ? v1[i] : v2[i]);
   }
   return r;
}

/// Return the elements of v1 if the condition c is true and sets the value v2
/// if the condition c is false.
///
/// Example code, at the ROOT prompt:
/// ~~~{.cpp}
/// using namespace ROOT::VecOps;
/// RVec<double> v1 {1., 2., 3.};
/// double v2 = 4.;
/// auto c = v1 > 1;
/// c
/// // (ROOT::VecOps::RVec<int> &) { 0, 1, 1 }
/// auto if_c_v1_else_v2 = Where(c, v1, v2);
/// if_c_v1_else_v2
/// // (ROOT::VecOps::RVec<double>) { 4.0000000, 2.0000000, 3.0000000 }
/// ~~~
template <typename T>
RVec<T> Where(const RVec<int> &c, const RVec<T> &v1, T v2)
{
   using size_type = typename RVec<T>::size_type;
   const size_type size = c.size();
   RVec<T> r;
   r.reserve(size);
   for (size_type i = 0; i < size; i++) {
      r.emplace_back(c[i] != 0 ? v1[i] : v2);
   }
   return r;
}

/// Return the elements of v2 if the condition c is false and sets the value v1
/// if the condition c is true.
///
/// Example code, at the ROOT prompt:
/// ~~~{.cpp}
/// using namespace ROOT::VecOps;
/// double v1 = 4.;
/// RVec<double> v2 {1., 2., 3.};
/// auto c = v2 > 1;
/// c
/// // (ROOT::VecOps::RVec<int> &) { 0, 1, 1 }
/// auto if_c_v1_else_v2 = Where(c, v1, v2);
/// if_c_v1_else_v2
/// // (ROOT::VecOps::RVec<double>) { 1.0000000, 4.0000000, 4.0000000 }
/// ~~~
template <typename T>
RVec<T> Where(const RVec<int> &c, T v1, const RVec<T> &v2)
{
   using size_type = typename RVec<T>::size_type;
   const size_type size = c.size();
   RVec<T> r;
   r.reserve(size);
   for (size_type i = 0; i < size; i++) {
      r.emplace_back(c[i] != 0 ? v1 : v2[i]);
   }
   return r;
}

/// Return a vector with the value v2 if the condition c is false and sets the
/// value v1 if the condition c is true.
///
/// Example code, at the ROOT prompt:
/// ~~~{.cpp}
/// using namespace ROOT::VecOps;
/// double v1 = 4.;
/// double v2 = 2.;
/// RVec<int> c {0, 1, 1};
/// auto if_c_v1_else_v2 = Where(c, v1, v2);
/// if_c_v1_else_v2
/// // (ROOT::VecOps::RVec<double>) { 2.0000000, 4.0000000, 4.0000000 }
/// ~~~
template <typename T>
RVec<T> Where(const RVec<int> &c, T v1, T v2)
{
   using size_type = typename RVec<T>::size_type;
   const size_type size = c.size();
   RVec<T> r;
   r.reserve(size);
   for (size_type i = 0; i < size; i++) {
      r.emplace_back(c[i] != 0 ? v1 : v2);
   }
   return r;
}

/// Return the concatenation of two RVecs.
///
/// Example code, at the ROOT prompt:
/// ~~~{.cpp}
/// using namespace ROOT::VecOps;
/// RVec<float> rvf {0.f, 1.f, 2.f};
/// RVec<int> rvi {7, 8, 9};
/// Concatenate(rvf, rvi);
/// // (ROOT::VecOps::RVec<float>) { 2.0000000, 4.0000000, 4.0000000 }
/// ~~~
template <typename T0, typename T1, typename Common_t = typename std::common_type<T0, T1>::type>
RVec<Common_t> Concatenate(const RVec<T0> &v0, const RVec<T1> &v1)
{
   RVec<Common_t> res;
   res.reserve(v0.size() + v1.size());
   auto &resAsVect = res.AsVector();
   auto &v0AsVect = v0.AsVector();
   auto &v1AsVect = v1.AsVector();
   resAsVect.insert(resAsVect.begin(), v0AsVect.begin(), v0AsVect.end());
   resAsVect.insert(resAsVect.end(), v1AsVect.begin(), v1AsVect.end());
   return res;
}

/// Return the angle difference \f$\Delta \phi\f$ of two scalars.
///
/// The function computes the closest angle from v1 to v2 with sign and is
/// therefore in the range \f$[-\pi, \pi]\f$.
/// The computation is done per default in radians \f$c = \pi\f$ but can be switched
/// to degrees \f$c = 180\f$.
template <typename T>
T DeltaPhi(T v1, T v2, const T c = ROOT::Internal::VecOps::pi)
{
   static_assert(std::is_floating_point<T>::value,
                 "DeltaPhi must be called with floating point values.");
   auto r = std::fmod(v2 - v1, 2.0 * c);
   if (r < -c) {
      r += 2.0 * c;
   }
   else if (r > c) {
      r -= 2.0 * c;
   }
   return r;
}

/// Return the angle difference \f$\Delta \phi\f$ in radians of two vectors.
///
/// The function computes the closest angle from v1 to v2 with sign and is
/// therefore in the range \f$[-\pi, \pi]\f$.
/// The computation is done per default in radians \f$c = \pi\f$ but can be switched
/// to degrees \f$c = 180\f$.
template <typename T>
RVec<T> DeltaPhi(const RVec<T>& v1, const RVec<T>& v2, const T c = ROOT::Internal::VecOps::pi)
{
   using size_type = typename RVec<T>::size_type;
   const size_type size = v1.size();
   auto r = RVec<T>(size);
   for (size_type i = 0; i < size; i++) {
      r[i] = DeltaPhi(v1[i], v2[i], c);
   }
   return r;
}

/// Return the angle difference \f$\Delta \phi\f$ in radians of a vector and a scalar.
///
/// The function computes the closest angle from v1 to v2 with sign and is
/// therefore in the range \f$[-\pi, \pi]\f$.
/// The computation is done per default in radians \f$c = \pi\f$ but can be switched
/// to degrees \f$c = 180\f$.
template <typename T>
RVec<T> DeltaPhi(const RVec<T>& v1, T v2, const T c = ROOT::Internal::VecOps::pi)
{
   using size_type = typename RVec<T>::size_type;
   const size_type size = v1.size();
   auto r = RVec<T>(size);
   for (size_type i = 0; i < size; i++) {
      r[i] = DeltaPhi(v1[i], v2, c);
   }
   return r;
}

/// Return the angle difference \f$\Delta \phi\f$ in radians of a scalar and a vector.
///
/// The function computes the closest angle from v1 to v2 with sign and is
/// therefore in the range \f$[-\pi, \pi]\f$.
/// The computation is done per default in radians \f$c = \pi\f$ but can be switched
/// to degrees \f$c = 180\f$.
template <typename T>
RVec<T> DeltaPhi(T v1, const RVec<T>& v2, const T c = ROOT::Internal::VecOps::pi)
{
   using size_type = typename RVec<T>::size_type;
   const size_type size = v2.size();
   auto r = RVec<T>(size);
   for (size_type i = 0; i < size; i++) {
      r[i] = DeltaPhi(v1, v2[i], c);
   }
   return r;
}

/// Return the square of the distance on the \f$\eta\f$-\f$\phi\f$ plane (\f$\Delta R\f$) from
/// the collections eta1, eta2, phi1 and phi2.
///
/// The function computes \f$\Delta R^2 = (\eta_1 - \eta_2)^2 + (\phi_1 - \phi_2)^2\f$
/// of the given collections eta1, eta2, phi1 and phi2. The angle \f$\phi\f$ can
/// be set to radian or degrees using the optional argument c, see the documentation
/// of the DeltaPhi helper.
template <typename T>
RVec<T> DeltaR2(const RVec<T>& eta1, const RVec<T>& eta2, const RVec<T>& phi1, const RVec<T>& phi2, const T c = M_PI)
{
   const auto dphi = DeltaPhi(phi1, phi2, c);
   return (eta1 - eta2) * (eta1 - eta2) + dphi * dphi;
}

/// Return the distance on the \f$\eta\f$-\f$\phi\f$ plane (\f$\Delta R\f$) from
/// the collections eta1, eta2, phi1 and phi2.
///
/// The function computes \f$\Delta R = \sqrt{(\eta_1 - \eta_2)^2 + (\phi_1 - \phi_2)^2}\f$
/// of the given collections eta1, eta2, phi1 and phi2. The angle \f$\phi\f$ can
/// be set to radian or degrees using the optional argument c, see the documentation
/// of the DeltaPhi helper.
template <typename T>
RVec<T> DeltaR(const RVec<T>& eta1, const RVec<T>& eta2, const RVec<T>& phi1, const RVec<T>& phi2,
               const T c = ROOT::Internal::VecOps::pi)
{
   return sqrt(DeltaR2(eta1, eta2, phi1, phi2, c));
}

/// Return the distance on the \f$\eta\f$-\f$\phi\f$ plane (\f$\Delta R\f$) from
/// the scalars eta1, eta2, phi1 and phi2.
///
/// The function computes \f$\Delta R = \sqrt{(\eta_1 - \eta_2)^2 + (\phi_1 - \phi_2)^2}\f$
/// of the given scalars eta1, eta2, phi1 and phi2. The angle \f$\phi\f$ can
/// be set to radian or degrees using the optional argument c, see the documentation
/// of the DeltaPhi helper.
template <typename T>
T DeltaR(T eta1, T eta2, T phi1, T phi2, const T c = ROOT::Internal::VecOps::pi)
{
   const auto dphi = DeltaPhi(phi1, phi2, c);
   return std::sqrt((eta1 - eta2) * (eta1 - eta2) + dphi * dphi);
}

/// Return the invariant mass of two particles given the collections of the quantities
/// transverse momentum (pt), rapidity (eta), azimuth (phi) and mass.
///
/// The function computes the invariant mass of two particles with the four-vectors
/// (pt1, eta2, phi1, mass1) and (pt2, eta2, phi2, mass2).
template <typename T>
RVec<T> InvariantMass(
        const RVec<T>& pt1, const RVec<T>& eta1, const RVec<T>& phi1, const RVec<T>& mass1,
        const RVec<T>& pt2, const RVec<T>& eta2, const RVec<T>& phi2, const RVec<T>& mass2)
{
   // Conversion from (pt, eta, phi, mass) to (x, y, z, e) coordinate system
   const auto x1 = pt1 * cos(phi1);
   const auto y1 = pt1 * sin(phi1);
   const auto z1 = pt1 * sinh(eta1);
   const auto e1 = sqrt(x1 * x1 + y1 * y1 + z1 * z1 + mass1 * mass1);

   const auto x2 = pt2 * cos(phi2);
   const auto y2 = pt2 * sin(phi2);
   const auto z2 = pt2 * sinh(eta2);
   const auto e2 = sqrt(x2 * x2 + y2 * y2 + z2 * z2 + mass2 * mass2);

   // Addition of particle four-vectors
   const auto e = e1 + e2;
   const auto x = x1 + x2;
   const auto y = y1 + y2;
   const auto z = z1 + z2;

   // Return invariant mass with (+, -, -, -) metric
   return sqrt(e * e - x * x - y * y - z * z);
}

/// Return the invariant mass of multiple particles given the collections of the
/// quantities transverse momentum (pt), rapidity (eta), azimuth (phi) and mass.
///
/// The function computes the invariant mass of multiple particles with the
/// four-vectors (pt, eta, phi, mass).
template <typename T>
T InvariantMass(const RVec<T>& pt, const RVec<T>& eta, const RVec<T>& phi, const RVec<T>& mass)
{
   // Conversion from (mass, pt, eta, phi) to (e, x, y, z) coordinate system
   const auto x = pt * cos(phi);
   const auto y = pt * sin(phi);
   const auto z = pt * sinh(eta);
   const auto e = sqrt(x * x + y * y + z * z + mass * mass);

   // Addition of particle four-vectors
   const auto xs = Sum(x);
   const auto ys = Sum(y);
   const auto zs = Sum(z);
   const auto es = Sum(e);

   // Return invariant mass with (+, -, -, -) metric
   return std::sqrt(es * es - xs * xs - ys * ys - zs * zs);
}

////////////////////////////////////////////////////////////////////////////////
/// Print a RVec at the prompt:
template <class T>
std::ostream &operator<<(std::ostream &os, const RVec<T> &v)
{
   // In order to print properly, convert to 64 bit int if this is a char
   constexpr bool mustConvert = std::is_same<char, T>::value || std::is_same<signed char, T>::value ||
                                std::is_same<unsigned char, T>::value || std::is_same<wchar_t, T>::value ||
                                std::is_same<char16_t, T>::value || std::is_same<char32_t, T>::value;
   using Print_t = typename std::conditional<mustConvert, long long int, T>::type;
   os << "{ ";
   auto size = v.size();
   if (size) {
      for (std::size_t i = 0; i < size - 1; ++i) {
         os << (Print_t)v[i] << ", ";
      }
      os << (Print_t)v[size - 1];
   }
   os << " }";
   return os;
}

#if (_VECOPS_USE_EXTERN_TEMPLATES)

#define TVEC_EXTERN_UNARY_OPERATOR(T, OP) extern template RVec<T> operator OP<T>(const RVec<T> &);

#define TVEC_EXTERN_BINARY_OPERATOR(T, OP)                                                          \
   extern template auto operator OP<T, T>(const T &x, const RVec<T> &v)->RVec<decltype(x OP v[0])>; \
   extern template auto operator OP<T, T>(const RVec<T> &v, const T &y)->RVec<decltype(v[0] OP y)>; \
   extern template auto operator OP<T, T>(const RVec<T> &v0, const RVec<T> &v1)->RVec<decltype(v0[0] OP v1[0])>;

#define TVEC_EXTERN_ASSIGN_OPERATOR(T, OP)                           \
   extern template RVec<T> &operator OP<T, T>(RVec<T> &, const T &); \
   extern template RVec<T> &operator OP<T, T>(RVec<T> &, const RVec<T> &);

#define TVEC_EXTERN_LOGICAL_OPERATOR(T, OP)                                 \
   extern template RVec<int> operator OP<T, T>(const RVec<T> &, const T &); \
   extern template RVec<int> operator OP<T, T>(const T &, const RVec<T> &); \
   extern template RVec<int> operator OP<T, T>(const RVec<T> &, const RVec<T> &);

#define TVEC_EXTERN_FLOAT_TEMPLATE(T)  \
   extern template class RVec<T>;      \
   TVEC_EXTERN_UNARY_OPERATOR(T, +)    \
   TVEC_EXTERN_UNARY_OPERATOR(T, -)    \
   TVEC_EXTERN_UNARY_OPERATOR(T, !)    \
   TVEC_EXTERN_BINARY_OPERATOR(T, +)   \
   TVEC_EXTERN_BINARY_OPERATOR(T, -)   \
   TVEC_EXTERN_BINARY_OPERATOR(T, *)   \
   TVEC_EXTERN_BINARY_OPERATOR(T, /)   \
   TVEC_EXTERN_ASSIGN_OPERATOR(T, +=)  \
   TVEC_EXTERN_ASSIGN_OPERATOR(T, -=)  \
   TVEC_EXTERN_ASSIGN_OPERATOR(T, *=)  \
   TVEC_EXTERN_ASSIGN_OPERATOR(T, /=)  \
   TVEC_EXTERN_LOGICAL_OPERATOR(T, <)  \
   TVEC_EXTERN_LOGICAL_OPERATOR(T, >)  \
   TVEC_EXTERN_LOGICAL_OPERATOR(T, ==) \
   TVEC_EXTERN_LOGICAL_OPERATOR(T, !=) \
   TVEC_EXTERN_LOGICAL_OPERATOR(T, <=) \
   TVEC_EXTERN_LOGICAL_OPERATOR(T, >=) \
   TVEC_EXTERN_LOGICAL_OPERATOR(T, &&) \
   TVEC_EXTERN_LOGICAL_OPERATOR(T, ||)

#define TVEC_EXTERN_INTEGER_TEMPLATE(T) \
   extern template class RVec<T>;       \
   TVEC_EXTERN_UNARY_OPERATOR(T, +)     \
   TVEC_EXTERN_UNARY_OPERATOR(T, -)     \
   TVEC_EXTERN_UNARY_OPERATOR(T, ~)     \
   TVEC_EXTERN_UNARY_OPERATOR(T, !)     \
   TVEC_EXTERN_BINARY_OPERATOR(T, +)    \
   TVEC_EXTERN_BINARY_OPERATOR(T, -)    \
   TVEC_EXTERN_BINARY_OPERATOR(T, *)    \
   TVEC_EXTERN_BINARY_OPERATOR(T, /)    \
   TVEC_EXTERN_BINARY_OPERATOR(T, %)    \
   TVEC_EXTERN_BINARY_OPERATOR(T, &)    \
   TVEC_EXTERN_BINARY_OPERATOR(T, |)    \
   TVEC_EXTERN_BINARY_OPERATOR(T, ^)    \
   TVEC_EXTERN_ASSIGN_OPERATOR(T, +=)   \
   TVEC_EXTERN_ASSIGN_OPERATOR(T, -=)   \
   TVEC_EXTERN_ASSIGN_OPERATOR(T, *=)   \
   TVEC_EXTERN_ASSIGN_OPERATOR(T, /=)   \
   TVEC_EXTERN_ASSIGN_OPERATOR(T, %=)   \
   TVEC_EXTERN_ASSIGN_OPERATOR(T, &=)   \
   TVEC_EXTERN_ASSIGN_OPERATOR(T, |=)   \
   TVEC_EXTERN_ASSIGN_OPERATOR(T, ^=)   \
   TVEC_EXTERN_ASSIGN_OPERATOR(T, >>=)  \
   TVEC_EXTERN_ASSIGN_OPERATOR(T, <<=)  \
   TVEC_EXTERN_LOGICAL_OPERATOR(T, <)   \
   TVEC_EXTERN_LOGICAL_OPERATOR(T, >)   \
   TVEC_EXTERN_LOGICAL_OPERATOR(T, ==)  \
   TVEC_EXTERN_LOGICAL_OPERATOR(T, !=)  \
   TVEC_EXTERN_LOGICAL_OPERATOR(T, <=)  \
   TVEC_EXTERN_LOGICAL_OPERATOR(T, >=)  \
   TVEC_EXTERN_LOGICAL_OPERATOR(T, &&)  \
   TVEC_EXTERN_LOGICAL_OPERATOR(T, ||)

TVEC_EXTERN_INTEGER_TEMPLATE(char)
TVEC_EXTERN_INTEGER_TEMPLATE(short)
TVEC_EXTERN_INTEGER_TEMPLATE(int)
TVEC_EXTERN_INTEGER_TEMPLATE(long)
// TVEC_EXTERN_INTEGER_TEMPLATE(long long)

TVEC_EXTERN_INTEGER_TEMPLATE(unsigned char)
TVEC_EXTERN_INTEGER_TEMPLATE(unsigned short)
TVEC_EXTERN_INTEGER_TEMPLATE(unsigned int)
TVEC_EXTERN_INTEGER_TEMPLATE(unsigned long)
// TVEC_EXTERN_INTEGER_TEMPLATE(unsigned long long)

TVEC_EXTERN_FLOAT_TEMPLATE(float)
TVEC_EXTERN_FLOAT_TEMPLATE(double)

#undef TVEC_EXTERN_UNARY_OPERATOR
#undef TVEC_EXTERN_BINARY_OPERATOR
#undef TVEC_EXTERN_ASSIGN_OPERATOR
#undef TVEC_EXTERN_LOGICAL_OPERATOR
#undef TVEC_EXTERN_INTEGER_TEMPLATE
#undef TVEC_EXTERN_FLOAT_TEMPLATE

#define TVEC_EXTERN_UNARY_FUNCTION(T, NAME, FUNC) extern template RVec<PromoteType<T>> NAME(const RVec<T> &);

#define TVEC_EXTERN_STD_UNARY_FUNCTION(T, F) TVEC_EXTERN_UNARY_FUNCTION(T, F, std::F)

#define TVEC_EXTERN_BINARY_FUNCTION(T0, T1, NAME, FUNC)                           \
   extern template RVec<PromoteTypes<T0, T1>> NAME(const RVec<T0> &, const T1 &); \
   extern template RVec<PromoteTypes<T0, T1>> NAME(const T0 &, const RVec<T1> &); \
   extern template RVec<PromoteTypes<T0, T1>> NAME(const RVec<T0> &, const RVec<T1> &);

#define TVEC_EXTERN_STD_BINARY_FUNCTION(T, F) TVEC_EXTERN_BINARY_FUNCTION(T, T, F, std::F)

#define TVEC_EXTERN_STD_FUNCTIONS(T)             \
   TVEC_EXTERN_STD_UNARY_FUNCTION(T, abs)        \
   TVEC_EXTERN_STD_BINARY_FUNCTION(T, fdim)      \
   TVEC_EXTERN_STD_BINARY_FUNCTION(T, fmod)      \
   TVEC_EXTERN_STD_BINARY_FUNCTION(T, remainder) \
   TVEC_EXTERN_STD_UNARY_FUNCTION(T, exp)        \
   TVEC_EXTERN_STD_UNARY_FUNCTION(T, exp2)       \
   TVEC_EXTERN_STD_UNARY_FUNCTION(T, expm1)      \
   TVEC_EXTERN_STD_UNARY_FUNCTION(T, log)        \
   TVEC_EXTERN_STD_UNARY_FUNCTION(T, log10)      \
   TVEC_EXTERN_STD_UNARY_FUNCTION(T, log2)       \
   TVEC_EXTERN_STD_UNARY_FUNCTION(T, log1p)      \
   TVEC_EXTERN_STD_BINARY_FUNCTION(T, pow)       \
   TVEC_EXTERN_STD_UNARY_FUNCTION(T, sqrt)       \
   TVEC_EXTERN_STD_UNARY_FUNCTION(T, cbrt)       \
   TVEC_EXTERN_STD_BINARY_FUNCTION(T, hypot)     \
   TVEC_EXTERN_STD_UNARY_FUNCTION(T, sin)        \
   TVEC_EXTERN_STD_UNARY_FUNCTION(T, cos)        \
   TVEC_EXTERN_STD_UNARY_FUNCTION(T, tan)        \
   TVEC_EXTERN_STD_UNARY_FUNCTION(T, asin)       \
   TVEC_EXTERN_STD_UNARY_FUNCTION(T, acos)       \
   TVEC_EXTERN_STD_UNARY_FUNCTION(T, atan)       \
   TVEC_EXTERN_STD_BINARY_FUNCTION(T, atan2)     \
   TVEC_EXTERN_STD_UNARY_FUNCTION(T, sinh)       \
   TVEC_EXTERN_STD_UNARY_FUNCTION(T, cosh)       \
   TVEC_EXTERN_STD_UNARY_FUNCTION(T, tanh)       \
   TVEC_EXTERN_STD_UNARY_FUNCTION(T, asinh)      \
   TVEC_EXTERN_STD_UNARY_FUNCTION(T, acosh)      \
   TVEC_EXTERN_STD_UNARY_FUNCTION(T, atanh)      \
   TVEC_EXTERN_STD_UNARY_FUNCTION(T, floor)      \
   TVEC_EXTERN_STD_UNARY_FUNCTION(T, ceil)       \
   TVEC_EXTERN_STD_UNARY_FUNCTION(T, trunc)      \
   TVEC_EXTERN_STD_UNARY_FUNCTION(T, round)      \
   TVEC_EXTERN_STD_UNARY_FUNCTION(T, erf)        \
   TVEC_EXTERN_STD_UNARY_FUNCTION(T, erfc)       \
   TVEC_EXTERN_STD_UNARY_FUNCTION(T, lgamma)     \
   TVEC_EXTERN_STD_UNARY_FUNCTION(T, tgamma)

TVEC_EXTERN_STD_FUNCTIONS(float)
TVEC_EXTERN_STD_FUNCTIONS(double)
#undef TVEC_EXTERN_STD_UNARY_FUNCTION
#undef TVEC_EXTERN_STD_BINARY_FUNCTION
#undef TVEC_EXTERN_STD_UNARY_FUNCTIONS

#ifdef R__HAS_VDT

#define TVEC_EXTERN_VDT_UNARY_FUNCTION(T, F) TVEC_EXTERN_UNARY_FUNCTION(T, F, vdt::F)

TVEC_EXTERN_VDT_UNARY_FUNCTION(float, fast_expf)
TVEC_EXTERN_VDT_UNARY_FUNCTION(float, fast_logf)
TVEC_EXTERN_VDT_UNARY_FUNCTION(float, fast_sinf)
TVEC_EXTERN_VDT_UNARY_FUNCTION(float, fast_cosf)
TVEC_EXTERN_VDT_UNARY_FUNCTION(float, fast_tanf)
TVEC_EXTERN_VDT_UNARY_FUNCTION(float, fast_asinf)
TVEC_EXTERN_VDT_UNARY_FUNCTION(float, fast_acosf)
TVEC_EXTERN_VDT_UNARY_FUNCTION(float, fast_atanf)

TVEC_EXTERN_VDT_UNARY_FUNCTION(double, fast_exp)
TVEC_EXTERN_VDT_UNARY_FUNCTION(double, fast_log)
TVEC_EXTERN_VDT_UNARY_FUNCTION(double, fast_sin)
TVEC_EXTERN_VDT_UNARY_FUNCTION(double, fast_cos)
TVEC_EXTERN_VDT_UNARY_FUNCTION(double, fast_tan)
TVEC_EXTERN_VDT_UNARY_FUNCTION(double, fast_asin)
TVEC_EXTERN_VDT_UNARY_FUNCTION(double, fast_acos)
TVEC_EXTERN_VDT_UNARY_FUNCTION(double, fast_atan)

#endif // R__HAS_VDT

#endif // _VECOPS_USE_EXTERN_TEMPLATES

} // namespace VecOps

// Allow to use RVec as ROOT::RVec
using ROOT::VecOps::RVec;

} // namespace ROOT

#endif // ROOT_TVEC
