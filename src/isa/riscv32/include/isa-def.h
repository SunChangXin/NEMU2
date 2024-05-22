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

#ifndef __ISA_RISCV_H__
#define __ISA_RISCV_H__

#include <common.h>

// 定义了一个 RISC-V CPU 状态结构体，根据编译选项选择不同位数的架构
typedef struct {
    // 通用寄存器组，根据是否启用了 CONFIG_RVE 宏来决定大小
    word_t gpr[MUXDEF(CONFIG_RVE, 16, 32)];
    // 程序计数器
    vaddr_t pc;
} MUXDEF(CONFIG_RV64, riscv64_CPU_state, riscv32_CPU_state);

// 解码信息结构体，根据编译选项选择不同位数的架构
typedef struct {
    union {
        // 存储指令的值，联合体中的 val 成员是一个 32 位的无符号整数
        uint32_t val;
    } inst;
} MUXDEF(CONFIG_RV64, riscv64_ISADecodeInfo, riscv32_ISADecodeInfo);


#define isa_mmu_check(vaddr, len, type) (MMU_DIRECT)

#endif
