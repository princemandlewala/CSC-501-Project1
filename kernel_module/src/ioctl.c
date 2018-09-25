//////////////////////////////////////////////////////////////////////
//                      North Carolina State University
//
//
//
//                             Copyright 2016
//
////////////////////////////////////////////////////////////////////////
//
// This program is free software; you can redistribute it and/or modify it
// under the terms and conditions of the GNU General Public License,
// version 2, as published by the Free Software Foundation.
//
// This program is distributed in the hope it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
//
////////////////////////////////////////////////////////////////////////
//
//   Author:  Hung-Wei Tseng, Yu-Chia Liu
//
//   Description:
//     Core of Kernel Module for Processor Container
//
////////////////////////////////////////////////////////////////////////

#include "processor_container.h"
//#include "core.c"

#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/poll.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/kthread.h>

struct Node{
	int pid;
	struct Node* next;
};

struct Container{
	u64 cid;
	struct Node* head;
	struct Container* next;
};

struct Container_List{
	struct Container* head;
};

struct Container_List global_list_of_containers;

//mutex_init(container_mutex);
//struct mutex container_mutex;
static DEFINE_MUTEX(container_mutex);

//#define DEFINE_MUTEX(container_mutex) container_mutex = __MUTEX_INITIALIZER(container_mutex) 

int add_container_to_container_list(struct Container *new_container){
	if(global_list_of_containers.head == NULL){
		printk("No containers exist\n");
		new_container->next = NULL;
		mutex_lock(&container_mutex);
		global_list_of_containers.head = new_container;
		mutex_unlock(&container_mutex);
	}else{
		printk("At least 1 Container exist\n ");
		struct Container *iterator = global_list_of_containers.head;
		while(iterator->next != NULL){
			iterator = iterator->next;
		}
		mutex_lock(&container_mutex);
		iterator->next = new_container;
		mutex_unlock(&container_mutex);
	}
}

struct Container* check_if_container_exists(u64 value){
	if(global_list_of_containers.head == NULL){
		printk("humarva ke pass koi container nai hai \n");
		return(NULL);
	}else{
		printk("uparwale ki meherbani ek container to hai \n");
		struct Container *iterator = global_list_of_containers.head;
		while(iterator->next != NULL){
			if(iterator->cid == value){
				printk("Eureka!!! container mil gaya \n");
				return(iterator);
			}
			printk("container cid %d",iterator->cid);
			iterator = iterator->next;
		}
		if(iterator->cid == value){
			printk("o mai la! yeh to last wala nikla");
			return(iterator);
		}
		printk("O BC! Yeh to mila hi nai");
		return(NULL);
	}
}


struct Container* get_container(u64 containerID){
	struct Container* iterator = global_list_of_containers.head;
	while(iterator->cid != containerID){
		if(iterator->next == NULL){
			break;
		}else{
			iterator = iterator->next;
		}
	}
	if(iterator->cid == containerID){
		return(iterator);
	}else{
		return(NULL);
	}
}

int delete_task_from_container(struct Container* container, int processID){
	struct Node* iterator = container->head;
	struct Node* previous=NULL;
	printk("hello 1\n");
	if(iterator == NULL){
		printk("sala koi node nai hai andar \n");
	}
	while(iterator->pid != processID){
		printk("hello 2\n");
		if(iterator->next == NULL){
			printk("hello 3\n");
			return(0);
		}else{
			previous = iterator;
			iterator = iterator->next;
			printk("hello 4\n");
		}
	}
	if(previous == NULL && iterator->pid == processID){
		printk("hello 5\n");
		mutex_lock(&container_mutex);
		container->head = iterator->next;
		mutex_unlock(&container_mutex);
		kfree((void *)iterator);
		return(1);
	}else if(iterator->next == NULL && iterator->pid == processID){
		printk("hello 6\n");
		mutex_lock(&container_mutex);
		previous->next = NULL;
		mutex_unlock(&container_mutex);
		kfree((void *)iterator);
		return(1);
	}else if(iterator->pid == processID){
		printk("hello 7\n");
		mutex_lock(&container_mutex);
		previous->next = iterator->next;
		mutex_unlock(&container_mutex);
		kfree((void *)iterator);
		return(1);
	}
	printk("hello 8\n");
	return(0);	
}

int check_if_no_process_in_container(struct Container* container){
	if(container->head == NULL){
		printk("container khali hai");
		return(1);
	}else{
		printk("container me koi hai");
		return(0);
	}
}


int delete_container(struct Container* container){
	struct Container* iterator = global_list_of_containers.head;
	struct Container* previous = NULL;
	printk("hello 9 \n");
	while(iterator->cid != container->cid){
		if(iterator->next == NULL){
			printk("hello 10 \n");
			return(0);
		}else{
			previous = iterator;
			iterator = iterator->next;
			printk("hello 11 \n");
		}
	}
	if(previous == NULL && iterator->cid == container->cid){
		printk("hello 12 \n");
		mutex_lock(&container_mutex);
		global_list_of_containers.head = iterator->next;
		mutex_unlock(&container_mutex);
		printk("%llu ko maar diya",container->cid);
		kfree((void *)iterator);
		return(1);
	}else if(iterator->next == NULL && iterator->cid == container->cid){
		printk("hello 13 \n");
		mutex_lock(&container_mutex);
		previous->next = NULL;
		mutex_unlock(&container_mutex);
		kfree((void *)iterator);
		return(1);
	}else if(iterator->cid == container->cid){
		printk("hello 14 \n");
		mutex_lock(&container_mutex);
		previous->next = iterator->next;
		mutex_unlock(&container_mutex);
		kfree((void *)iterator);
		return(1);
	}
	printk("hello 15 \n");
	return(0);
}
/**
 * Delete the task in the container.
 * 
 * external functions needed:
 * mutex_lock(), mutex_unlock(), wake_up_process(), 
 */
int processor_container_delete(struct processor_container_cmd __user *user_cmd)
{
    printk("In container delete \n");
    //printk("Current process is %d",(int) getpid() );
    struct processor_container_cmd *pcontainerStruct;
    pcontainerStruct = kmalloc(sizeof(struct processor_container_cmd), GFP_KERNEL);
    long cd = copy_from_user(pcontainerStruct, user_cmd, sizeof(struct processor_container_cmd));
    struct Container* container_to_check_for = get_container(pcontainerStruct->cid);
    int m = 10;
    if(container_to_check_for != NULL){
	printk("papa zinda hai");
    	m = delete_task_from_container(container_to_check_for,current->pid);
    }else{
	printk("task ka baap hi nai mila");
    }
    if(m==1){
	printk("task ka khoon karva diya");
    }else if(m==0){
	printk("task mila hi nai marne ke liye");   
    }
    int are_there_any_process_in_container = check_if_no_process_in_container(container_to_check_for);
    if(are_there_any_process_in_container == 1){
	delete_container(container_to_check_for);
	printk("container akela bacha that, maar diya");
    }
    return 0;
}



/**
 * Create a task in the corresponding container.
 * external functions needed:
 * copy_from_user(), mutex_lock(), mutex_unlock(), set_current_state(), schedule()
 * 
 * external variables needed:
 * struct task_struct* current  
 */
int processor_container_create(struct processor_container_cmd __user *user_cmd)
{
    struct processor_container_cmd *pcontainerStruct;
    pcontainerStruct = kmalloc(sizeof(struct processor_container_cmd), GFP_KERNEL);
    long cd = copy_from_user(pcontainerStruct, user_cmd, sizeof(struct processor_container_cmd));
    printk("In container create\n");
    struct Node *new_thread;
    new_thread = kmalloc(sizeof(struct Node), GFP_KERNEL);
    new_thread->pid = current->pid;
    new_thread->next = NULL;
    struct Container *m =check_if_container_exists(pcontainerStruct->cid) ;
    if(m==NULL){
	printk("creating new container\n");
	struct Container *container_new;
    	container_new = kmalloc(sizeof(struct Container), GFP_KERNEL);
	container_new->cid = pcontainerStruct->cid;
	container_new->head = NULL;
	container_new->next = NULL;
	add_container_to_container_list(container_new);
	m = check_if_container_exists(pcontainerStruct->cid);
    }
    printk("adding thread to list\n");
    struct Node* iterator = m->head;
    printk("iterator created");
    if(iterator!=NULL){
    	while(iterator->next != NULL){
		printk("Thread ID: %d",current->pid);
		iterator = iterator->next;
    	}
	mutex_lock(&container_mutex);
	iterator->next = new_thread;
	mutex_unlock(&container_mutex);
    }else{
	mutex_lock(&container_mutex);
	m->head = new_thread;
	mutex_unlock(&container_mutex);
	//iterator = new_thread;
    }
    printk("iterator ended");
    //printk("Current process is: %d", (int)getpid());
    printk("Current conatiner ID: %llu Process ID: %d Process Name: %s \n", pcontainerStruct->cid, current->pid,current->comm);
    return 0;
}

/**
 * switch to the next task in the next container
 * 
 * external functions needed:
 * mutex_lock(), mutex_unlock(), wake_up_process(), set_current_state(), schedule()
 */
int processor_container_switch(struct processor_container_cmd __user *user_cmd)
{
    printk("In container switch\n");
    //printk("Current Process is: %d", (int) getpid());
    //printk("Container ID: %d", (int) user_cmd->cid);
    return 0;
}

/**
 * control function that receive the command in user space and pass arguments to
 * corresponding functions.
 */
int processor_container_ioctl(struct file *filp, unsigned int cmd,
                              unsigned long arg)
{
    switch (cmd)
    {
    case PCONTAINER_IOCTL_CSWITCH:
        return processor_container_switch((void __user *)arg);
    case PCONTAINER_IOCTL_CREATE:
        return processor_container_create((void __user *)arg);
    case PCONTAINER_IOCTL_DELETE:
        return processor_container_delete((void __user *)arg);
    default:
        return -ENOTTY;
    }
}
