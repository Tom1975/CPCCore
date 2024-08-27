#pragma once

#if defined (MINIMUM_DEPENDENCIES) ||defined (TEST_VECTOR)

#pragma pack(push, 1)

namespace std
{
#ifdef  __circle__
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
#endif

   template <typename T>
   class vector {
   public:
      
      typedef T* iterator;
      typedef const T* const_iterator;

      vector() noexcept;
      vector(unsigned int size);
      //vector(const T&);
      vector(const vector& v);
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

      vector& operator=(vector&& _Right)
      {
         if (&_Right != this)
         {
            // clear
            clear();
            if ( size_of_element_list_ < _Right.size_of_element_list_)
            {
               delete []element_list_;
               size_of_element_list_ = _Right.size_of_element_list_;
               element_list_ = new T[size_of_element_list_]();
            }
            
            size_ = _Right.size_;
            //memcpy ( element_list_, _Right.element_list_, sizeof(T)*size_);
            for (unsigned int idx = 0; idx < size_; idx++)
            {
               element_list_[idx] = _Right.element_list_[idx];
            }

         }
         return *this;
      }
   
      vector& operator=(const vector& _Right)
      {
         if (&_Right != this)
         {
            // clear
            clear();
            if ( size_of_element_list_ < _Right.size_of_element_list_)
            {
               delete []element_list_;
               size_of_element_list_ = _Right.size_of_element_list_;
               element_list_ = new T[size_of_element_list_];
            }
            
            size_ = _Right.size_;
            //memcpy ( element_list_, _Right.element_list_, sizeof(T)*size_);
            for (unsigned int idx = 0; idx < size_; idx++)
            {
               element_list_[idx] = _Right.element_list_[idx];
            }

         }
         return *this;
      }   

   protected:
      unsigned int size_;
      unsigned int size_of_element_list_;
      T* element_list_;
   };

   template <typename T>
   vector<T>::vector() noexcept  : size_(0), size_of_element_list_(0), element_list_(nullptr)
   {

   }

   template <typename T>
   vector<T>::vector(unsigned int size) : size_(0), size_of_element_list_(size), element_list_(nullptr)
   {
      element_list_ = new T[size_of_element_list_]();
   }

   /*template <typename T>
   vector<T>::vector(const T& v) : size_(0), size_of_element_list_(size), element_list_(nullptr)
   {
      *this = v;
   }*/
  
   template <typename T>
   vector<T>::vector(const  vector& v)
   {
      *this = v;
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
         size_of_element_list_ = (size_of_element_list_+1)*2;
         T* tmp = element_list_;
         element_list_ = new T[ size_of_element_list_]();
         for (unsigned int idx = 0; idx < size_; idx++)
         {
            element_list_[idx] = tmp[idx];
         }
         //memcpy(element_list_, tmp, size_ * sizeof(T));
         delete[] tmp;
      }
      element_list_[size_]= _Val;
      size_++;
      
   }

   template <typename T>
   void vector<T>::push_back(const T& _Val)
   {
      if (size_ >= size_of_element_list_)
      {
         size_of_element_list_ = (size_of_element_list_+1)*2;
         T* tmp = element_list_;
         element_list_ = new T[ size_of_element_list_]();
         for (unsigned int idx = 0; idx < size_; idx++)
         {
            element_list_[idx] = tmp[idx];
         }
         //memcpy(element_list_, tmp, size_ * sizeof(T));
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
#pragma pack(pop)
#else
   #include <vector>
#endif