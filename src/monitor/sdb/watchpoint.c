/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include "sdb.h"
#include "watchpoint.h"

#define NR_WP 32

/******************** 监视点结构体 **********************/

// watchpoint.h
/* 
typedef struct watchpoint {
  int NO;
  struct watchpoint *next;
  bool flag;                // use / unuse
  char expr[100];
  int new_value;
  int old_value;
} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;
*/

WP wp_pool[NR_WP] = {};

// 构建长度为 32 的空链表
void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;      	// head ：指向链表的头部（第一个监视点）
  free_ = wp_pool;  	// free_：用于管理空闲监视点的链表
}


/******************** 获取一个空闲节点new_wp() 和 释放节点free_wp(WP *wp) **********************/

WP* new_wp(){
    for(WP* p = free_ ; p -> next != NULL ; p = p -> next){  // 遍历 free_
        if( p -> flag == false){    // unuse
            p -> flag = true;
            if(head == NULL){
                head = p;
            }
            return p;
        }
    }
    printf("No unuse point.\n");
    assert(0);
    return NULL;
}

void free_wp(WP *wp){
    if(head -> NO == wp -> NO){
        head -> flag = false;
        head = NULL;
        printf("Delete watchpoint success.\n");
        return ;
    }
    for(WP* p = head ; p -> next != NULL ; p = p -> next){
        if(p -> next -> NO  == wp -> NO)
        {
            p -> next = p -> next -> next;
            p -> next -> flag = false; // 没有被使用
            printf("free success.\n");
            return ;
        }
    }
}

















