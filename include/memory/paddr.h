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

#ifndef __MEMORY_PADDR_H__
#define __MEMORY_PADDR_H__

#include <common.h>

/*
PMEM_LEFT:
	定义：PMEM_LEFT 是一个宏，表示物理内存的左边界。
	类型转换：通过 (paddr_t) 进行类型转换，将其转换为 paddr_t 类型（假设 paddr_t 是物理地址的类型）。
PMEM_RIGHT:
	定义：PMEM_RIGHT 是一个宏，表示物理内存的右边界。
	CONFIG_MBASE 是物理内存的基地址，CONFIG_MSIZE 是物理内存的大小。
	- 1 是为了保证 PMEM_RIGHT 包含在物理内存范围内。
	类型转换：同样通过 (paddr_t) 进行类型转换，将其转换为 paddr_t 类型。
RESET_VECTOR:
	定义：RESET_VECTOR 是一个宏，表示重置向量的地址。
	CONFIG_PC_RESET_OFFSET 是相对于物理内存基地址的偏移量，表示重置向量在物理内存中的位置。
	这个宏的目的是计算出重置向量的实际物理地址。
*/
#define PMEM_LEFT  ((paddr_t)CONFIG_MBASE)
#define PMEM_RIGHT ((paddr_t)CONFIG_MBASE + CONFIG_MSIZE - 1)
#define RESET_VECTOR (PMEM_LEFT + CONFIG_PC_RESET_OFFSET)

/* convert the guest physical address in the guest program to host virtual address in NEMU */
uint8_t* guest_to_host(paddr_t paddr);
/* convert the host virtual address in NEMU to guest physical address in the guest program */
paddr_t host_to_guest(uint8_t *haddr);

static inline bool in_pmem(paddr_t addr) {
  return addr - CONFIG_MBASE < CONFIG_MSIZE;
}

word_t paddr_read(paddr_t addr, int len);
void paddr_write(paddr_t addr, int len, word_t data);

#endif
