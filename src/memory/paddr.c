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

#include <memory/host.h>
#include <memory/paddr.h>
#include <device/mmio.h>
#include <isa.h>

// 根据配置选择静态数组或动态分配内存
#if defined(CONFIG_PMEM_MALLOC)
static uint8_t *pmem = NULL; // 如果使用动态分配内存，则定义指针
#else // CONFIG_PMEM_GARRAY

// include/generated/autoconf.h  #define CONFIG_MSIZE 0x8000000  128MB
static uint8_t pmem[CONFIG_MSIZE] PG_ALIGN = {}; // 如果使用静态数组，则定义数组

#endif


// 将客户机物理地址映射到主机物理内存地址
uint8_t* guest_to_host(paddr_t paddr) { return pmem + paddr - CONFIG_MBASE; }

// 将主机物理内存地址映射回客户机物理地址
paddr_t host_to_guest(uint8_t *haddr) { return haddr - pmem + CONFIG_MBASE; }

// 从物理内存中读取数据
static word_t pmem_read(paddr_t addr, int len) {
  word_t ret = host_read(guest_to_host(addr), len);     // include/memory/host.h
  return ret;
}

// 向物理内存中写入数据
static void pmem_write(paddr_t addr, int len, word_t data) {
  host_write(guest_to_host(addr), len, data);
}

// 物理内存边界检查，如果地址超出范围则触发异常
static void out_of_bound(paddr_t addr) {
  panic("address = " FMT_PADDR " is out of bound of pmem [" FMT_PADDR ", " FMT_PADDR "] at pc = " FMT_WORD,
      addr, PMEM_LEFT, PMEM_RIGHT, cpu.pc);
}

// 初始化物理内存
void init_mem() {
#if defined(CONFIG_PMEM_MALLOC)
  // 如果使用动态分配内存，分配内存空间并进行断言
  pmem = malloc(CONFIG_MSIZE);
  assert(pmem);
#endif

#ifdef CONFIG_MEM_RANDOM
  // 如果启用了随机初始化内存内容，则使用随机数填充内存
  memset(pmem, rand(), CONFIG_MSIZE);
#endif

  // 记录物理内存区域范围   FMT_PADDR 是一个宏，用于格式化打印物理地址
  Log("physical memory area [" FMT_PADDR ", " FMT_PADDR "]", PMEM_LEFT, PMEM_RIGHT);
}


// 从指定物理地址读取数据
word_t paddr_read(paddr_t addr, int len) {
  /* likely */
  // 在代码中使用 likely(cond) 时，实际上是告诉编译器，条件 cond 很可能为真
  // 期望优化器将其执行路径标记为可能执行的路径
  // include/macro.h
  
  /* in_pmem */
  // 判断给定的物理地址 addr 是否在某个特定的内存范围内，即是否在持久内存（PMEM）中
  // include/memory/paddr.h
  
  if (likely(in_pmem(addr))) return pmem_read(addr, len);
  IFDEF(CONFIG_DEVICE, return mmio_read(addr, len));
  out_of_bound(addr);
  return 0;
}

// 向指定物理地址写入数据
void paddr_write(paddr_t addr, int len, word_t data) {
  if (likely(in_pmem(addr))) { pmem_write(addr, len, data); return; }
  IFDEF(CONFIG_DEVICE, mmio_write(addr, len, data); return);
  out_of_bound(addr);
}

