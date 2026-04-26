// RayApi.h

#ifndef INCLUDED_RAYAPI_H
#define INCLUDED_RAYAPI_H

// #define RAY_BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))
#define RAY_BUILD_BUG_ON(condition) extern char (*RAY_Bug(void)) [sizeof(char[1-2*!!(condition)])]

// <1> linux common
// include/asm-generic/param.h (HZ)
// include/linux/jiffies.h

extern int RAY_printk( const char * fmt, ... );
extern char *RAY_strrchr(const char *str, char c);
extern unsigned long RAY_jiffies(void);
extern unsigned long RAY_msecs_to_jiffies( unsigned int m );
extern unsigned long RAY_usecs_to_jiffies( unsigned int u );
extern unsigned int RAY_jiffies_to_msecs( const unsigned long j );
extern unsigned int RAY_jiffies_to_usecs( const unsigned long j );
extern int RAY_HZ(void);
extern int RAY_PAGE_ALIGN( int size );

// <2> memory

//内存分配标志
#define RAY_GFP_KERNEL	1
#define RAY_GFP_NOIO	2
#define	RAY_GFP_ATOMIC	3
#define RAY_GFP_NOWARN	4

#define RAY_PAGE_SIZE	1

extern void * RAY_kmalloc( int size, int flag );
extern void * RAY_kzalloc(int size, int gfp_flags);
extern void RAY_kfree( void * p );

extern void * RAY_vmalloc_user( int size );
extern void * RAY_vmalloc_32(int size);
extern void RAY_vfree( void * buf );

extern void * RAY_memcpy( void * dst, const void * src, int size );
extern void * RAY_memset( void * dst, int ch, int size );
extern void RAY_strlcpy( void * dst, const void * src, int size );

extern void * RAY_vmalloc_to_page(unsigned long addr);
extern int RAY_vm_insert_page(void* vma, unsigned long addr, void* page);
extern unsigned int RAY_pageSizeGet(unsigned int pageSize);

extern int RAY_copy_to_user( void * dst, const void * src, int size );
extern int RAY_copy_from_user( void * dst, const void * src, int size );

extern int RAY_remap_vmalloc_range( void * vma, void * addr, unsigned long pgoff);

// <3> lock : 
// include/linux/spinlock.h

typedef struct RAY_spinlock_st
{
	char data[80];
} RAY_spinlock_t;

extern void RAY_spin_lock_init( RAY_spinlock_t * splock );
extern void RAY_spin_lock( RAY_spinlock_t * splock );
extern void RAY_spin_unlock( RAY_spinlock_t * splock );
extern void RAY_spin_lock_bh( RAY_spinlock_t * splock );
extern void RAY_spin_unlock_bh( RAY_spinlock_t * splock );
extern void RAY_spin_lock_irqsave(RAY_spinlock_t * splock, unsigned long flags);
extern void RAY_spin_unlock_irqrestore(RAY_spinlock_t * splock, unsigned long flags);

typedef struct RAY_mutex_st
{
	char data[48];
} RAY_mutex_t;

extern void RAY_mutex_init( RAY_mutex_t * mutex );
extern void RAY_mutex_lock( RAY_mutex_t * mutex );
extern void RAY_mutex_unlock( RAY_mutex_t * mutex );

typedef struct RAY_semaphore_st
{
	char data[80];
} RAY_semaphore_t;

extern void RAY_sema_init( RAY_semaphore_t * semaphore, int val );
extern void RAY_up(RAY_semaphore_t *semaphore);
extern int RAY_down_interruptible(RAY_semaphore_t *semaphore);

// <4> wait queue
// include/linux/wait.h

typedef struct RAY_wait_queue_head_st
{
	char data[56];
} RAY_wait_queue_head_t;

extern void RAY_init_waitqueue_head( RAY_wait_queue_head_t * wq );
extern void RAY_wake_up_interruptible( RAY_wait_queue_head_t * wq );
extern int RAY_wait_event_interruptible_timeout(RAY_wait_queue_head_t* wq, int* state, unsigned long timeOut);
extern void RAY_poll_wait( void * filp, RAY_wait_queue_head_t * wq, void * tbl );

// <5> work struct
// include/linux/workqueue.h

typedef struct RAY_work_struct_st
{
	char data[96];
} RAY_work_struct_t; 

typedef void (* RAY_work_func_t)( RAY_work_struct_t * wk );

extern void RAY_INIT_WORK( RAY_work_struct_t * wk, RAY_work_func_t func );
extern void RAY_schedule_work( RAY_work_struct_t * wk );
extern void RAY_cancel_work_sync(RAY_work_struct_t * wk);
extern void RAY_flush_scheduled_work(void);
extern void RAY_create_workqueue(void** wq, char* str);
extern void RAY_queue_work(void* wq, RAY_work_struct_t* wk);
extern void RAY_destroy_workqueue(void* wq);

// <6> timer
// include/linux/timer.h

typedef struct RAY_timer_list_st
{
	char data[120];
} RAY_timer_list_t;

typedef void (* RAY_timer_func_t)( unsigned long arg );

extern int RAY_is_kernel_version_4_15_0_below(void);
extern void RAY_setup_timer( RAY_timer_list_t * tmr, RAY_timer_func_t func, unsigned long data );
extern void RAY_timer_setup(RAY_timer_list_t * tmr, void* func);
extern void RAY_timer_set_expires( RAY_timer_list_t * tmr, unsigned long expires );
extern void RAY_add_timer( RAY_timer_list_t * tmr );
extern void RAY_del_timer_sync( RAY_timer_list_t * tmr );
extern void RAY_mod_timer( RAY_timer_list_t * tmr, unsigned long expires );

// <7> atomic_t
typedef struct RAY_atomic_st
{
	char data[8];
} RAY_atomic_t;

extern void RAY_atomic_set(RAY_atomic_t * atomic, int i);
extern int RAY_atomic_inc_return(RAY_atomic_t * atomic);
extern void RAY_atomic_dec_return(RAY_atomic_t * atomic);
extern void RAY_atomic_dec(RAY_atomic_t* atomic);

// <8> math
extern unsigned long long RAY_do_div(unsigned long long x, unsigned long long y);

// <9> List
#undef offsetof
#ifdef __compiler_offsetof
#define offsetof(TYPE,MEMBER) __compiler_offsetof(TYPE,MEMBER)
#else
#define offsetof(TYPE, MEMBER) ((unsigned long) &((TYPE *)0)->MEMBER)
#endif

/**
 * RAY_container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define RAY_container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})


/*
 * These are non-NULL pointers that will result in page faults
 * under normal circumstances, used to verify that nobody uses
 * non-initialized list entries.
 */
#define RAY_POISON_POINTER_DELTA 0
#define RAY_LIST_POISON1  ((void *) 0x00100100 + RAY_POISON_POINTER_DELTA)
#define RAY_LIST_POISON2  ((void *) 0x00200200 + RAY_POISON_POINTER_DELTA)

/*
 * Simple doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */

struct RAY_list_head {
	struct RAY_list_head *next, *prev;
};

static inline void RAY_INIT_LIST_HEAD(struct RAY_list_head *list)
{
	list->next = list;
	list->prev = list;
}

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __RAY_list_add(struct RAY_list_head *new,
			      struct RAY_list_head *prev,
			      struct RAY_list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

/**
 * RAY_list_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void RAY_list_add(struct RAY_list_head *new, struct RAY_list_head *head)
{
	__RAY_list_add(new, head, head->next);
}

/**
 * RAY_list_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void RAY_list_add_tail(struct RAY_list_head *new, struct RAY_list_head *head)
{
	__RAY_list_add(new, head->prev, head);
}

/**
 * RAY_list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static inline int RAY_list_empty(const struct RAY_list_head *head)
{
	return head->next == head;
}

/**
 * RAY_list_entry - get the struct for this entry
 * @ptr:	the &struct RAY_list_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 */
#define RAY_list_entry(ptr, type, member) \
	RAY_container_of(ptr, type, member)

/**
 * RAY_list_first_entry - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define RAY_list_first_entry(ptr, type, member) \
	RAY_list_entry((ptr)->next, type, member)

/**
 * RAY_list_first_entry - get the end element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define RAY_list_end_entry(ptr, type, member) \
	RAY_list_entry((ptr)->prev, type, member)

/**
 * RAY_list_first_entry_byUser - get the element from a list by user setting
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define RAY_list_first_entry_byUser(ptr, type, member) \
	RAY_list_entry(ptr, type, member)

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __RAY_list_del(struct RAY_list_head * prev, struct RAY_list_head * next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: RAY_list_empty() on entry does not return true after this, the entry is
 * in an undefined state.
 */
static inline void RAY_list_del(struct RAY_list_head *entry)
{
	__RAY_list_del(entry->prev, entry->next);
	entry->next = RAY_LIST_POISON1;
	entry->prev = RAY_LIST_POISON2;
}

/**
 * list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define RAY_list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = RAY_list_entry((head)->next, typeof(*pos), member),	\
		n = RAY_list_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = RAY_list_entry(n->member.next, typeof(*n), member))

#endif // INCLUDED_RAYAPI_H

