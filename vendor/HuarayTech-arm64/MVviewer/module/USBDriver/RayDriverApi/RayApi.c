// RayApi.c

#include "RayApi.h"

#include <stdarg.h>
#include <linux/version.h>
#include <linux/poll.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/math64.h>

///////////////////////////////////////////////////////////
// <1> linux common

int RAY_printk( const char * fmt, ... )
{
	int ret = 0;
	va_list va;
	va_start( va, fmt );
	ret = vprintk( fmt, va );
	va_end( va );
	return ret;
}

char *RAY_strrchr(const char *str, char c)
{
	return strrchr(str, c);
}

unsigned long RAY_jiffies()
{
	return jiffies;
}

unsigned long RAY_msecs_to_jiffies( unsigned int m )
{
	return msecs_to_jiffies( m );
}

unsigned long RAY_usecs_to_jiffies( unsigned int u )
{
	return usecs_to_jiffies( u );
}

unsigned int RAY_jiffies_to_msecs( const unsigned long j )
{
	return jiffies_to_msecs( j );
}

unsigned int RAY_jiffies_to_usecs( const unsigned long j )
{
	return jiffies_to_usecs( j );
}

int RAY_HZ()
{
	return HZ;
}

int RAY_PAGE_ALIGN( int size )
{
	return PAGE_ALIGN( size );
}

///////////////////////////////////////////////////////////
// <2> memory

void * RAY_kmalloc( int size, int flag )
{
	int myflag = GFP_KERNEL;
	switch( flag )
	{
	case RAY_GFP_KERNEL :
		myflag = GFP_KERNEL;
		break;
	case RAY_GFP_ATOMIC:
		myflag = GFP_ATOMIC;
		break;
	default :
		break;
	}
	return kmalloc( size, myflag );
}

void * RAY_kzalloc(int size, int flag)
{
	int myflag = GFP_KERNEL;
	switch( flag )
	{
	case RAY_GFP_KERNEL :
		myflag = GFP_KERNEL;
		break;
	default :
		break;
	}

	return kzalloc(size, myflag);
}

void RAY_kfree( void * p )
{
	kfree( p );
	return;
}

void * RAY_vmalloc_user( int size )
{
	return vmalloc_user( size );
}

void * RAY_vmalloc_32(int size)
{
	return vmalloc_32(size);
}

void RAY_vfree( void * buf )
{
	vfree( buf );
}

void * RAY_memcpy( void * dst, const void * src, int size )
{
	return memcpy( dst, src, size );
}

void * RAY_memset( void * dst, int ch, int size )
{
	return memset( dst, ch, size );
}

void RAY_strlcpy( void * dst, const void * src, int size )
{
	 strlcpy( dst, src, size );
}

void * RAY_vmalloc_to_page(unsigned long addr)
{
	return vmalloc_to_page((void*)addr);
}

int RAY_vm_insert_page(void* vma, unsigned long addr, void* page)
{
	struct vm_area_struct* vmaData = (struct vm_area_struct*)vma;
	struct page* pageData = (struct page*)page;
	
	return vm_insert_page(vmaData, addr, pageData);
}

unsigned int RAY_pageSizeGet(unsigned int pageSize)
{
	unsigned int __pageSize = PAGE_SIZE;

	switch( pageSize )
	{
	case RAY_PAGE_SIZE :
		__pageSize = PAGE_SIZE;
		break;
	default :
		break;
	}

	return __pageSize;
}

int RAY_copy_to_user( void * dst, const void * src, int size )
{
	return (int)copy_to_user( dst, src, size );
}

int RAY_copy_from_user( void * dst, const void * src, int size )
{
	return (int)copy_from_user( dst, src, size );
}

int RAY_remap_vmalloc_range( void * vma, void * addr, unsigned long pgoff)
{
	int ret = remap_vmalloc_range( (struct vm_area_struct *)vma, addr, pgoff );
	if( ret == 0 )
	{
		((struct vm_area_struct *)vma)->vm_flags |= ( VM_IO | VM_DONTEXPAND | VM_USERMAP );
	}
	return ret;
}

///////////////////////////////////////////////////////////
// <3> lock

RAY_BUILD_BUG_ON( sizeof(RAY_spinlock_t) < sizeof(spinlock_t) );

void RAY_spin_lock_init( RAY_spinlock_t * splock )
{
	spin_lock_init( (spinlock_t *)splock );
}

void RAY_spin_lock( RAY_spinlock_t * splock )
{
	spin_lock( (spinlock_t *)splock );
}

void RAY_spin_unlock( RAY_spinlock_t * splock )
{
	spin_unlock( (spinlock_t *)splock );
}

void RAY_spin_lock_bh( RAY_spinlock_t * splock )
{
	spin_lock_bh( (spinlock_t *)splock );
}

void RAY_spin_unlock_bh( RAY_spinlock_t * splock )
{
	spin_unlock_bh( (spinlock_t *)splock );
}

void RAY_spin_lock_irqsave(RAY_spinlock_t * splock, unsigned long flags)
{
	spin_lock_irqsave((spinlock_t *)splock, flags);
}

void RAY_spin_unlock_irqrestore(RAY_spinlock_t * splock, unsigned long flags)
{
	spin_unlock_irqrestore((spinlock_t *)splock, flags);
}

void RAY_mutex_init( RAY_mutex_t * mutex )
{
	mutex_init( (struct mutex *)mutex );
}

void RAY_mutex_lock( RAY_mutex_t * mutex )
{
	mutex_lock( (struct mutex *)mutex );
}

void RAY_mutex_unlock( RAY_mutex_t * mutex )
{
	mutex_unlock( (struct mutex *)mutex );
}

void RAY_sema_init( RAY_semaphore_t * semaphore, int val )
{
	sema_init( (struct semaphore *)semaphore, val );
}

void RAY_up(RAY_semaphore_t *semaphore)
{
	up((struct semaphore *)semaphore);
}

int RAY_down_interruptible(RAY_semaphore_t *semaphore)
{
	return down_interruptible((struct semaphore *)semaphore);
}

///////////////////////////////////////////////////////////
// <4> wait queue

RAY_BUILD_BUG_ON( sizeof(RAY_wait_queue_head_t) < sizeof(wait_queue_head_t) );

void RAY_init_waitqueue_head( RAY_wait_queue_head_t * wq )
{
	init_waitqueue_head( (wait_queue_head_t*)(wq) );
}

void RAY_wake_up_interruptible( RAY_wait_queue_head_t * wq )
{
	wake_up_interruptible( (wait_queue_head_t *)(wq) );
}

int RAY_wait_event_interruptible_timeout(RAY_wait_queue_head_t* wq, int* state, unsigned long timeOut)
{
	wait_queue_head_t* wait = (wait_queue_head_t *)wq;
	return wait_event_interruptible_timeout(*wait, *state == true, timeOut);
}

void RAY_poll_wait( void * filp, RAY_wait_queue_head_t * wq, void * tbl )
{
	poll_wait( (struct file *)filp, (wait_queue_head_t *)wq, (poll_table *)tbl );
	return;
}

///////////////////////////////////////////////////////////
// <5> work struct

RAY_BUILD_BUG_ON( sizeof(RAY_work_struct_t) < sizeof(struct work_struct) );

void RAY_INIT_WORK( RAY_work_struct_t * wk, RAY_work_func_t func )
{
	INIT_WORK( (struct work_struct *)wk, (work_func_t)func );
}

void RAY_schedule_work( RAY_work_struct_t * wk )
{
	schedule_work( (struct work_struct *)wk );
}

void RAY_cancel_work_sync(RAY_work_struct_t * wk)
{
	cancel_work_sync((struct work_struct *)wk);
}

void RAY_flush_scheduled_work(void)
{
	flush_scheduled_work();
}

void RAY_create_workqueue(void** wq, char* str)
{
	*wq = (void*)create_workqueue(str);
}

void RAY_queue_work(void* wq, RAY_work_struct_t* wk)
{
	queue_work((struct workqueue_struct*)wq, (struct work_struct*)wk);
}

void RAY_destroy_workqueue(void* wq)
{
	destroy_workqueue((struct workqueue_struct*)wq);
}


///////////////////////////////////////////////////////////
// <6> timer

RAY_BUILD_BUG_ON( sizeof(RAY_timer_list_t) < sizeof(struct timer_list) );

int RAY_is_kernel_version_4_15_0_below(void)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0)
	return 1;
#else
	return 0;
#endif
}

void RAY_setup_timer( RAY_timer_list_t * tmr, RAY_timer_func_t func, unsigned long data )
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0)
	setup_timer( (struct timer_list *)tmr, func, data );
#endif
}

void RAY_timer_setup(RAY_timer_list_t * tmr, void* func)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,15,0)
	timer_setup((struct timer_list *)tmr, func, 0);
#endif
}

void RAY_timer_set_expires( RAY_timer_list_t * tmr, unsigned long expires )
{
	((struct timer_list *)tmr)->expires = expires;
}

void RAY_add_timer( RAY_timer_list_t * tmr )
{
	add_timer( (struct timer_list *)tmr );
}

void RAY_del_timer_sync( RAY_timer_list_t * tmr )
{
	del_timer_sync( (struct timer_list *)tmr );
}

void RAY_mod_timer( RAY_timer_list_t * tmr, unsigned long expires )
{
	mod_timer( (struct timer_list *)tmr, expires );
}

///////////////////////////////////////////////////////////
// <7> atomic_t
void RAY_atomic_set(RAY_atomic_t * atomic, int i)
{
	atomic_set((atomic_t*)atomic, i);
}

int RAY_atomic_inc_return(RAY_atomic_t * atomic)
{
	return atomic_inc_return((atomic_t*)atomic);
}

void RAY_atomic_dec_return(RAY_atomic_t * atomic)
{
	atomic_dec_return((atomic_t*)atomic);
}

void RAY_atomic_dec(RAY_atomic_t* atomic)
{
	atomic_dec((atomic_t*)atomic);
}

unsigned long long RAY_do_div(unsigned long long x, unsigned long long y)
{
	return div64_u64(x,y);
}
