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

#ifndef __CPU_DECODE_H__
#define __CPU_DECODE_H__

#include <isa.h>

/*
 * 解码结构体，用于存储解码后的指令信息。
 */
typedef struct Decode {
  vaddr_t pc;    // 当前指令的虚拟地址
  vaddr_t snpc;  // 静态下一条指令的虚拟地址
  vaddr_t dnpc;  // 动态下一条指令的虚拟地址
  ISADecodeInfo isa;  // ISA 解码信息结构体
  IFDEF(CONFIG_ITRACE, char logbuf[128]); // 用于指令跟踪的日志缓冲区
} Decode;


// --- pattern matching mechanism ---
/*     pattern_decode
  用于解析一个包含二进制模式的字符串，将其解析为 key、mask 和 shift 三个参数。
  key 是最终的模式值，mask 是模式掩码，shift 是掩码右移的位数
*/
__attribute__((always_inline))
static inline void pattern_decode(const char *str, int len, 
    uint64_t *key, uint64_t *mask, uint64_t *shift) {
  uint64_t __key = 0, __mask = 0, __shift = 0;
#define macro(i) \
  if ((i) >= len) goto finish; \
  else { \
    char c = str[i]; \
    if (c != ' ') { \
      Assert(c == '0' || c == '1' || c == '?', \
          "invalid character '%c' in pattern string", c); \
      __key  = (__key  << 1) | (c == '1' ? 1 : 0); \
      __mask = (__mask << 1) | (c == '?' ? 0 : 1); \
      __shift = (c == '?' ? __shift + 1 : 0); \
    } \
  }

#define macro2(i)  macro(i);   macro((i) + 1)
#define macro4(i)  macro2(i);  macro2((i) + 2)
#define macro8(i)  macro4(i);  macro4((i) + 4)
#define macro16(i) macro8(i);  macro8((i) + 8)
#define macro32(i) macro16(i); macro16((i) + 16)
#define macro64(i) macro32(i); macro32((i) + 32)
  macro64(0);
  Log("include/cpu/decode.h:62  Pattern: %d\n", *str);
  Log("Key:     0x%lx\n", *key);
  Log("Mask:    0x%lx\n", *mask);
  Log("Shift:   %ln\n", shift);
  panic("pattern too long");
#undef macro
finish:
  *key = __key >> __shift;
  *mask = __mask >> __shift;
  *shift = __shift;
}



__attribute__((always_inline))
static inline void pattern_decode_hex(const char *str, int len,
    uint64_t *key, uint64_t *mask, uint64_t *shift) {
  uint64_t __key = 0, __mask = 0, __shift = 0;
#define macro(i) \
  if ((i) >= len) goto finish; \
  else { \
    char c = str[i]; \
    if (c != ' ') { \
      Assert((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || c == '?', \
          "invalid character '%c' in pattern string", c); \
      __key  = (__key  << 4) | (c == '?' ? 0 : (c >= '0' && c <= '9') ? c - '0' : c - 'a' + 10); \
      __mask = (__mask << 4) | (c == '?' ? 0 : 0xf); \
      __shift = (c == '?' ? __shift + 4 : 0); \
    } \
  }

  macro16(0);
  panic("pattern too long");
#undef macro
finish:
  *key = __key >> __shift;
  *mask = __mask >> __shift;
  *shift = __shift;
}


// --- pattern matching wrappers for decode ---
#define INSTPAT(pattern, ...) do { \
  uint64_t key, mask, shift; \
  pattern_decode(pattern, STRLEN(pattern), &key, &mask, &shift); \
  if ((((uint64_t)INSTPAT_INST(s) >> shift) & mask) == key) { \
    INSTPAT_MATCH(s, ##__VA_ARGS__); \
    goto *(__instpat_end); \
  } \
} while (0)

#define INSTPAT_START(name) { const void ** __instpat_end = &&concat(__instpat_end_, name);
#define INSTPAT_END(name)   concat(__instpat_end_, name): ; }

/*
- INSTPAT_START(name) 宏：
	这个宏以给定的 name 参数开始一个指令模式的定义区块。
	{ 表示宏的起始，定义了一个新的作用域。
	const void ** __instpat_end = &&concat(__instpat_end_, name);：
	这行代码定义了一个指针 __instpat_end，其类型是 const void **。
	&& 是获取标签地址的操作符。
	concat(__instpat_end_, name) 会将 __instpat_end_ 与 name 连接起来，作为一个新的标签名。
	整个宏定义的目的是定义一个新的指令模式区块的开始，并为这个区块定义一个结束标签。
- INSTPAT_END(name) 宏：
	这个宏结束前面定义的指令模式区块。
	concat(__instpat_end_, name): ;：
	这行代码定义了一个带有特定名称的标签，用于标记指令模式区块的结束。
	: 表示标签的定义。
	; 表示一个空语句，这是为了确保宏的结束。
	整个宏定义的目的是为前面开始的指令模式区块定义一个结束标签。

*/
#endif
