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

#include <isa.h>
#include <memory/paddr.h>

// 这与uint8_t类型不一致，但由于我们不直接访问数组，所以这样做是可以的
static const uint32_t img [] = {
  0x00000297,  // auipc t0, 0: 将当前 PC 加上 0 并将结果存储在寄存器 t0 中
  0x00028823,  // sb  zero,16(t0): 将寄存器 zero 的值（值为 0）存储到地址为当前 PC 加上 16 的内存位置中
  0x0102c503,  // lbu a0,16(t0): 将地址为当前 PC 加上 16 的内存位置中的字节加载到寄存器 a0 中
  0x00100073,  // ebreak (used as nemu_trap): 触发 NemU 中断指令，通常用于中断处理
  0xdeadbeef,  // some data: 一些数据，可能用于测试或其他目的的占位符数据
};

static void restart() {
  // RESET_VECTOR 定义在 include/memory/paddr.h ：
  // 重置向量（Reset Vector）是在计算机系统中特定的地址 是处理器启动或重启时执行的第一个指令的地址
  // RESET_VECTOR 宏用来定义重置向量的地址
  cpu.pc = RESET_VECTOR;

  /* The zero register is always 0. */
  cpu.gpr[0] = 0;
}

void init_isa() {
  /* 加载内置镜像 */
  //将预先定义的指令数组 img 复制到内存中的指定位置
  memcpy(guest_to_host(RESET_VECTOR), img, sizeof(img));

  /* 重启系统 */
  restart();
}
