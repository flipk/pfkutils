/** \file thread2thread2.h"
 * \brief describes all classes for Thread2Thread v2.
 */

#ifndef __T2T2_HEADER_FILE__
#define __T2T2_HEADER_FILE__ 1

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <vector>
#include <list>
#include <atomic>
#include <type_traits>

#include "posix_fe.h"

/** Thread2Thread2 namespace, encapsulates all data structures for this API
 */
namespace Thread2Thread2 {

//////////////////////////// ERROR HANDLING ////////////////////////////

/** the list of error types passed to an
 * t2t2_assert_handler_t assertion handler.
 * \note the errors which are a result of a user error are listed first;
 *    errors which are most likely internal bugs are listed next. */
enum class t2t2_error_t {

    // please keep this in sync with t2t2_error_types[]

    NO_ERROR, // this is first, and internal.

    // the following errors are the result of user error.
    BUFFER_SIZE_TOO_BIG_FOR_POOL,   //!< buffer size too big
    DOUBLE_FREE,       //!< freeing a buffer already freed
    QUEUE_IN_A_SET,   //!< queue is currently in a set
    QUEUE_SET_EMPTY,    //!< queue set empty
    ENQUEUE_EMPTY_POINTER,    //!< enqueue empty pointer

    // the following errors are most likely internal bugs.
    LINKS_MAGIC_CORRUPT,        //!< (internal) magic sig corrupt
    LINKS_ADD_ALREADY_ON_LIST,   //!< (internal) already on a list
    LINKS_REMOVE_NOT_ON_LIST,    //!< (internal) not on any list
    POOL_RELEASE_ALREADY_ON_LIST, //!< (internal) already released
    QUEUE_DEQUEUE_NOT_ON_THIS_LIST, //!< (internal) not on this list
    QUEUE_ENQUEUE_ALREADY_ON_A_LIST, //!< (internal) already on a list

    NUM_ERRORS // must be last
};

/** this array contains descriptive strings for t2t2_error_t enum values;
 * as long as the value is < NUM_ERRORS, it is safe to use an enum
 * value to index this array. */
extern const char * t2t2_error_types[];

/** defines the signature of any user-supplied assertion handler
 * \note If your assert handler is passed fatal=true, your assert handler
 *   \em must not continue execution of this thread. To continue execution
 * is to invite SEGV or other errors. */
typedef void (*t2t2_assert_handler_t)(t2t2_error_t e, bool fatal,
                                     const char *filename,
                                     int lineno);

/** a global variable holds a pointer to an assertion handler function
 * \note If your assert handler is passed fatal=true, your assert handler
 *   \em must not continue execution of this thread. To continue execution
 * is to invite SEGV or other errors. */
extern t2t2_assert_handler_t t2t2_assert_handler;

//////////////////////////// T2T2_POOL_STATS ////////////////////////////

/** statistics for buffer pools */
struct t2t2_pool_stats {
    t2t2_pool_stats(int _buffer_size = 0);
    void init(int _buffer_size);

    int buffer_size;      //!< sizeof each buffer in the pool
    int total_buffers;    //!< current size of the pool
    int buffers_in_use;   //!< how many of those buffers in use
    int alloc_fails;      //!< how many times alloc/get returned null
    int grows;            //!< how many times pool has been grown
    int double_frees;     //!< how many times free buffer freed again
};

//////////////////////////// T2T2_WAIT_FLAG ////////////////////////////

/** wait interval values, for dequeuing and pool allocs; note GROW is
 * not relevant to dequeueing */
enum wait_flag
{
    /** -2 means grow pool if empty (not applicable to dequeue) */
    GROW = -2,
    WAIT_FOREVER = -1, //!< -1 means wait forever for a free buffer
    NO_WAIT = 0,       //!<  0 means return NULL immediately if empty
    ONE_SEC = 1000     //!< any value >0 is # of milliseconds to wait
};

#define __T2T2_INCLUDE_INTERNAL__ 1
#include "thread2thread2_internal.h"
#undef  __T2T2_INCLUDE_INTERNAL__

//////////////////////////// T2T2_POOL ////////////////////////////

/** template for a user's pool.
 * \param BaseT  the user's message class which is derived from
 *               t2t2_message_base.
 * \param derivedTs a list of user's message classes which are further
 *               derived from BaseT; can be an empty list (not present)
 *               if the user has no further derived messages from BaseT.
 * \note the size of the buffers in this pool will be dermined by the
 *    size of the largest class in BaseT and derivedTs list.
 */
template <class BaseT, class... derivedTs>
class t2t2_pool : public __t2t2_pool
{
public:
    // note largest_type<> also verifies all the derivedTs
    // are derived from BaseT.
    static const int buffer_size = largest_type<BaseT,derivedTs...>::size;
    /** \brief constructor for a pool.
     * \param _num_bufs_init  how many buffers to put in pool initially.
     * \param _bufs_to_add_when_growing  if you use get(wait==-2) and it
     *        grows the pool, how many buffers should it add at a time. */
    t2t2_pool(int _num_bufs_init = 0,
             int _bufs_to_add_when_growing = 1)
        : __t2t2_pool(buffer_size, _num_bufs_init,
                     _bufs_to_add_when_growing) { }
    virtual ~t2t2_pool(void) { }

    /** get a new message from the pool and specify how long to wait.
     * the refcount on the returned message will be 1.
     * \return  true if allocation succeeded, false, if pool empty.
     * \param ptr   pointer to new object returned here.
     * \param wait_ms  how long to wait, \ref wait_flag :
     *           <ul> <li> -2 = T2T2_GROW : grow pool if empty </li>
     *                <li> -1 = T2T2_WAIT_FOREVER : wait forever </li>
     *                <li> 0 = T2T2_NO_WAIT : don't wait at all, if the
     *                     pool is empty, return NULL
     *                     immediately. </li>
     *                <li> >0 : wait for some milliseconds for a buffer
     *                     to become available, then give up </li> </ul>
     * \param args  if class T has a constructor, you must pass the args
     *              to the constructor here.
     * \note the type of the first parameter may be either the BaseT
     *   type or one of the derivedTs used to declare this pool.  If
     *   the type T for the first arg is \em not derived from BaseT,
     *   this will throw a static_assert at compile time. Also, if the
     *   buffers in the pool are not big enough for type T, that will
     *   also throw a static_assert. (All derived types intended to be
     *   contained by this pool should be included in the derivedTs
     *   argument, to prevent that problem.) */
    template <class T, typename... ConstructorArgs>
    bool alloc(pxfe_shared_ptr<T> * ptr, int wait_ms,
               ConstructorArgs&&... args);

    __T2T2_EVIL_CONSTRUCTORS(t2t2_pool);
    __T2T2_EVIL_DEFAULT_CONSTRUCTOR(t2t2_pool);
};

//////////////////////////// T2T2_QUEUE ////////////////////////////

/** template for a FIFO queue of messages.
 * \param BaseT  the user's base message class */
template <class BaseT>
class t2t2_queue
{
    template <class queuesetBaseT> friend class t2t2_queue_set;
    __t2t2_queue q;
public:
    /** constructor for a queue.
     *  a queue has a linked list, a mutex to protect updates to the list,
     *  and a pthread condition for blocking to sleep if the queue is empty
     *  and the user wants to wait to dequeue.  */
    t2t2_queue(void);
    virtual ~t2t2_queue(void) { }

    /** enqueue a message into this queue; the message must come
     *  from a pool of the same type. note the queue is a FIFO.
     * \param msg  message to enqueue
     * \return true if success, false if not
     * \note   this does a take() on the shared_ptr, so the user's
     *     pxfe_shared_ptr is now empty.
     * \note   it is safe for multiple threads to enqueue messages
     *     to a single queue. that is an expected use case. */
    template <class T> bool enqueue(pxfe_shared_ptr<T> &msg);

    /** return true if this queue has no messages.
     * /note this is only advisory. it is subject to race conditions,
     *   i.e. a thread may enqueue a message to this queue one
     *   instruction after this function determine it is empty,
     *   so by the time it returns true, it is wrong.
     *   don't depend on it, it's useful only as a diagnostic.
     */
    bool empty(void);

    /** dequeue a message from this queue in FIFO order.
     * \param wait_ms  how long to wait: \ref wait_flag
     *           <ul> <li> -1 = T2T2_WAIT_FOREVER : wait forever </li>
     *                <li> 0 = T2T2_NO_WAIT : don't wait at all, if the
     *                     queue is empty, return NULL
     *                     immediately. </li>
     *                <li> >0 : wait for some milliseconds for a message
     *                     to arrive, then give up </li> </ul>
     * \note the dequeue method is safe to call from multiple threads
     *       on the same queue at the same time. thus you could
     *       implement "load balancing" by having multiple threads
     *       dequeue from the same queue to spread out requests to
     *       multiple cores. this is NOT true of t2t2_queue_set!
     * \note do not call this dequeue method if this
     *       queue has been added to a t2t2_queue_set. that will throw
     *       an assertion. */
    pxfe_shared_ptr<BaseT>  dequeue(int wait_ms);

    __T2T2_EVIL_CONSTRUCTORS(t2t2_queue<BaseT>);
    __T2T2_EVIL_NEW(t2t2_queue<BaseT>);
};

//////////////////////////// T2T2_QUEUE_SET ////////////////////////////

/** template for a set of queues.
 * \param BaseT the user's base message class that all member queues
 *              are based on.
 */
template <class BaseT>
class t2t2_queue_set
{
    __t2t2_queue_set  qs;
public:
    /** constructor, requires attributes for the mutex and condition
     * used when dequeuing from the set.
     * \note when a t2t2_queue has been added to this set, that queue
     *       uses the mutex and condition from this set, rather than
     *       their own (in other words, all queues which are added to
     *       this set will use the \em same mutex and condition, in
     *       order to close certain race conditions).
     * \note this class is not multi-thread safe, that is you should
     *       not allow one thread to do add/remove while another does
     *       dequeue. that would be very bad. */
    t2t2_queue_set(void);

    /** destructor will clean up the list of queues. */
    ~t2t2_queue_set(void);

    /** add a queue to this set. an identifier will be associated
     * with the queue; this identifier is returned in dequeue().
     * may be done at any time.
     * \param q the t2t2_queue to add to this set. the queue must be
     *           based on the same base message class as this set.
     * \param id the identifier for this queue. this identifier will
     *    be returned to the caller of dequeue(). this value will also
     *    serve as the priority of this queue relative to the other
     *    queues in this set. if all queues are empty at the entry to
     *    dequeue, this value will have no effect; dequeue() will wake
     *    up and return the first message enqueued to any of the
     *    queues in this set. \em however, if multiple queues have
     *    waiting messages, the queue with the lowest id is serviced
     *    first.
     * \return true if successfully added, false if not. most likely
     *    failure cause is the given set is already added to some other
     *    queue set.
     * \note a given queue can only be in zero or one sets, not more.
     *    if it is not a member of any set, you may use the queue's
     *    own dequeue() method; if a queue is a member of a set, that
     *    queue's dequeue() method must NOT be used.
     * \note this class is not multi-thread safe, that is you should
     *       not allow one thread to do add/remove while another does
     *       dequeue. that would be very bad. */
    bool add_queue(t2t2_queue<BaseT> *q, int id);

    /** remove a queue from this set. may be done at any time.
     * \note this class is not multi-thread safe, that is you should
     *       not allow one thread to do add/remove while another does
     *       dequeue. that would be very bad. */
    void remove_queue(t2t2_queue<BaseT> *q);

    /** monitor all queues added to this set, and dequeue a message
     * as soon as one becomes available in any queues in this set.
     * \param wait_ms  how long to wait: \ref wait_flag
     *           <ul> <li> -1 = T2T2_WAIT_FOREVER : wait forever </li>
     *                <li> 0 = T2T2_NO_WAIT : don't wait at all, if all the
     *                     queues are empty, return NULL
     *                     immediately. </li>
     *                <li> >0 : wait for some milliseconds for a message
     *                     to arrive, then give up </li> </ul>
     * \param id  a pointer to a user supplied variable to receive the
     *      identifier passed to add_queue(). this will inform the user
     *      which queue became active. if the user does not require this
     *      information, this argument may be omitted (default to NULL).
     * \note this class is not multi-thread safe, that is you should
     *       not allow one thread to do add/remove while another does
     *       dequeue. that would be very bad.
     * \note the dequeue method is NOT safe to call from multiple
     *       threads at the same time on the same set; multiple threads
     *       calling dequeue on different sets is fine.
     * \note it is NOT safe to call an individual queue's dequeue
     *       method if that queue has been added to a set. */
    pxfe_shared_ptr<BaseT> dequeue(int wait_ms, int *id = NULL);
};

//////////////////////////// T2T2_MESSAGE_BASE ////////////////////////////

/** base class for all T2T2 messages.
 * \param BaseT      the derived data type's base class name. */
template <class BaseT>
class t2t2_message_base : public pxfe_shared_ptr_base
{
public:
    // users don't delete; users should be using pxfe_shared_ptr.
    static void operator delete(void *ptr);
protected:
    t2t2_message_base(void) { }
    virtual ~t2t2_message_base(void) { }

private:
    __t2t2_pool * __pool;

    // this is a 'no-throw' version of operator new,
    // which returns NULL instead of throwing.
    // this is important, because it prevents calling
    // the constructor function on a NULL.
    static void * operator new(size_t wanted_sz,
                               __t2t2_pool *pool,
                               int wait_ms) throw ();
    // t2t2_pool.alloc is what invokes new().
    template <class poolBaseT,
              class... poolDerivedTs> friend class t2t2_pool;

    __T2T2_EVIL_CONSTRUCTORS(t2t2_message_base);
    __T2T2_EVIL_NEW(t2t2_message_base);
};

#define __T2T2_INCLUDE_INTERNAL__ 2
#include "thread2thread2_internal.h"
#undef  __T2T2_INCLUDE_INTERNAL__


}; // namespace Thread2Thread2

///////////////////////// STREAM OPS /////////////////////////

// this has to be outside the namespace
std::ostream &operator<<(std::ostream &strm,
                         const Thread2Thread2::t2t2_pool_stats &stats);

#endif /* __T2T2_HEADER_FILE__ */

/** \mainpage  Thread2Thread2: message pools and queues

\section Overview Overview

Classes of interest provided by this library:
 <ul>
 <li> \ref Thread2Thread2::t2t2_message_base
 <li> \ref pxfe_shared_ptr
 <li> \ref Thread2Thread2::wait_flag
 <li> \ref Thread2Thread2::t2t2_pool
    <ul>
    <li> \ref Thread2Thread2::t2t2_pool_stats
    </ul>
 <li> \ref Thread2Thread2::t2t2_queue
 <li> \ref Thread2Thread2::t2t2_queue_set
 <li> \ref Thread2Thread2::t2t2_assert_handler
   <ul>
   <li> \ref Thread2Thread2::t2t2_error_t
   </ul>
 </ul>

This API allows you to do the following:
  <ul>
  <li> Manage pools of buffers which are real-time to
       allocate from and release to.

     <ul>
     <li> These pools may contain a single message type, or the
          buffers may be automatically sized properly for the largest
          in a set of message types (as long as they're all derived
          classes from a common base message class).
     </ul>

  <li> Messages are true C++ objects, supporting constructors and
       virtual destructors and virtual methods supporting a heirarchy
       of message classes.

     <ul>
     <li> Messages are passed using a facility very similar to
          std::shared_ptr from the standard C++ library. The user is
          given a shared pointer during alloc, the user gives it back
          when it is enqueued to a pool, and the recipient gets it
          back when the queue is dequeued. The objects are truly
          reference-counted and can be copied and reassigned, and the
          reference counts are managed automatically.

     <li> The constructor is called at "alloc" time from the buffer
          pool, and the destructor is called automatically when the
          last "shared pointer" reference is destructed.
     </ul>

  <li> Message queues are declared using a base class, but may carry
       any message class derived from that base class. It is up to the
       user to figure out the type of the derived class when it
       arrives, but this is made easier with the automatic
       dynamic_cast facility built into the shared pointers.

     <ul>
     <li> The shared pointer type has a copy-constructor that
          automatically performs a dynamic_cast and sets the contained
          pointer if successful, or sets it to NULL if not.

     <li> The shared pointer also has an assignment operator which
          performs a dynamic_cast.

     <li> The derived type shared pointer counts as an extra reference
          against the underlying buffer, so there's no worry of losing
          references or a shared pointer living longer than the buffer
          it references.
     </ul>

  <li> Message queues may be serviced one at a time (i.e. if you have
       a 1-to-1 or 1-to-many mapping of queue-to-thread) or they may
       be serviced in a set (one thread managing many queues).

     <ul>
     <li> A queue may be dequeued by multiple threads at the same
          time, to implement a form of "load balancing" where a pool
          of worker threadss spread out among your CPU cores. Whatever
          thread is either currently idle or comes up to the dequeue
          method next will be the one to receive the next message.

     <li> Queues may be added to or removed from a set at any time.
          (A queue cannot be in a set if you want to dequeue from it
          individually.)

     <li> When queues are in a set, they are prioritized according to
          a user-specified identifier; in this way, high priority
          queues can be processed before low priority queues.

     </ul>

  </ul>

\section Rules Rules

   <ul>
   <li> All messages must derive from t2t2_message_base<> template. They
        are allowed to derive in multiple levels, but t2t2_message_base
        must be the root of the inheritence heirarchy.

   <li> The argument to the t2t2_message_base<> template must be the name
        of the class which is deriving from it; that is to say, all
        message classes deriving from t2t2_message_base<> must be of the
        form:
\code
class MY_MSG_BASE : public Thread2Thread2::t2t2_message_base<MY_MSG_BASE>
{
public:
   // recommend defining some "convenience" typedefs here, such as:
   typedef Thread2Thread2::t2t2_queue<MY_MSG_BASE> queue_t;
   typedef Thread2Thread2::t2t2_queue_set<MY_MSG_BASE> queue_set_t;
   typedef pxfe_shared_ptr<MY_MSG_BASE> sp_t;

   MY_MSG_BASE( <constructor args here> );
   MY_MSG_BASE( <optional constructor overloading is supported> );
   virtual ~MY_MSG_BASE(void); // optional destructor supported

   <fields and methods here>
};
\endcode

   <li> The user may optionally specialize further derived message
        types from MY_MSG_BASE, if desired.

\code
class MY_MSG_DERIVED_TYPE1 : public MY_MSG_BASE
{
public:
   // recommend defining some "convenience" typedefs here, such as:
   typedef pxfe_shared_ptr<MY_MSG_DERIVED_TYPE1> sp_t;
   // optionally define a pool which contains buffers big enough
   // only for this one message type.
   typedef Thread2Thread2::t2t2_pool<MY_MSG_BASE,
                                     MY_MSG_DERIVED_TYPE1> pool_t;

   MY_MSG_DERIVED_TYPE1( <constructor args> );
   virtual ~MY_MSG_DERIVED_TYPE1(void);

   <fields and methods here>
};

class MY_MSG_DERIVED_TYPE2 : public MY_MSG_BASE
{
   // recommend defining some "convenience" typedefs here, such as:
   typedef pxfe_shared_ptr<MY_MSG_DERIVED_TYPE2> sp_t;

   MY_MSG_DERIVED_TYPE2( <constructor args> );
   virtual ~MY_MSG_DERIVED_TYPE2(void);

   <fields and methods here>
};

// or, optionally define a single pool which contains buffers
// big enough for multiple message types -- just list the types
// after the base class type. The t2t2_pool template will figure
// out which derived type is the largest and define buffers that size.
typedef Thread2Thread2::t2t2_pool<MY_MSG_BASE,
                                  MY_MSG_DERIVED_TYPE1,
                                  MY_MSG_DERIVED_TYPE2> pool_1_and_2_t;
\endcode

   <li> A t2t2_pool<> must be declared using a class derived from
        t2t2_message_base<> as the first argument.  A t2t2_pool<> may also
        specify as additional arguments message classes derived from that
        first argument. The buffers in the pool will be sized according
        to the largest of all the specified data types. Any data type
        derived from that first argument can be allocated from this pool,
        as long as the buffers in the pool are large enough.

\code
Thread2Thread2::t2t2_pool<MY_MSG_BASE,
                         MY_MSG_DERIVED_TYPE1,
                         MY_MSG_DERIVED_TYPE2>   my_msg_pool(
                                  num_initial_buffers,
                                  buffers_to_add_during_grow,
                                  mutex_attrs, cond_attrs);

// this pool contains buffers sized by the largest derived type.
\endcode

   <li> A t2t2_queue<> must be declared using a class derived from
        t2t2_message_base<> (the same class used as the first arg to
        t2t2_pool<>).  That queue can carry any message class derived
        from that class, but the dequeue() method of the queue will
        return a shared pointer of the base class, and it is up to the
        user to use the casting constructor or casting assignment
        operator of pxfe_shared_ptr to get to the derived type.

\code
Thread2Thread2::t2t2_queue<MY_MSG_BASE>  my_msg_queue(
                                  mutex_attrs, cond_attrs);

// this queue can carry messages of any type derived from MY_MSG_BASE.
\endcode

   <li> All queues added to a t2t2_queue_set must be declared using the
        same message base class (a class derived from
        t2t2_message_base<>).

\code
Thread2Thread2::t2t2_queue<MY_MSG_BASE>       q1(mattr,cattr);
Thread2Thread2::t2t2_queue<MY_MSG_BASE>       q2(mattr,cattr);
Thread2Thread2::t2t2_queue<OTHER_MSG_BASE>    q3(mattr,cattr);

Thread2Thread2::t2t2_queue_set<MY_MSG_BASE>   qset(mattr,cattr);

qset.add_queue(&q1, 1); // ok
qset.add_queue(&q2, 2); // ok
// qset.add_queue(&q3, 3);  // error: cannot convert argument 1
\endcode

   </ul>

\section Example Example

Suppose you had three messages you wanted to pass in a system, a base
class messages and two derived classes. Define them as below, deriving
the first class from Thread2Thread2::t2t2_message_base.

\subsection msgclassdecls Message class declarations

\note I recommend declaring typedefs in your message classes to simplify
   declarations of shared pointers, queues, queue sets, and pools.

\code
namespace t2t2 = Thread2Thread2; // convenience alias
class my_message_base : public t2t2::t2t2_message_base<my_message_base>
{
public:
    // convenience
    typedef t2t2::t2t2_pool      <my_message_base> pool_t;
    typedef t2t2::t2t2_queue     <my_message_base> queue_t;
    typedef t2t2::t2t2_queue_set <my_message_base> queue_set_t;
    typedef pxfe_shared_ptr<my_message_base> sp_t;

    <your message contents here>

    my_message_base( <constructor arguments here> )
    {
        <constructor function body>
    }
    virtual ~my_message_base(void)
    {
        <destructor function body>
    }
    virtual void print(void) { <you may specify virtual functions!> }
};

class my_message_derived1 : public my_message_base
{
public:
    // convenience
    typedef t2t2::t2t2_pool<my_message_base,
                          my_message_derived1> pool1_t;
    typedef pxfe_shared_ptr<my_message_derived1> sp_t;

    <your message contents here>

    my_message_derived1( <constructor arguments here> )
    {
        <constructor function body>
    }
    virtual ~my_message_derived1(void)
    {
        <destructor function body>
    }
    virtual void print(void) { <you may override virtual functions!> }
};

class my_message_derived2 : public my_message_base
{
public:
    // convenience
    typedef t2t2::t2t2_pool<my_message_base,
                          my_message_derived2> pool2_t;
    typedef pxfe_shared_ptr<my_message_derived2> sp_t;

    <your message contents here>

    my_message_derived2( <constructor arguments here> )
    {
        <constructor function body>
    }
    virtual ~my_message_derived2(void)
    {
        <destructor function body>
    }
    virtual void print(void) { <you may override virtual functions!> }
};
\endcode

\note the arguments to the t2t2_pool template are a variable-length
   list of types. the first argument must be the base message type,
   but the remainder can be any collection of derived message types.
   what is important is that the largest message is specified, as
   the buffers in this pool will be sized to fit the largest message.

\subsection sharedptr Shared Pointers

Some of the functions below return a
\ref pxfe_shared_ptr
which automatically manages reference counts in the returned
message. The user may assign to another shared_ptr or keep
this shared pointer as long as wanted.  Just like std::shared_ptr,
when the last reference is lost, the message will be destructed,
except here, it will be returned to the pool.

Note that the typedefs added above make these easier to declare.
Next, use those convenience types to declare a pool which can contain
a set of buffers, and a queue for passing messages from one thread to
another.

\subsection pools Message Pools

Queues and pools use pthread mutex and condition, so you can supply
custom attributes for those if you need to change their properties.

\code
    pthread_mutexattr_t  mattr;
    pthread_condattr_t   cattr;
    pthread_mutexattr_init(&mattr);
    pthread_condattr_init(&cattr);
    pthread_condattr_setclock(&cattr, CLOCK_MONOTONIC);

    // this pool starts with 1 buffer large enough to contain
    // a base or a derived1; but each time you specify GROW to
    // the allocator and it needs to grow, it will add 10 buffers.
    my_message_derived1::pool1_t  mypool1(1,10,&mattr,&cattr);

    // this pool has buffers large enough for the derived2 class.
    my_message_derived2::pool2_t  mypool2(1,10,&mattr,&cattr);

    // declare two queues which can carry the my_message_base
    // type, or any type derived from it. we are declaring two
    // here just to demonstrate the t2t2_queue_set functionality.
    my_message_base    ::queue_t  myqueue1(    &mattr,&cattr);
    my_message_base    ::queue_t  myqueue2(    &mattr,&cattr);

    // declare a queue set; next we'll add queues to the set.
    my_message_base::queue_set_t      qset(    &mattr,&cattr);

    // add the two queues to the set, with unique id numbers
    // for each queue. note also the id also serves as a priority.
    // lower value ids will be serviced first.
    qset.add_queue(&myqueue1, 1);
    qset.add_queue(&myqueue2, 2);

    // no longer need the attribute objects.
    pthread_mutexattr_destroy(&mattr);
    pthread_condattr_destroy(&cattr);
\endcode

Now, a sender may allocate new buffers from the pool, and any arguments
required for the object constructors are passed through the arguments
to the \ref Thread2Thread2::t2t2_pool::alloc call.

\subsection enqueuing Enqueueing into Message Queues

Once it has allocated a buffer and filled it out, it enqueues it using
\ref Thread2Thread2::t2t2_queue::enqueue.

\code
    my_message_base::sp_t  spmb;
    if (mypool1.alloc(&spmb,
                     <time to wait>,
               <args to my_message_base constructor>))
    {
        printf("enqueuing a message NOW\n");
        myqueue1.enqueue(spmb);
    }
    else
        printf("ALLOC FAILED\n");
\endcode

\note the "sp_t" type is a shortcut typedef for pxfe_shared_ptr defined
   above.  also note the enqueue "takes" the value out of the
   pxfe_shared_ptr, so the "spmb" is now empty (NULL).

\note we are allocating a "my_message_base" from the pool declared
   under "my_message_derived1". that pool has buffers big enough for
   the derived1 type (which are bigger than the base class) but that's
   ok -- the alloc method will check that the buffers in the specified
   pool are big enough for the type being requested.

A sender may also allocate a derived message type too, using the same
syntax. Note that the pool should have documented in its template
argument list every derived type that will be allocated from it (just
to ensure the buffers in that pool are big enough for every type that
will be allocated from it). If the buffers are not big enough for the
requested type, you will get static_assert compile time errors telling
you such.

Also note, this example shows you how to specify that the pool should
grow if it is currently empty.

\code
    my_message_derived1::sp_t  spmd1;
    if (mypool1.alloc(&spmd1,
                     t2t2::GROW,
                 <args to my_message_derived1 constructor> ))
    {
        printf("enqueuing a message NOW\n");
        myqueue2.enqueue(spmd1);
    }
    else
        printf("ALLOC FAILED\n");
\endcode

\note In this example we use a t2t2_queue_set to group queues together
   and service them together (the set initialization was shown above);
   however, if you do not add a queue to a set, you may use the
   "dequeue" method in a single queue's class to process messages.

\subsection dequeuing Dequeuing from Message Queues

The recipient thread then dequeues from these queues using
\ref Thread2Thread2::t2t2_queue_set::dequeue
to process the messages in its own time.

\ref pxfe_shared_ptr has a helpful facility for
converting between message types. It has a copy-constructor which
invokes dynamic_cast and attempts to convert from the base class to
the derived class, and it also has a casting assignment operator which
does the same. Both are demonstrated in the code below.

\code
    while (keep_going)
    {
        my_message_base::sp_t x;
        int qid = -1;
        // wait for 250 milliseconds for a message.
        x = qset.dequeue(250, &qid);
        if (x) // checks for NULL pointer
        {
            printf("READER GOT MSG on queue %d:\n", qid);
            x->print(); // virtual method in base class.

            my_message_derived1::sp_t  y;
            // this one tests the casting operator=
            if (y = x) // assign and then check for !null
            {
                 // access y->fieldname from derived1 fields
            }

            // this one tests the casting constructor
            my_message_derived2::sp_t  z = x;
            if (z) // checking for !null
            {
                 // access z->fieldname from derived2 fields
            }
        }
        else
            printf("READER GOT NULL\n");
    }
\endcode

\section errorhandling Error Handling / Assertions

Finally, by default errors are caught and printed on stderr; fatal
errors cause an exit of the process. If you want your own handler for
Thread2Thread2 errors, you can register a function to handle them
using the global variable \ref Thread2Thread2::t2t2_assert_handler.

You can also access printable string descriptions of the error enums
via the global array \ref Thread2Thread2::t2t2_error_types, as long
as the enum value is less than NUM_ERRORS.

\note If your assert handler is passed fatal=true, your assert handler
   \em must not continue execution of this thread. To continue execution
   is to invite SEGV or other errors.

\code

static void
my_t2t2_assert_handler(t2t2::t2t2_error_t e, bool fatal,
                      const char *filename, int lineno)
{
    fprintf(stderr,
            "\n\nERROR: Thread2Thread2 ASSERTION %d (%s) at %s:%d\n\n",
            e, t2t2::t2t2_error_types[e], filename, lineno);

    // i want a core dump that i can gdb
    kill(0, SIGABRT);
}

void register_new_assertion_handler(void)
{
   Thread2Thread2::t2t2_assert_handler = &my_t2t2_assert_handler;
}
\endcode

\section moreexample More Examples

See the page called \ref examplecode for full source code to
an example program.

 */

/** \page examplecode Example test program

Here is the complete source code to the included test program.

\include thread2thread2_test.cc

*/
