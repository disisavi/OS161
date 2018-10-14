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

 /*
  * Driver code for whale mating problem
  */
#include <types.h>
#include <lib.h>
#include <thread.h>
#include <test.h>
#include <synch.h>

#define NMATING 10

static struct semaphore *male_sem;
static struct semaphore *female_sem;
static struct semaphore *match_sem;
static struct lock *match_lock;
static struct lock *male_lock;
static struct lock *female_lock;

static
void 
init()
{
	match_lock = lock_create("Match_lock");
	male_lock = lock_create("male_lock");
	female_lock = lock_create("female_lock");

	KASSERT(match_lock != NULL);
	KASSERT(male_lock != NULL);
	KASSERT(female_lock != NULL);
	
	male_sem = sem_create("male_Sem",0);
	female_sem = sem_create("female_Sem",0);
	match_sem = sem_create("Match_Sem",0);

	KASSERT(male_sem != NULL);
	KASSERT(female_sem != NULL);
	KASSERT(match_lock != NULL);
}

static
void
male(void *p, unsigned long which)
{
	(void)p;
	kprintf("Male whale #%ld starting\n", which);
	//Just waiting for the mathcmaker to signal that match has been found...
	lock_acquire(male_lock);
	P(male_sem);
	lock_release(male_lock);
	kprintf("The male %ld signalled... Mating successfull.\n",which);
}

static
void
female(void *p, unsigned long which)
{
	(void)p;
	kprintf("female whale #%ld starting\n", which);
	//Just waiting for the matchmaker to signal that a match has been foound...
	lock_acquire(female_lock);
	P(female_sem);
	lock_release(female_lock);	
	kprintf("The female %ld signalled... Mating successfull.\n", which);
}

static
void
matchmaker(void *p, unsigned long which)
{
	(void)p;
	kprintf("matchmaker whale #%ld starting\n", which);
	//critical section
	lock_acquire(match_lock);
	bool condition = true;
	lock_release(match_lock);
	//checking if there are any semaphores which are in waiting. If yes, it increments its count and then comes out of critical section
	while(condition)
	{
		lock_acquire(match_lock);
		if(female_sem->sem_count == 0 && male_sem->sem_count == 0)
		{
			V(female_sem);
			V(male_sem);
			condition = false;
		}
		lock_release(match_lock);
	}
	
	kprintf("The matchMaker %ld signalled... Mating successfull.\n", which);
}


// Change this function as necessary
int
whalemating(int nargs, char **args)
{

	kprintf("\n");
	int i, j, err = 0;

	(void)nargs;
	(void)args;
	init();
	for (i = 0; i < 3; i++) {
		for (j = 0; j < NMATING; j++) {
			switch (i) {
			case 0:
				err = thread_fork("Male Whale Thread",
					NULL, male, NULL, j);
				break;
			case 1:
				err = thread_fork("Female Whale Thread",
					NULL, female, NULL, j);
				break;
			case 2:
				err = thread_fork("Matchmaker Whale Thread",
					NULL, matchmaker, NULL, j);
				break;
			}
			if (err) {
				panic("whalemating: thread_fork failed: %s)\n",
					strerror(err));
			}
		}
	}
	return 0;
}
