/*
 * Copyright (c) 2001, 2002, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <types.h>
#include <lib.h>
#include <clock.h>
#include <thread.h>
#include <test.h>
#include <synch.h>
#define NPEOPLE 20


static struct semaphore *male_sem;
static struct semaphore *female_sem;
static struct lock *male_lock;
static struct lock *female_lock;
static struct cv *male_cv;
static struct cv *female_cv;
int male_in_bathroom = 0;
int female_in_bathroom = 0;

static
void
bathroom_init()
{
	male_sem = sem_create("male_sem",4);
	female_sem = sem_create("female_sem",4);

	male_lock = lock_create("male_lock");
	female_lock = lock_create("female_lock");

	male_cv = cv_create("male_cv");
	female_cv = cv_create("female_cv");
	KASSERT(male_lock != NULL);
	KASSERT(male_sem != NULL);
	KASSERT(female_lock != NULL);
	KASSERT(female_sem != NULL);
}


static
void
shower()
{
	// The thread enjoys a refreshing shower!
        clocksleep(1);
}

static
void
boy(void *p, unsigned long which)
{
	(void)p;
	kprintf("boy #%ld starting\n", which);
	// Implement this function
	P(male_sem);
	// use bathroom
	
	bool notwashroomUsed = true;
	while(notwashroomUsed)
	{
		if(male_in_bathroom <3 && female_in_bathroom == 0)
		{
			lock_acquire(male_lock);
			male_in_bathroom++;
			kprintf("boy #%ld entering bathroom...\n", which);
			kprintf("boys in bathroom now = #%d ...\n", male_in_bathroom);
			lock_release(male_lock);
			shower();
			kprintf("boy #%ld leaving bathroom\n", which);
			V(male_sem);
			lock_acquire(male_lock);
			notwashroomUsed = false;
			male_in_bathroom--;
			kprintf("boys in bathroom now = #%d ...\n", male_in_bathroom);
			lock_release(male_lock);	
			if(male_in_bathroom == 0 && female_sem->sem_count==0)
			{
				lock_acquire(male_lock);
				cv_wait(male_cv,male_lock);
				lock_release(male_lock);

				if(lock_do_i_hold(female_lock)){
					cv_signal(female_cv,female_lock);
				}
			}
		}
	}

}

static
void
girl(void *p, unsigned long which)
{
	(void)p;
	kprintf("girl #%ld starting\n", which);
	P(female_sem);
	// use bathroom
	bool notwashroomUsed = true;
	while(notwashroomUsed)
	{
		if(female_in_bathroom<3 && male_in_bathroom == 0)
		{
			lock_acquire(female_lock);
			female_in_bathroom++;
			kprintf("girl #%ld entering bathroom...\n", which);
			kprintf("Girls in bathroom now = #%d ...\n",  female_in_bathroom);
			lock_release(female_lock);
			shower();
			kprintf("girl #%ld leaving bathroom\n", which);
			V(female_sem);
			lock_acquire(female_lock);
			notwashroomUsed = false;
			female_in_bathroom--;
			kprintf("girls in bathroom now = #%d ...\n", female_in_bathroom);
			lock_release(female_lock);
			if(female_in_bathroom == 0 && male_sem->sem_count >0)
			{
				lock_acquire(female_lock);
				cv_wait(female_cv,female_lock);
				lock_release(female_lock);

				if(lock_do_i_hold(male_lock))
				{
					cv_signal(male_cv,male_lock);
				}
			}
		}
	}
}

// Change this function as necessary
int
bathroom(int nargs, char **args)
{

	int i, err=0;

	(void)nargs;
	(void)args;

	bathroom_init();

	for (i = 0; i < NPEOPLE; i++) {
		switch(i % 2) {
			case 0:
			err = thread_fork("Boy Thread", NULL,
					  boy, NULL, i);
			break;
			case 1:
			err = thread_fork("Girl Thread", NULL,
					  girl, NULL, i);
			break;
		}
		if (err) {
			panic("bathroom: thread_fork failed: %s)\n",
				strerror(err));
		}
	}


	return 0;
}

