#pragma once

#ifndef MINIMUM_DEPENDENCIES   
   #include <vector>
#else

namespace std
{
   template <class _Ty>
   constexpr const _Ty& (min)(const _Ty& _Left,
      const _Ty& _Right) 
   { // return smaller of _Left and _Right
      if (_Right < _Left) 
      {
         return _Right;
      }
      return _Left;
   }

   template <class _Ty>
   constexpr const _Ty& (max)(const _Ty& _Left,
      const _Ty& _Right)
   { // return smaller of _Left and _Right
      if (_Right > _Left)
      {
         return _Right;
      }
      return _Left;
   }

   template <typename T>
   class vector {
   public:
      
      typedef T* iterator;
      typedef const T* const_iterator;

      vector();
      vector(unsigned int size);
      virtual ~vector();

      void clear() noexcept;

      void push_back(T&& _Val);
      void push_back(const T& _Val);

      // element access
      T& operator [](unsigned int);
      const T& operator [](unsigned int) const;

      T& back();
      T& at(unsigned int);

      // Iterator
      iterator begin() noexcept;
      iterator end() noexcept;
      unsigned int size() const noexcept 
      {
         return size_;
      }
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
      element_list_ = new T[size_];
   }

   template <typename T>
   vector<T>::~vector()
   {
      delete[]element_list_;
   }

   template <typename T>
   void vector<T>::push_back(T&& _Val)
   {
      if (size_ >= size_of_element_list_)
      {
         size_of_element_list_ *= 2;
         T* tmp = element_list_;
         element_list_ = (T*)new void* [ sizeof (T) * size_of_element_list_];
         memcpy(element_list_, tmp, size_of_element_list_ * sizeof(T));
         delete[] tmp;
      }
      element_list_[size_] = _Val;
      size_++;
      
   }

   template <typename T>
   void vector<T>::push_back(const T& _Val)
   {
      if (size_ >= size_of_element_list_)
      {
         size_of_element_list_ *= 2;
         T* tmp = element_list_;
         element_list_ = (T*)new void*[sizeof(T) * size_of_element_list_];
         memcpy(element_list_, tmp, size_of_element_list_ * sizeof(T));
         delete[] tmp;
      }
      element_list_[size_] = _Val;
      size_++;
   }

   template <typename T>
   T& vector<T>::back()
   {	
      return element_list_[size_-1];
   }

   template <typename T>
   T& vector<T>::at(unsigned int idx)
   {
      return element_list_[idx];
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