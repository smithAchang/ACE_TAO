// -*- C++ -*-

//=============================================================================
/**
 *  @file Ring_Queue.h
 *
 *  @author Smith.Achang <changyunlei@126.com>
 */
//=============================================================================

#ifndef ACE_RING_QUEUE_H
#define ACE_RING_QUEUE_H
#include /**/ "ace/pre.h"


#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/os_include/os_stddef.h"

ACE_BEGIN_VERSIONED_NAMESPACE_DECL

class ACE_Allocator;

template <class T>
class ACE_Ring_Queue;

/**
 * @class ACE_Ring_Queue_Iterator
 *
 * @brief Implement an iterator over an ring queue.
 */
template <class T>
class ACE_Ring_Queue_Iterator
{
public:
  ACE_Ring_Queue_Iterator (ACE_Ring_Queue<T> &q, int end = 0);

  // = Iteration methods.

  /// Pass back the @a next_item that hasn't been seen in the queue.
  /// Returns 0 when all items have been seen, else 1.
  int next (T *&next_item);

  /// Move forward by one element in the set.  Returns 0 when all the
  /// items in the queue have been seen, else 1.
  int advance ();

  /// Move to the first element in the queue.  Returns 0 if the
  /// queue is empty, else 1.
  int first ();

  /// Returns 1 when all items have been seen, else 0.
  int done () const;

  /// Dump the state of an object.
  void dump () const;

  /// Declare the dynamic allocation hooks.
  ACE_ALLOC_HOOK_DECLARE;

private:
  /// Pointer to the current node in the iteration.
  ACE_Node<T> *current_;

  /// Pointer to the queue we're iterating over.
  ACE_Ring_Queue<T> &queue_;
};

/**
 * @class ACE_Ring_Queue_Const_Iterator
 *
 * @brief Implement an iterator over an const ring queue.
 */
template <class T>
class ACE_Ring_Queue_Const_Iterator
{
public:
  ACE_Ring_Queue_Const_Iterator (const ACE_Ring_Queue<T> &q, int end = 0);

  // = Iteration methods.

  /// Pass back the @a next_item that hasn't been seen in the queue.
  /// Returns 0 when all items have been seen, else 1.
  int next (T *&next_item);

  /// Move forward by one element in the set.  Returns 0 when all the
  /// items in the queue have been seen, else 1.
  int advance ();

  /// Move to the first element in the queue.  Returns 0 if the
  /// queue is empty, else 1.
  int first ();

  /// Returns 1 when all items have been seen, else 0.
  int done () const;

  /// Dump the state of an object.
  void dump () const;

  /// Declare the dynamic allocation hooks.
  ACE_ALLOC_HOOK_DECLARE;

private:
  /// Pointer to the current node in the iteration.
  ACE_Node<T> *current_;

  /// Pointer to the queue we're iterating over.
  const ACE_Ring_Queue<T> &queue_;
};

/**
 * @class ACE_Ring_Queue
 *
 * @brief A Queue of "fixed" length.
 *
 * This implementation of an ring queue
 *
 * <b> Requirements and Performance Characteristics</b>
 *   - Internal Structure
 *      An array formed as ring
 *   - Duplicates allowed?
 *       Yes
 *   - Random access allowed?
 *       No
 *   - Search speed
 *       N/A
 *   - Insert/replace speed
 *       N/A
 *   - Iterator still valid after change to container?
 *       Yes
 *   - Frees memory for removed elements?
 *       Yes
 *   - Items inserted by
 *       Value
 *   - Requirements for contained type
 *       -# Default constructor
 *       -# Copy constructor
 *       -# operator=
 */
template <class T>
class ACE_Ring_Queue
{
public:
  friend class ACE_Ring_Queue_Iterator<T>;
  friend class ACE_Ring_Queue_Const_Iterator<T>;

  // Trait definition.
  typedef ACE_Ring_Queue_Iterator<T> ITERATOR;
  typedef ACE_Ring_Queue_Const_Iterator<T> CONST_ITERATOR;

  /// Construction.  Use user specified allocation strategy
  /// if specified.
  /**
   * Initialize an empty queue using the strategy provided.
   */
  ACE_Ring_Queue (size_t size, ACE_Allocator *alloc = nullptr);

  /// Copy constructor.
  /**
   * Initialize the queue to be a copy of the provided queue.
   */
  ACE_Ring_Queue (const ACE_Ring_Queue<T> &);

  /// Assignment operator.
  /**
   * Perform a deep copy of rhs.
   */
  void operator= (const ACE_Ring_Queue<T> &);

  /// Destructor.
  /**
   * Clean up the memory for the queue.
   */
  ~ACE_Ring_Queue ();

  // = Check boundary conditions.

  /// Returns true if the container is empty, otherwise returns false.
  /**
   * Constant time check to see if the queue is empty.
   */
  bool is_empty () const;

  /// Returns 0.
  /**
   * The queue cannot be full, so it always returns 0.
   */
  bool is_full () const;

  // = Classic queue operations.

  /// Adds @a new_item to the tail of the queue.  Returns 0 on success,
  /// -1 on failure.
  /**
   * Insert an item at the end of the queue.
   */
  int enqueue_tail (const T &new_item);

  /// Adds @a new_item to the head of the queue.  Returns 0 on success,
  /// -1 on failure.
  /**
   * Insert an item at the head of the queue.
   */
  int enqueue_head (const T &new_item);

  /// Removes and returns the first @a item on the queue.  Returns 0 on
  /// success, -1 if the queue was empty.
  /**
   * Remove an item from the head of the queue.
   */
  int dequeue_head (T &item);

  // = Additional utility methods.

  /// Reset the ACE_Ring_Queue to be empty and release all its
  /// dynamically allocated resources.
  /**
   * Delete the queue nodes.
   */
  void reset ();

  /// Get the @a slot th element in the set.  Returns -1 if the element
  /// isn't in the range {0..#cur_size_ - 1}, else 0.
  /**
   * Find the item in the queue between 0 and the provided index of the
   * queue.
   */
  int get (T *&item, size_t slot = 0) const;

  /// Set the @a slot th element of the queue to @a item.
  /**
   * Set the @a slot th element in the set.  Will pad out the set with
   * empty nodes if @a slot is beyond the range {0..#cur_size_ - 1}.
   * Returns -1 on failure, 0 if @a slot isn't initially in range, and
   * 0 otherwise.
   */
  int set (const T &item, size_t slot);

  /// The number of items in the queue.
  /**
   * Return the size of the queue.
   */
  size_t size () const;

  /// Dump the state of an object.
  void dump () const;

  // = STL-styled unidirectional iterator factory.
  ACE_Ring_Queue_Iterator<T> begin ();
  ACE_Ring_Queue_Iterator<T> end ();

  /// Declare the dynamic allocation hooks.
  ACE_ALLOC_HOOK_DECLARE;

protected:
  /// Current size of the queue.
  size_t size_;
  size_t cur_size_;
  size_t front_, rear_;
  bool empty_;
  
  /// Pointer to the dummy node in the circular linked Queue.
  ACE_Array<T> array_;
  

};

ACE_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "ace/Ring_Queue.inl"
#endif /* __ACE_INLINE__ */

#include "ace/Ring_Queue.cpp"

#include /**/ "ace/post.h"
#endif /* ACE_RING_QUEUE_H */
