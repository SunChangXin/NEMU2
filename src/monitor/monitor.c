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

void init_rand();
void init_log(const char *log_file);
void init_mem();
void init_difftest(char *ref_so_file, long img_size, int port);
void init_device();
void init_sdb();
void init_disasm(const char *triple);


static void welcome() {
  /* 打印跟踪信息是否开启 */
  Log("Trace: %s", MUXDEF(CONFIG_TRACE, ANSI_FMT("ON", ANSI_FG_GREEN), ANSI_FMT("OFF", ANSI_FG_RED)));
  
  /* 如果跟踪功能被启用，打印相应提示信息 */
  IFDEF(CONFIG_TRACE, Log("If trace is enabled, a log file will be generated "
                          "to record the trace. This may lead to a large log file. "
                          "If it is not necessary, you can disable it in menuconfig"));
  
  /* 打印构建时间 */
  Log("Build time: %s, %s", __TIME__, __DATE__);
  
  /* 打印欢迎信息 */
  printf("Welcome to %s-NEMU!\n", ANSI_FMT(str(__GUEST_ISA__), ANSI_FG_YELLOW ANSI_BG_RED));
  printf("For help, type \"h\"\n");
  
  /* 打印练习提示信息 */
  // Log("Exercise: Please remove me in the source code and compile NEMU again.");
  // assert(0);
}


#ifndef CONFIG_TARGET_AM
#include <getopt.h>

void sdb_set_batch_mode();

static char *log_file = NULL;
static char *diff_so_file = NULL;
static char *img_file = NULL;
static int difftest_port = 1234;


static long load_img() {
  /* 如果没有指定镜像文件，则使用默认的内置镜像 */
  if (img_file == NULL) {
    Log("No image is given. Use the default built-in image.");
    return 4096; 	// 默认内置镜像大小
  }

  /* 打开镜像文件 */
  FILE *fp = fopen(img_file, "rb");
  Assert(fp, "Can not open '%s'", img_file);

  /* 获取镜像文件大小 */
  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);

  /* 打印镜像文件信息 */
  Log("The image is %s, size = %ld", img_file, size);

  /* 将文件指针重新定位到文件开头 */
  fseek(fp, 0, SEEK_SET);

  /* 从镜像文件中读取内容到内存中 */
  int ret = fread(guest_to_host(RESET_VECTOR), size, 1, fp);
  assert(ret == 1);

  /* 关闭文件 */
  fclose(fp);
  return size;
}


static int parse_args(int argc, char *argv[]) {
  /* 定义命令行选项表 */
  const struct option table[] = {
    {"batch"    , no_argument      , NULL, 'b'},
    {"log"      , required_argument, NULL, 'l'},
    {"diff"     , required_argument, NULL, 'd'},
    {"port"     , required_argument, NULL, 'p'},
    {"help"     , no_argument      , NULL, 'h'},
    {0          , 0                , NULL,  0 },
  };
  int o;
  /* 解析命令行选项 */
  // optarg 是一个全局变量，在使用 getopt 或 getopt_long 解析命令行参数时被系统提供和管理的。
  // 它用来存储当前解析到的选项的参数值（如果有的话）
  
  // 使用 getopt_long 函数来逐个解析命令行参数 
  // 这个函数会返回下一个选项的短名标识符，如果没有选项了则返回 -1
  while ( (o = getopt_long(argc, argv, "-bhl:d:p:", table, NULL)) != -1) {  
    switch (o) {
      case 'b': sdb_set_batch_mode(); break;  		     // sdb.c   是否启用批处理模式            	
      case 'p': sscanf(optarg, "%d", &difftest_port); break; // 使用 sscanf 来解析参数，并将解析得到的值存入 difftest_port 变量中
      case 'l': log_file = optarg; break;     		     // 将参数赋值给 log_file 变量 (定义在上面) 		
      case 'd': diff_so_file = optarg; break;		     // 将参数赋值给 diff_so_file 变量 (定义在上面) 	
      case  1 : img_file = optarg; return 0;		     // 将参数赋值给 img_file 变量 (镜像文件 定义在上面)
      default:		
        /* 显示用法信息并退出 */
        printf("Usage: %s [OPTION...] IMAGE [args]\n\n", argv[0]);
        printf("\t-b,--batch              run with batch mode\n");
        printf("\t-l,--log=FILE           output log to FILE\n");
        printf("\t-d,--diff=REF_SO        run DiffTest with reference REF_SO\n");
        printf("\t-p,--port=PORT          run DiffTest with port PORT\n");
        printf("\n");
        exit(0);
    }
  }
  return 0;
}


void init_monitor(int argc, char *argv[]) {
  /* Perform some global initialization. */

  /* 解析命令行参数 */
  /* 函数体在上面 */
  parse_args(argc, argv);

  /* 初始化随机数生成器 */ 
  // src/utils/timer.c
  init_rand();

  /* 初始化日志记录 */
  // src/utils/log.c
  init_log(log_file);

  /* 初始化内存 */
  // src/memory/paddr.c 
  init_mem();

  /* 如果定义了名为 CONFIG_DEVICE 的宏，那么 init_device() 函数将被调用以初始化设备 */
  IFDEF(CONFIG_DEVICE, init_device());      // src/device/device.c

  /* src/isa/riscv32/init.c */
  /* Perform ISA dependent initialization. */
  init_isa();

  /* 
   调用load_img()函数，将预定义的指令数组加载到内存中，并将加载的指令大小保存在img_size变量中。
   这个操作将会覆盖内置的指令。
  */
  /* 函数体在上面 */
  long img_size = load_img();


  /* 初始化差分测试功能，传入参考的.so文件路径、加载的指令大小和差分测试端口号 */
  // src/cpu/difftest/dut.c
  init_difftest(diff_so_file, img_size, difftest_port);


  /* src/monitor/sdb/sdb.c */
  /* 初始化调试器 */
  init_sdb();

#ifndef CONFIG_ISA_loongarch32r
  // 如果定义了 CONFIG_ITRACE 宏，则调用 init_disasm 函数，根据不同的ISA进行选择
  IFDEF(CONFIG_ITRACE, init_disasm(
    // 使用 MUXDEF 宏根据当前的ISA选择不同的架构
    MUXDEF(CONFIG_ISA_x86,     "i686",  		 // 如果是x86 ISA，则选择i686架构
    MUXDEF(CONFIG_ISA_mips32,  "mipsel",  		 // 如果是MIPS32 ISA，则选择mipsel架构
    MUXDEF(CONFIG_ISA_riscv,
    MUXDEF(CONFIG_RV64,        "riscv64", 		 // 如果是RISC-V ISA，则根据是否为RV64选择riscv64或riscv32架构
                               "riscv32"),
                               "bad"))) "-pc-linux-gnu"  // 默认为"bad"，表示不支持的ISA
  ));
#endif

  /* 打印欢迎信息 */
  /* 函数体在上面 */
  welcome();
}

#else // CONFIG_TARGET_AM
static long load_img() {
  /* 从外部符号中获取图像的起始地址和结束地址 */
  extern char bin_start, bin_end;
  /* 计算图像的大小 */
  size_t size = &bin_end - &bin_start;
  Log("img size = %ld", size);
  /* 将图像内容复制到内存中 */
  memcpy(guest_to_host(RESET_VECTOR), &bin_start, size);
  return size;
}


void am_init_monitor() {
  init_rand();
  init_mem();
  init_isa();
  load_img();
  IFDEF(CONFIG_DEVICE, init_device());
  welcome();
}
#endif
