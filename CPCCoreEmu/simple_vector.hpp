#pragma once

#ifndef MINIMUM_DEPENDENCIES   
   #include <vector>
#else

namespace std
{
   template <typename T>
   class vector {
   public:
      
      typedef T* iterator;
      typedef const T* const_iterator;

      vector();
      vector(unsigned int size);
      virtual ~vector();

      void clear() noexcept;

      // element access
      T& operator [](unsigned int);
      const T& operator [](unsigned int) const;

      // Iterator
      iterator begin() noexcept;
      iterator end() noexcept;

   protected:
      unsigned int size_;
      unsigned int size_of_element_list_;
      T* element_list_;
   };

   template <typename T>
   vector<T>::vector() : size_(0), size_of_element_list_(0), element_list_(nullptr)
   {

   }

   template <typename T>
   vector<T>::vector(unsigned int size) : size_(0), size_of_element_list_(size), element_list_(nullptr)
   {
      element_list_ = new T[size];
   }

   template <typename T>
   vector<T>::~vector()
   {
      delete[]element_list_;
   }

   // element access
   template <typename T>
   T& vector<T>::operator [](unsigned int idx)
   {
      return element_list_[idx];
   }

   template <typename T>
   const T& vector<T>::operator [](unsigned int idx) const
   {
      return element_list_[idx];
   }

   template <typename T>
   void vector<T>::clear() noexcept {
      unsigned int i;
      for (i = 0; i < size_; ++i)
         element_list_[i].~T();
      size_ = 0;
   }

   // Iterator
   template <typename T>
   typename vector<T>::iterator vector<T>::begin() noexcept {
      return element_list_;
   }

   template <typename T>
   typename vector<T>::iterator vector<T>::end() noexcept {
      return element_list_ + size_;
   }
}


#endif