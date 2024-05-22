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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// 头文件声明
int choose(int n);
char gen_num();
void gen(char c);
char gen_rand_op();

int buf_index = 0;
// 定义两个缓冲区，用于存储生成的随机表达式和生成的C语言程序
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`

// C语言程序的模板，用于生成随机表达式的程序
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; " 
"  printf(\"%%u\", result); "
"  return 0; "
"}";

// 生成随机表达式的函数
int choose(int n){
	int rnum = rand() % n;
	// printf("rnum: %d\n", rnum);
	return rnum;
}

char gen_num() {
    int num = rand() % 10;  // 生成 0 到 9 之间的随机数字
    char num_char = '0' + num;  // 将数字转换为字符
    
    /*
    // 除零情况的处理
    if(num_char=='0' && buf[buf_index]=='/')
    	num_char = '3';  // 0 --> 3
    	// buf[buf_index] == '*'
    */
    	
    return num_char;  // 将字符返回
}

void gen(char c) {
	// printf("charinput: %c\n", c);
    if (buf_index < 65536 - 2) {  	// 保留至少两个位置给空格和终止符
    	// 随机插入空格
        if (choose(4) == 0) {
            buf[buf_index++] = ' '; // 插入空格
        }
        
        
        
        buf[buf_index++] = c;
        buf[buf_index] = '\0';  // 添加字符串结束符
        // printf("buf_index: %d buf[buf_index]: %c\n",buf_index ,buf[buf_index - 1]);
    } else {
        printf("Buffer nearly full!\n");
        exit(1);
    }
}

char gen_rand_op() {
    switch (choose(4)) {
        case 0: return '+';
        case 1: return '-';
        case 2: return '*';
        case 3: return '/';
        default: return '+';  // 默认返回 '+'
    }
}



// 生成随机表达式的函数
static void gen_rand_expr() {
  switch (choose(3)) {
    case 0: gen(gen_num()); break; // 生成数字
    case 1: gen('('); gen_rand_expr(); gen(')'); break; // 生成括号包围的表达式
    default: 
    	gen_rand_expr(); 
    	char temp_op = gen_rand_op();
    	gen(temp_op);
    	if(temp_op == '/'){ 
    		gen(choose(9) + '1');
    		  		
    	} else {
    		gen_rand_expr();
    	}   	 
    	break; // 生成运算符连接的表达式    
  }
}

int main(int argc, char *argv[]) {
  int seed = time(0); 			// 获取当前时间作为种子
  srand(seed); 					// 初始化随机数生成器
  int loop = 1; 				// 默认只执行一次
  if (argc > 1) { 				// 如果提供了命令行参数，那么使用命令行参数确定循环次数
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) { // 循环生成和执行随机表达式
    gen_rand_expr(); 			// 生成随机表达式
	
    sprintf(code_buf, code_format, buf); 	// 将随机表达式插入到C语言程序模板中
    
    /*
    printf("buf: \n");
    for(int j=0;j<buf_index;j++){
		printf("%c", buf[j]);
	}
	printf("\n");
	printf("code_buf: %s buf_index:%d\n", code_buf, buf_index);
	*/
		
    FILE *fp = fopen("/tmp/.code.c", "w"); 	// 打开临时文件，准备写入C语言程序
    assert(fp != NULL); 					// 确保文件打开成功
    fputs(code_buf, fp); 					// 将C语言程序写入到文件中
    fclose(fp); 							// 关闭文件

    int ret = system("gcc /tmp/.code.c -o /tmp/.expr"); // 使用gcc编译器编译临时文件
    
    
    assert(ret == 0);
    if (ret != 0) continue; 		// 如果编译失败，那么跳过后续步骤，进入下一次循环

    fp = popen("/tmp/.expr", "r"); 	// 使用popen函数执行编译好的程序，并打开标准输出流
    assert(fp != NULL); 			// 确保打开成功

    int result;
    ret = fscanf(fp, "%d", &result); // 从标准输出中读取结果
    pclose(fp); 					 // 关闭标准输出流
	
	printf("%u %s\n", result, buf);
      
    
    // 清零 buf 和 code_buf
    memset(buf, 0, sizeof(buf));
    memset(code_buf, 0, sizeof(code_buf));
    buf_index = 0;
  }
  
  return 0;
}
