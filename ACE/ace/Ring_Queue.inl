// -*- C++ -*-
ACE_BEGIN_VERSIONED_NAMESPACE_DECL

template <class T> ACE_INLINE size_t
ACE_Ring_Queue<T>::size () const
{
  return this->cur_size_;
}

template <class T> ACE_INLINE bool
ACE_Ring_Queue<T>::is_empty () const
{
  //  ACE_TRACE ("ACE_Ring_Queue<T>::is_empty");
  return this->empty_;
}

template <class T> ACE_INLINE bool
ACE_Ring_Queue<T>::is_full () const
{
  //  ACE_TRACE ("ACE_Ring_Queue<T>::is_full");
  return !this->empty_ && this->front_ == this->rear_; 
}

ACE_END_VERSIONED_NAMESPACE_DECL
