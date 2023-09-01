#ifndef ACE_RING_QUEUE_CPP
#define ACE_RING_QUEUE_CPP

#include "ace/Ring_Queue.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#if !defined (__ACE_INLINE__)
#include "ace/Ring_Queue.inl"
#endif /* __ACE_INLINE__ */

#include "ace/Malloc_Base.h"
#include "ace/Log_Category.h"
#include "ace/os_include/os_errno.h"

ACE_BEGIN_VERSIONED_NAMESPACE_DECL

ACE_ALLOC_HOOK_DEFINE_Tc(ACE_Ring_Queue)

template <class T>
ACE_Ring_Queue<T>::ACE_Ring_Queue (size_t size, ACE_Allocator *alloc)
  : size_ (size),
    cur_size_ (0),
    front_(0),
    rear_(0),
    empty_(true),
    array_(size, alloc)
{
  
}

template <class T>
ACE_Ring_Queue<T>::ACE_Ring_Queue (const ACE_Ring_Queue<T> &rs)
   : size_ (rs.size_),
     cur_size_ (rs.cur_size_),
     front_(rs.front_),
     rear_(rs.rear_),
     empty_(rs.empty_),
     array_(rs.array_)
{

}

template <class T> void
ACE_Ring_Queue<T>::operator= (const ACE_Ring_Queue<T> &rs)
{
  //   ACE_TRACE ("ACE_Ring_Queue<T>::operator=");

  if (this != &us)
    {
      this->size_     = rs.size_;
      this->cur_size_ = rs.cur_size_;
      this->front_    = rs.front_;
      this->rear_     = rs.rear_;
      this->array_    = rs.array_;
    }
}

template <class T> ACE_Ring_Queue_Iterator<T>
ACE_Ring_Queue<T>::begin ()
{
  // ACE_TRACE ("ACE_Ring_Queue<T>::begin");
  return ACE_Ring_Queue_Iterator<T> (*this);
}

template <class T> ACE_Ring_Queue_Iterator<T>
ACE_Ring_Queue<T>::end ()
{
  // ACE_TRACE ("ACE_Ring_Queue<T>::end");
  return ACE_Ring_Queue_Iterator<T> (*this, 1);
}

template <class T> void
ACE_Ring_Queue<T>::dump () const
{
#if defined (ACE_HAS_DUMP)
  //   ACE_TRACE ("ACE_Ring_Queue<T>::dump");

  ACELIB_DEBUG ((LM_DEBUG, ACE_BEGIN_DUMP, this));
  ACELIB_DEBUG ((LM_DEBUG,  ACE_TEXT ("\nhead_ = %u"), this->head_));
  ACELIB_DEBUG ((LM_DEBUG,  ACE_TEXT ("\nhead_->next_ = %u"), this->head_->next_));
  ACELIB_DEBUG ((LM_DEBUG,  ACE_TEXT ("\ncur_size_ = %d\n"), this->cur_size_));

  T *item = 0;
#if !defined (ACE_NLOGGING)
  size_t count = 1;
#endif /* ! ACE_NLOGGING */

  for (ACE_Ring_Queue_Iterator<T> iter (*(ACE_Ring_Queue<T> *) this);
       iter.next (item) != 0;
       iter.advance ())
    ACELIB_DEBUG ((LM_DEBUG, ACE_TEXT ("count = %d\n"), count++));

  ACELIB_DEBUG ((LM_DEBUG, ACE_END_DUMP));
#endif /* ACE_HAS_DUMP */
}

template <class T> void
ACE_Ring_Queue<T>::copy_nodes (const ACE_Ring_Queue<T> &us)
{
  for (ACE_Node<T> *curr = us.head_->next_;
       curr != us.head_;
       curr = curr->next_)
    if (this->enqueue_tail (curr->item_) == -1)
      // @@ What's the right thing to do here?
      this->delete_nodes ();
}

template <class T> int
ACE_Ring_Queue<T>::enqueue_head (const T &new_item)
{
  //   ACE_TRACE ("ACE_Ring_Queue<T>::enqueue_head");

  ACE_Node<T> *temp = 0;

  // Create a new node that points to the original head.
  ACE_NEW_MALLOC_RETURN (temp,
                         static_cast<ACE_Node<T> *> (this->allocator_->malloc (sizeof (ACE_Node<T>))),
                         ACE_Node<T> (new_item, this->head_->next_),
                         -1);
  // Link this pointer into the front of the list.  Note that the
  // "real" head of the queue is <head_->next_>, whereas <head_> is
  // just a pointer to the dummy node.
  this->head_->next_ = temp;

  ++this->cur_size_;
  return 0;
}

template <class T> int
ACE_Ring_Queue<T>::enqueue_tail (const T &new_item)
{
  //   ACE_TRACE ("ACE_Ring_Queue<T>::enqueue_tail");

  // Insert <item> into the old dummy node location.  Note that this
  // isn't actually the "head" item in the queue, it's a dummy node at
  // the "tail" of the queue...
  this->head_->item_ = new_item;

  ACE_Node<T> *temp = 0;

  // Create a new dummy node.
  ACE_NEW_MALLOC_RETURN (temp,
                         static_cast<ACE_Node<T> *> (this->allocator_->malloc (sizeof (ACE_Node<T>))),
                         ACE_Node<T> (this->head_->next_),
                         -1);
  // Link this dummy pointer into the list.
  this->head_->next_ = temp;

  // Point the head to the new dummy node.
  this->head_ = temp;

  ++this->cur_size_;
  return 0;
}

template <class T> int
ACE_Ring_Queue<T>::dequeue_head (T &item)
{
  //   ACE_TRACE ("ACE_Ring_Queue<T>::dequeue_head");

  // Check for empty queue.
  if (this->is_empty ())
    return -1;

  ACE_Node<T> *temp = this->head_->next_;

  item = temp->item_;
  this->head_->next_ = temp->next_;
  ACE_DES_FREE_TEMPLATE (temp,
                         this->allocator_->free,
                         ACE_Node,
                         <T>);
  --this->cur_size_;
  return 0;
}

template <class T> void
ACE_Ring_Queue<T>::reset ()
{
  ACE_TRACE ("reset");
  this->size_     = 0;
  this->cur_size_ = 0;
  this->front_    = 0;
  this->rear_     = 0;
}

template <class T> int
ACE_Ring_Queue<T>::get (T *&item, size_t slot) const
{
  //   ACE_TRACE ("ACE_Ring_Queue<T>::get");

  ACE_Node<T> *curr = this->head_->next_;

  size_t i;

  for (i = 0; i < this->cur_size_; i++)
    {
      if (i == slot)
        break;

      curr = curr->next_;
    }

  if (i < this->cur_size_)
    {
      item = &curr->item_;
      return 0;
    }
  else
    return -1;
}

template <class T> int
ACE_Ring_Queue<T>::set (const T &item,
                             size_t slot)
{
  //   ACE_TRACE ("ACE_Ring_Queue<T>::set");

  ACE_Node<T> *curr = this->head_->next_;

  size_t i;

  for (i = 0;
       i < slot && i < this->cur_size_;
       ++i)
    curr = curr->next_;

  if (i < this->cur_size_)
    {
      // We're in range, so everything's cool.
      curr->item_ = item;
      return 0;
    }
  else
    {
      // We need to expand the list.

      // A common case will be increasing the set size by 1.
      // Therefore, we'll optimize for this case.
      if (i == slot)
        {
          // Try to expand the size of the set by 1.
          if (this->enqueue_tail (item) == -1)
            return -1;
          else
            return 0;
        }
      else
        {
          T const dummy = T ();

          // We need to expand the list by multiple (dummy) items.
          for (; i < slot; ++i)
            {
              // This head points to the existing dummy node, which is
              // about to be overwritten when we add the new dummy
              // node.
              curr = this->head_;

              // Try to expand the size of the set by 1, but don't
              // store anything in the dummy node (yet).
              if (this->enqueue_tail (dummy) == -1)
                return -1;
            }

          curr->item_ = item;
          return 0;
        }
    }
}

// ****************************************************************

template <class T> void
ACE_Ring_Queue_Const_Iterator<T>::dump () const
{
#if defined (ACE_HAS_DUMP)
  // ACE_TRACE ("ACE_Ring_Queue_Const_Iterator<T>::dump");
#endif /* ACE_HAS_DUMP */
}

template <class T>
ACE_Ring_Queue_Const_Iterator<T>::ACE_Ring_Queue_Const_Iterator (const ACE_Ring_Queue<T> &q, int end)
  : current_ (end == 0 ? q.head_->next_ : q.head_ ),
    queue_ (q)
{
  // ACE_TRACE ("ACE_Ring_Queue_Const_Iterator<T>::ACE_Ring_Queue_Const_Iterator");
}

template <class T> int
ACE_Ring_Queue_Const_Iterator<T>::advance ()
{
  // ACE_TRACE ("ACE_Ring_Queue_Const_Iterator<T>::advance");
  this->current_ = this->current_->next_;
  return this->current_ != this->queue_.head_;
}

template <class T> int
ACE_Ring_Queue_Const_Iterator<T>::first ()
{
  // ACE_TRACE ("ACE_Ring_Queue_Const_Iterator<T>::first");
  this->current_ = this->queue_.head_->next_;
  return this->current_ != this->queue_.head_;
}

template <class T> int
ACE_Ring_Queue_Const_Iterator<T>::done () const
{
  ACE_TRACE ("ACE_Ring_Queue_Const_Iterator<T>::done");

  return this->current_ == this->queue_.head_;
}

template <class T> int
ACE_Ring_Queue_Const_Iterator<T>::next (T *&item)
{
  // ACE_TRACE ("ACE_Ring_Queue_Const_Iterator<T>::next");
  if (this->current_ == this->queue_.head_)
    return 0;
  else
    {
      item = &this->current_->item_;
      return 1;
    }
}

// ****************************************************************

template <class T> void
ACE_Ring_Queue_Iterator<T>::dump () const
{
#if defined (ACE_HAS_DUMP)
  // ACE_TRACE ("ACE_Ring_Queue_Iterator<T>::dump");
#endif /* ACE_HAS_DUMP */
}

template <class T>
ACE_Ring_Queue_Iterator<T>::ACE_Ring_Queue_Iterator (ACE_Ring_Queue<T> &q, int end)
  : current_ (end == 0 ? q.head_->next_ : q.head_ ),
    queue_ (q)
{
  // ACE_TRACE ("ACE_Ring_Queue_Iterator<T>::ACE_Ring_Queue_Iterator");
}

template <class T> int
ACE_Ring_Queue_Iterator<T>::advance ()
{
  // ACE_TRACE ("ACE_Ring_Queue_Iterator<T>::advance");
  this->current_ = this->current_->next_;
  return this->current_ != this->queue_.head_;
}

template <class T> int
ACE_Ring_Queue_Iterator<T>::first ()
{
  // ACE_TRACE ("ACE_Ring_Queue_Iterator<T>::first");
  this->current_ = this->queue_.head_->next_;
  return this->current_ != this->queue_.head_;
}

template <class T> int
ACE_Ring_Queue_Iterator<T>::done () const
{
  ACE_TRACE ("ACE_Ring_Queue_Iterator<T>::done");

  return this->current_ == this->queue_.head_;
}

template <class T> int
ACE_Ring_Queue_Iterator<T>::next (T *&item)
{
  // ACE_TRACE ("ACE_Ring_Queue_Iterator<T>::next");
  if (this->current_ == this->queue_.head_)
    return 0;
  else
    {
      item = &this->current_->item_;
      return 1;
    }
}

ACE_END_VERSIONED_NAMESPACE_DECL

#endif /* ACE_RING_QUEUE_CPP */
