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

#include <cpu/cpu.h>
#include <cpu/decode.h>
#include <cpu/difftest.h>
#include <locale.h>
#include "isa.h"
#include "/home/scx/ysyx-workbench/nemu/src/monitor/sdb/watchpoint.h"

/* The assembly code of instructions executed is only output to the screen
 * when the number of instructions executed is less than this value.
 * This is useful when you use the `si' command.
 * You can modify this value as you want.
 */
#define MAX_INST_TO_PRINT 10

// 全局变量cpu
CPU_state cpu = {}; // CPU_state 在 src/isa/$ISA/include/isa-def.h 定义
uint64_t g_nr_guest_inst = 0;
static uint64_t g_timer = 0; // unit: us
static bool g_print_step = false;

void device_update();

static void trace_and_difftest(Decode *_this, vaddr_t dnpc)
{
#ifdef CONFIG_ITRACE_COND
    if (ITRACE_COND)
    {
        log_write("%s\n", _this->logbuf);
    }
#endif
    if (g_print_step)
    {
        IFDEF(CONFIG_ITRACE, puts(_this->logbuf));
    }
    IFDEF(CONFIG_DIFFTEST, difftest_step(_this->pc, dnpc));

    // 扫描所有监视点
    // bool expr_change = false;
    for (int i = 0; i < NR_WP; i++)
    {
        if (wp_pool[i].flag) // use
        {
            wp_pool[i].old_value = wp_pool[i].new_value;
            bool success = false;
            wp_pool[i].new_value = expr(wp_pool[i].expr, &success);

            if (success)
            {
                if (wp_pool[i].new_value != wp_pool[i].old_value)
                {
                    nemu_state.state = NEMU_STOP;
                    // expr_change = true;
                    printf("%s的值发生了变化\told_value: %d\tnew_value: %d\n", wp_pool[i].expr, wp_pool[i].old_value, wp_pool[i].new_value);
                    // printf("wp_pool[%d]的值发生了变化\n", i);
                    return;
                }
                else
                {
                    // printf("wp_pool[%d]的值: %d\n", i, tmp);
                }
            }
            else
            {
                Log("求值时发生错误.\n");
                assert(0);
            }
        }
    }
    // if (expr_change)
    //     nemu_state.state = NEMU_STOP; // 若值发生变化 则停止程序（NEMU）
    // printf("nemu_state.state: %d %d\n", nemu_state.state, NEMU_STOP);
}

/************* exec_once **************/
static void exec_once(Decode *s, vaddr_t pc)
{
    // vaddr_t 在 include/common.h 定义: typedef word_t vaddr_t;

    s->pc = pc;
    s->snpc = pc;
    isa_exec_once(s); // src/isa/riscv32/inst.c
    cpu.pc = s->dnpc; // 更新PC
#ifdef CONFIG_ITRACE
    char *p = s->logbuf;
    p += snprintf(p, sizeof(s->logbuf), FMT_WORD ":", s->pc);
    int ilen = s->snpc - s->pc;
    int i;
    uint8_t *inst = (uint8_t *)&s->isa.inst.val;
    for (i = ilen - 1; i >= 0; i--)
    {
        p += snprintf(p, 4, " %02x", inst[i]);
    }
    int ilen_max = MUXDEF(CONFIG_ISA_x86, 8, 4);
    int space_len = ilen_max - ilen;
    if (space_len < 0)
        space_len = 0;
    space_len = space_len * 3 + 1;
    memset(p, ' ', space_len);
    p += space_len;

#ifndef CONFIG_ISA_loongarch32r
    void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
    disassemble(p, s->logbuf + sizeof(s->logbuf) - p,
                MUXDEF(CONFIG_ISA_x86, s->snpc, s->pc), (uint8_t *)&s->isa.inst.val, ilen);
#else
    p[0] = '\0'; // the upstream llvm does not support loongarch32r
#endif
#endif
}

/************* execute **************/
static void execute(uint64_t n)
{
    /*  include/cpu/decode.h

        typedef struct Decode {
          vaddr_t pc;    		// 当前指令的虚拟地址
          vaddr_t snpc;  		// 静态下一条指令的虚拟地址
          vaddr_t dnpc;  		// 动态下一条指令的虚拟地址
          ISADecodeInfo isa;  		// ISA 解码信息结构体
          IFDEF(CONFIG_ITRACE, char logbuf[128]); 	// 用于指令跟踪的日志缓冲区
        } Decode;
    */

    Decode s;

    // 输出PA0地址
    Log("PC0: 0x%x\n", cpu.pc);

    for (; n > 0; n--)
    {
        exec_once(&s, cpu.pc);
        g_nr_guest_inst++; // 用于记录客户指令的计数器
        trace_and_difftest(&s, cpu.pc);
        if (nemu_state.state != NEMU_RUNNING)
            break;
        IFDEF(CONFIG_DEVICE, device_update());
    }
}

static void statistic()
{ // 用于输出一些统计数据，包括主机时间花费、总的虚拟机指令数以及模拟频率

    IFNDEF(CONFIG_TARGET_AM, setlocale(LC_NUMERIC, ""));
#define NUMBERIC_FMT MUXDEF(CONFIG_TARGET_AM, "%", "%'") PRIu64

    /*
        NUMBERIC_FMT 宏：
        NUMBERIC_FMT 使用了 MUXDEF 宏来选择不同的格式化字符串。
          -	如果 CONFIG_TARGET_AM 被定义，NUMBERIC_FMT 将会是 "%" PRIu64。
          -	如果 CONFIG_TARGET_AM 未被定义，NUMBERIC_FMT 将会是 %" PRIu64。
        PRIu64 是 C 标准库中定义的用于无符号 64 位整数的格式宏。
    */

    Log("host time spent = " NUMBERIC_FMT " us", g_timer);
    Log("total guest instructions = " NUMBERIC_FMT, g_nr_guest_inst);
    if (g_timer > 0)
        Log("simulation frequency = " NUMBERIC_FMT " inst/s", g_nr_guest_inst * 1000000 / g_timer);
    else
        Log("Finish running in less than 1 us and can not calculate the simulation frequency");
}

void assert_fail_msg()
{
    isa_reg_display();
    statistic();
}

/* Simulate how the CPU works. */
/************* cpu_exec **************/
void cpu_exec(uint64_t n)
{
    g_print_step = (n < MAX_INST_TO_PRINT);
    switch (nemu_state.state)
    {
    case NEMU_END:
    case NEMU_ABORT:
        printf("Program execution has ended. To restart the program, exit NEMU and run again.\n");
        return;
    default:
        nemu_state.state = NEMU_RUNNING;
    }

    uint64_t timer_start = get_time(); // src/utils/timer.c

    execute(n); // n = -1 时跳过

    uint64_t timer_end = get_time();
    g_timer += timer_end - timer_start; // 计算执行n条命令的时间 加到 g_timer 中

    switch (nemu_state.state)
    {
    case NEMU_RUNNING:
        nemu_state.state = NEMU_STOP;
        break;

    case NEMU_END:
    case NEMU_ABORT:
        Log("nemu: %s at pc = " FMT_WORD,
            (nemu_state.state == NEMU_ABORT ? ANSI_FMT("ABORT", ANSI_FG_RED) : (nemu_state.halt_ret == 0 ? ANSI_FMT("HIT GOOD TRAP", ANSI_FG_GREEN) : ANSI_FMT("HIT BAD TRAP", ANSI_FG_RED))),
            nemu_state.halt_pc);
        // fall through
    case NEMU_QUIT:
        statistic();
    }
}
