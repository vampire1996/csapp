## bomb实验流程
首先解压bomb.tar,该压缩文件是为自学学生提供的。
在bomb文件夹下打开终端，输入
```
gdb bomb 
```
进入调试bomb界面。

```
objdump -d bomb
```
将bomb文件反汇编，可以将输出的反汇编代码保存到文档中。

根据反汇编文件，可以得到6个phase的反汇编代码

将需要输入的内容保存在answer.txt中，gdb运行
```
run answer.txt
```
 
### (1) phase 1
phase 1的反汇编代码

```
0000000000400ee0 <phase_1>:
  400ee0:	48 83 ec 08          	sub    $0x8,%rsp  #申请了8个字节的栈空间
  400ee4:	be 00 24 40 00       	mov    $0x402400,%esi
  400ee9:	e8 4a 04 00 00       	callq  401338 <strings_not_equal>
  400eee:	85 c0                	test   %eax,%eax
  400ef:	74 05                	je     400ef7 <phase_1+0x17>
  400ef2:	e8 43 05 00 00       	callq  40143a <explode_bomb>
  400ef7:	48 83 c4 08          	add    $0x8,%rsp
  400efb:	c3                   	retq 
```


输入字符串的首地址保存在$rdi中--第一个参数

%rax--保存返回值

```
test   %eax,%eax 
```
将两个操作数相与,用于判断%eax是否为0，如果为0那么跳转到<phase_1+0x17>即为400ef7:这行代码，即可跳过explode_bomb

%eax中保存输入字符串和地址0x402400开始保存的字符串，输入

```
x/s 0x402400
```
输出该字符串

```
Border relations with Canada have never been better.
```

因此拆除该炸弹只需要输入与之相同的字符串即可

### (2) phase 2
phase 2的反汇编代码

```
0000000000400efc <phase_2>:
  400efc:	55                   	push   %rbp  #将寄存器值入栈 这样之后的修改就不会改变程序中结束后该寄存器的值
  400efd:	53                   	push   %rbx
  400efe:	48 83 ec 28          	sub    $0x28,%rsp  # %rsp=%rsp-$0x28  $0x28为立即数  %rsp中的值减小2*16+8=40 可以理解为在内存中开辟40个字节的空间
  400f02:	48 89 e6             	mov    %rsp,%rsi
  400f05:	e8 52 05 00 00       	callq  40145c <read_six_numbers>
  400f0a:	83 3c 24 01          	cmpl   $0x1,(%rsp)  #cmpl  比较双字 (%rsp)  %rsp的值作为地址 找到内存中该地址的值将其和0x1相比 如果不等就会爆炸
  400f0e:	74 20                	je     400f30 <phase_2+0x34>  #相等 则跳转
  400f10:	e8 25 05 00 00       	callq  40143a <explode_bomb>
  400f15:	eb 19                	jmp    400f30 <phase_2+0x34>
  400f17:	8b 43 fc             	mov    -0x4(%rbx),%eax # -0x4是地址偏移量  %eax=(%rbx-4)=(%rsp) (前一次循环的(%rsp))
  400f1a:	01 c0                	add    %eax,%eax # %eax=2*(%rsp) (前一次循环的(%rsp))
  400f1c:	39 03                	cmp    %eax,(%rbx) # 比较(%rbx) 和2* (%rbx-4) 二者必须相等
  400f1e:	74 05                	je     400f25 <phase_2+0x29> #phase_2+0x29=0x400f25
  400f20:	e8 15 05 00 00       	callq  40143a <explode_bomb>
  400f25:	48 83 c3 04          	add    $0x4,%rbx # %rbx=%rbx+4
  400f29:	48 39 eb             	cmp    %rbp,%rbx # %rbx=%rsp+24  时 终止循环
  400f2c:	75 e9                	jne    400f17 <phase_2+0x1b> #phase_2+0x1b=0x400f17
  400f2e:	eb 0c                	jmp    400f3c <phase_2+0x40> #phase_2+0x40=0x400f3c
  400f30:	48 8d 5c 24 04       	lea    0x4(%rsp),%rbx  # %rbx=%rsp+4
  400f35:	48 8d 6c 24 18       	lea    0x18(%rsp),%rbp # %rbp=%rsp+24
  400f3a:	eb db                	jmp    400f17 <phase_2+0x1b>
  400f3c:	48 83 c4 28          	add    $0x28,%rsp  # 加上之前减去的值以保证%rsp在宏观上的不变
  400f40:	5b                   	pop    %rbx
  400f41:	5d                   	pop    %rbp
  400f42:	c3                   	retq
```

设置断点在调用read_six_numbers之后
```
(gdb) break *0x400f0a
```

猜测read_six_numbers是输入6个数字，那么运行run,接下来输入 
```
1 2 3 4 5 6
```

接下来程序中会比较$0x1,(%rsp),因此首选查看%rsp
```
(gdb) print /x $rsp
```

得到
```
$1 = 0x7fffffffdc30
```
该值是一个地址，再查看该地址处的int值

```
(gdb) print *(int *) 0x7fffffffdc30
```
可以发现就是我们刚输入的第一个数字，
```
(gdb) print *(int *) 0x7fffffffdc34
```
将地址+4发现输出2，呢么就可以理解:(%rsp),(%rsp+4),(%rsp+8),(%rsp+12),(%rsp+16),(%rsp+20) 分别是输入6个数字的起始位置

可以发现 之后的程序就是比较这六个数 是否是一个逐个乘以2的等比数列

因此只需输入,就可以拆掉炸弹
```
1 2 4 8 16 32 64 
```


### (3) phase 3
phase 3的反汇编代码

```
1 2
0000000000400f43 <phase_3>:
  400f43:	48 83 ec 18          	sub    $0x18,%rsp         # %rsp=%rsp-$0x18  $0x18为立即数  %rsp中的值减小16+8=24 可以理解为在内存中开辟40个字节的空间
  400f47:	48 8d 4c 24 0c       	lea    0xc(%rsp),%rcx     # (%rsp+0xc)->%rcx
  400f4c:	48 8d 54 24 08       	lea    0x8(%rsp),%rdx     # (%rsp+0x8)->%rdx
  400f51:	be cf 25 40 00       	mov    $0x4025cf,%esi
  400f56:	b8 00 00 00 00       	mov    $0x0,%eax
  400f5b:	e8 90 fc ff ff       	callq  400bf0 <__isoc99_sscanf@plt>  # 有两个输入    x1 x2  %eax=x2
  400f60:	83 f8 01             	cmp    $0x1,%eax  # %eax > 1 则跳转  有符号数比较
  400f63:	7f 05                	jg     400f6a <phase_3+0x27>
  400f65:	e8 d0 04 00 00       	callq  40143a <explode_bomb>
  400f6a:	83 7c 24 08 07       	cmpl   $0x7,0x8(%rsp)  # 7 < (%rsp+0x8)=x1   则跳转  无符号数比较
  400f6f:	77 3c                	ja     400fad <phase_3+0x6a>
  400f71:	8b 44 24 08          	mov    0x8(%rsp),%eax  # %eax=x1
  400f75:	ff 24 c5 70 24 40 00 	jmpq   *0x402470(,%rax,8)  # *(8*rax+0x402470)  
  400f7c:	b8 cf 00 00 00       	mov    $0xcf,%eax
  400f81:	eb 3b                	jmp    400fbe <phase_3+0x7b>
  400f83:	b8 c3 02 00 00       	mov    $0x2c3,%eax
  400f88:	eb 34                	jmp    400fbe <phase_3+0x7b>
  400f8a:	b8 00 01 00 00       	mov    $0x100,%eax
  400f8f:	eb 2d                	jmp    400fbe <phase_3+0x7b>
  400f91:	b8 85 01 00 00       	mov    $0x185,%eax
  400f96:	eb 26                	jmp    400fbe <phase_3+0x7b>
  400f98:	b8 ce 00 00 00       	mov    $0xce,%eax
  400f9d:	eb 1f                	jmp    400fbe <phase_3+0x7b>
  400f9f:	b8 aa 02 00 00       	mov    $0x2aa,%eax
  400fa4:	eb 18                	jmp    400fbe <phase_3+0x7b>
  400fa6:	b8 47 01 00 00       	mov    $0x147,%eax
  400fab:	eb 11                	jmp    400fbe <phase_3+0x7b>
  400fad:	e8 88 04 00 00       	callq  40143a <explode_bomb>
  400fb2:	b8 00 00 00 00       	mov    $0x0,%eax
  400fb7:	eb 05                	jmp    400fbe <phase_3+0x7b>
  400fb9:	b8 37 01 00 00       	mov    $0x137,%eax
  400fbe:	3b 44 24 0c          	cmp    0xc(%rsp),%eax
  400fc2:	74 05                	je     400fc9 <phase_3+0x86>
  400fc4:	e8 71 04 00 00       	callq  40143a <explode_bomb>
  400fc9:	48 83 c4 18          	add    $0x18,%rsp
  400fcd:	c3                   	retq   
```

断点调试提示有两个输入
```
(gdb) stepi
__isoc99_sscanf (s=0x603820 <input_strings+160> "1 2", format=0x4025cf "%d %d")
    at isoc99_sscanf.c:26
26	isoc99_sscanf.c: 没有那个文件或目录.
```

```
%rsp = (void *) 0x7fffffffdc50
```

首先尝试输入x1=1 x2=2
 
```
print *(int *) 0x7fffffffdc50 
(%rsp)=0
print *(int *) 0x7fffffffdc54 
(%rsp+4)=0

print *(int *) 0x7fffffffdc58 
(%rsp+8)=1  # 第一个输入
print *(int *) 0x7fffffffdc5c 
(%rsp+c)=2   # 第二个输入


```

```
jmpq   *0x402470(,%rax,8)  # *(8*rax+0x402470) 
```

当%rax=1，8*rax+0x402470=0x402478,*(0x402478)=0x400fb9

在程序中，跳转到

```
mov    $0x137,%eax
```
如果0xc(%rsp)=x2=0x137 就可以跳过炸弹，所以，最终只需输入

```
1 311
```

### (4) phase 4
phase 4的反汇编代码

```

000000000040100c <phase_4>:
  40100c:	48 83 ec 18          	sub    $0x18,%rsp
  401010:	48 8d 4c 24 0c       	lea    0xc(%rsp),%rcx               # (%rsp+0xc)->%rcx=x2
  401015:	48 8d 54 24 08       	lea    0x8(%rsp),%rdx               # (%rsp+0x8)->%rdx=x1
  40101a:	be cf 25 40 00       	mov    $0x4025cf,%esi
  40101f:	b8 00 00 00 00       	mov    $0x0,%eax
  401024:	e8 c7 fb ff ff       	callq  400bf0 <__isoc99_sscanf@plt> #%eax=scanff 匹配的参数个数 尝试输入 两个ax参数 三个参数 断点执行%eax均为 2 
  401029:	83 f8 02             	cmp    $0x2,%eax                    # %eax != 2 则跳转  有符号数比较  跳转后炸弹爆炸  因此参数个数必须为2
  40102c:	75 07                	jne    401035 <phase_4+0x29>
  40102e:	83 7c 24 08 0e       	cmpl   $0xe,0x8(%rsp)               # 0xe > (%rsp+0x8)=x1   则跳转  无符号数比较  所以x1<e
  401033:	76 05                	jbe    40103a <phase_4+0x2e>
  401035:	e8 00 04 00 00       	callq  40143a <explode_bomb>
  40103a:	ba 0e 00 00 00       	mov    $0xe,%edx                    # %edx=0xe
  40103f:	be 00 00 00 00       	mov    $0x0,%esi                    # %esi=0
  401044:	8b 7c 24 08          	mov    0x8(%rsp),%edi               # %edi=x1
  401048:	e8 81 ff ff ff       	callq  400fce <func4>
  40104d:	85 c0                	test   %eax,%eax                    # %eax和自己相与 !=0 则跳转到炸弹 所以%eax必须==0
  40104f:	75 07                	jne    401058 <phase_4+0x4c>
  401051:	83 7c 24 0c 00       	cmpl   $0x0,0xc(%rsp)               # (%rsp+0xc)=0 才能避开炸弹   x2=0
  401056:	74 05                	je     40105d <phase_4+0x51>
  401058:	e8 dd 03 00 00       	callq  40143a <explode_bomb>
  40105d:	48 83 c4 18          	add    $0x18,%rsp
  401061:	c3                   	retq   
```

```
0000000000400fce <func4>:  # %edx=0xe %esi=0 %edi=x1
  400fce:	48 83 ec 08          	sub    $0x8,%rsp
  400fd2:	89 d0                	mov    %edx,%eax  # %eax=%edx             第一次循环  %eax=0xe
  400fd4:	29 f0                	sub    %esi,%eax  # %eax-=%esi                        %eax=0xe
  400fd6:	89 c1                	mov    %eax,%ecx  # %ecx=%eax                         %ecx=0xe
  400fd8:	c1 e9 1f             	shr    $0x1f,%ecx # %ecx>>0x1f 逻辑右移               %ecx=0
  400fdb:	01 c8                	add    %ecx,%eax  # %eax=%eax+%ecx                    %eax=0xe
  400fdd:	d1 f8                	sar    %eax       # %eax>>1 算数右移                  %eax=0x7
  400fdf:	8d 0c 30             	lea    (%rax,%rsi,1),%ecx  #%ecx=%rax+%rsi            %ecx=0x7
  400fe2:	39 f9                	cmp    %edi,%ecx    # %edi=x1
  400fe4:	7e 0c                	jle    400ff2 <func4+0x24>  # %ecx<=x1  则跳转        7<=x1跳转  希望%eax!=0 那么x1
  400fe6:	8d 51 ff             	lea    -0x1(%rcx),%edx
  400fe9:	e8 e0 ff ff ff       	callq  400fce <func4>
  400fee:	01 c0                	add    %eax,%eax
  400ff0:	eb 15                	jmp    401007 <func4+0x39>
  400ff2:	b8 00 00 00 00       	mov    $0x0,%eax   # 希望这句执行 
  400ff7:	39 f9                	cmp    %edi,%ecx    # %ecx>=x1  则跳转 希望这次跳转执行 避免%eax被变成非0的数 因此x1=%edi 可取值为7  7>>1=3  7>>2=1 7>>3=0
  400ff9:	7d 0c                	jge    401007 <func4+0x39>
  400ffb:	8d 71 01             	lea    0x1(%rcx),%esi
  400ffe:	e8 cb ff ff ff       	callq  400fce <func4>
  401003:	8d 44 00 01          	lea    0x1(%rax,%rax,1),%eax # %eax=%rax+%rax+1  那么%eax一定!=0 希望这句不要执行
  401007:	48 83 c4 08          	add    $0x8,%rsp
  40100b:	c3                   	retq 
```
根据phase4 可以得知 
```
(%rsp+8)=x1  # 第一个输入，
(%rsp+c)=x2   # 第二个输入
```
根据分析可知x2=0,在func中分析可得x1=%edi 可取值为7  7>>1=3  7>>2=1 7>>3=0
所以取答案为
```
0 0
```
### (5) phase 5
phase 5的反汇编代码

```
0000000000401062 <phase_5>:
  401062:	53                   	push   %rbx
  401063:	48 83 ec 20          	sub    $0x20,%rsp
  401067:	48 89 fb             	mov    %rdi,%rbx
  40106a:	64 48 8b 04 25 28 00 	mov    %fs:0x28,%rax
  401071:	00 00 
  401073:	48 89 44 24 18       	mov    %rax,0x18(%rsp)
  401078:	31 c0                	xor    %eax,%eax               #将%eax 清零  
  40107a:	e8 9c 02 00 00       	callq  40131b <string_length>
  40107f:	83 f8 06             	cmp    $0x6,%eax               #%eax=输入字符串的长度 因此必须输入连续的6个字符 x1x2x3x4x5x6
  401082:	74 4e                	je     4010d2 <phase_5+0x70>   #%eax=6 跳转
  401084:	e8 b1 03 00 00       	callq  40143a <explode_bomb>
  401089:	eb 47                	jmp    4010d2 <phase_5+0x70>
  40108b:	0f b6 0c 03          	movzbl (%rbx,%rax,1),%ecx     # (%rbx)=输入6个字符的第一个字符的地址 带0位扩展的移动 (%rbx+%rax)  --> 一个单字节数字  将其转换为4字节的保存在%ecx 中 同时将高位补0
  40108f:	88 0c 24             	mov    %cl,(%rsp)             # (%rsp)=cx 的最低字节 =一个字符的ASCII码
  401092:	48 8b 14 24          	mov    (%rsp),%rdx            # %rdx==cx 的最低字节
  401096:	83 e2 0f             	and    $0xf,%edx              # %edx==cx 的最低4位
  #(gdb) x/s 0x4024b0   0x4024b0 <array.3449>:	"maduiersnfotvbylSo you think you can stop the bomb with ctrl-c, do you?" 根据该字符串中的偏移量找到flyers 的位置计算离0x4024b0 的偏移量
  # 该偏移量就是我们需要的 也就是说该偏移量对应输入6个字符的低4位 找到低4位符合要求的字符即可
  # *(0x4024b0+0x9)=f--i  *(0x4024b0+0xf)=l--o *(0x4024b0+0xe)=y--n  *(0x4024b0+0x5)=e--e  *(0x4024b0+0x6)=y--f *(0x4024b0+0x7)=e --g  
  # ionefg
  401099:	0f b6 92 b0 24 40 00 	movzbl 0x4024b0(%rdx),%edx    # (0x4024b0+cx 的最低4位)  某字符的int数值  比如'1' %edx=1
  4010a0:	88 54 04 10          	mov    %dl,0x10(%rsp,%rax,1)  # (%rsp+%rax+16)=%dl =某字符的int数值
  4010a4:	48 83 c0 01          	add    $0x1,%rax
  4010a8:	48 83 f8 06          	cmp    $0x6,%rax
  4010ac:	75 dd                	jne    40108b <phase_5+0x29>  $ rax=6 终止循环
  4010ae:	c6 44 24 16 00       	movb   $0x0,0x16(%rsp)    # movb 字节传送  (%rsp+%rax)=0
  4010b3:	be 5e 24 40 00       	mov    $0x40245e,%esi     # 输入 x/s 0x40245e 0x40245e:	"flyers"
  4010b8:	48 8d 7c 24 10       	lea    0x10(%rsp),%rdi    # 将之前存储的6个字节作为strings_not_equal 的输入
  4010bd:	e8 76 02 00 00       	callq  401338 <strings_not_equal>
  4010c2:	85 c0                	test   %eax,%eax          # %eax=0 跳转 
  4010c4:	74 13                	je     4010d9 <phase_5+0x77>
  4010c6:	e8 6f 03 00 00       	callq  40143a <explode_bomb>
  4010cb:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  4010d0:	eb 07                	jmp    4010d9 <phase_5+0x77>
  4010d2:	b8 00 00 00 00       	mov    $0x0,%eax            # %eax=0
  4010d7:	eb b2                	jmp    40108b <phase_5+0x29>
  4010d9:	48 8b 44 24 18       	mov    0x18(%rsp),%rax
  4010de:	64 48 33 04 25 28 00 	xor    %fs:0x28,%rax
  4010e5:	00 00 
  4010e7:	74 05                	je     4010ee <phase_5+0x8c>
  4010e9:	e8 42 fa ff ff       	callq  400b30 <__stack_chk_fail@plt>  #检查数组是否越界
  4010ee:	48 83 c4 20          	add    $0x20,%rsp
  4010f2:	5b                   	pop    %rbx
  4010f3:	c3                   	retq 
```

### (6) phase 6
phase 6的反汇编代码

```
00000000004010f4 <phase_6>:
  4010f4:	41 56                	push   %r14
  4010f6:	41 55                	push   %r13
  4010f8:	41 54                	push   %r12
  4010fa:	55                   	push   %rbp
  4010fb:	53                   	push   %rbx
  4010fc:	48 83 ec 50          	sub    $0x50,%rsp
  401100:	49 89 e5             	mov    %rsp,%r13
  401103:	48 89 e6             	mov    %rsp,%rsi
  401106:	e8 51 03 00 00       	callq  40145c <read_six_numbers>    #输入6个数 x1 x2 x3 x4 x5 x6
  40110b:	49 89 e6             	mov    %rsp,%r14
  40110e:	41 bc 00 00 00 00    	mov    $0x0,%r12d                   # %r12d=0
  401114:	4c 89 ed             	mov    %r13,%rbp                    # %r13=xn
  401117:	41 8b 45 00          	mov    0x0(%r13),%eax
  40111b:	83 e8 01             	sub    $0x1,%eax
  40111e:	83 f8 05             	cmp    $0x5,%eax                    # %eax=xn-1  xn-1<=5则跳转 xn<=6
  401121:	76 05                	jbe    401128 <phase_6+0x34>        #无符号数比较 所以xn-1>=0 1<=xn<=  
  401123:	e8 12 03 00 00       	callq  40143a <explode_bomb>
  401128:	41 83 c4 01          	add    $0x1,%r12d
  40112c:	41 83 fc 06          	cmp    $0x6,%r12d                    # %r12d的低32位=6跳转
  401130:	74 21                	je     401153 <phase_6+0x5f>
  401132:	44 89 e3             	mov    %r12d,%ebx                   # %ebx=%r12d=1

  401135:	48 63 c3             	movslq %ebx,%rax                    # movslq 将一个双字符号扩展后送到一个四字地址中 %rax=%ebx=1
  401138:	8b 04 84             	mov    (%rsp,%rax,4),%eax           # %eax =(%rsp+4*%rax)=(%rsp+4*%rax) xn 
  40113b:	39 45 00             	cmp    %eax,0x0(%rbp)               # %eax=xn 和(%rbp)=xi 不相等则跳转
  40113e:	75 05                	jne    401145 <phase_6+0x51>
  401140:	e8 f5 02 00 00       	callq  40143a <explode_bomb>
  401145:	83 c3 01             	add    $0x1,%ebx                    # %ebx=%r12d=2
  401148:	83 fb 05             	cmp    $0x5,%ebx                    # 遍历 xi--x6 和xi作比较 都应该和x1不等
  40114b:	7e e8                	jle    401135 <phase_6+0x41>
  40114d:	49 83 c5 04          	add    $0x4,%r13                    # (%r13)=xi  (%r13=%r13+4)=x(i+1)
  401151:	eb c1                	jmp    401114 <phase_6+0x20>        # 两个循环 遍历x1--x6 两两都不想等则跳出循环 同时需要满足都小于 6等于

  401153:	48 8d 74 24 18       	lea    0x18(%rsp),%rsi              # %rsi=%rsi+24  --对应6个数字
  401158:	4c 89 f0             	mov    %r14,%rax                    # (%rax)=x1
  40115b:	b9 07 00 00 00       	mov    $0x7,%ecx                    # %ecx=7
  401160:	89 ca                	mov    %ecx,%edx                    # %edx=7
  401162:	2b 10                	sub    (%rax),%edx                  # %edx=7-x1
  401164:	89 10                	mov    %edx,(%rax)                  # x1=7-x1
  401166:	48 83 c0 04          	add    $0x4,%rax                    # %rax=rax+4
  40116a:	48 39 f0             	cmp    %rsi,%rax                    # 遍历6个数字 将xi := 7-xi
  40116d:	75 f1                	jne    401160 <phase_6+0x6c>


  40116f:	be 00 00 00 00       	mov    $0x0,%esi                    # %esi=0
  401174:	eb 21                	jmp    401197 <phase_6+0xa3>         
  401176:	48 8b 52 08          	mov    0x8(%rdx),%rdx                %rdx=(%rdx+8)
  # %rdx =(%rdx +8)  (0x6032d8)=0x6032e0 (0x6032e8)=0x6032f0 (0x6032f8)=0x603300 (0x603308)=0x603310 (0x603318)=0x603320 (0x603328)=0x603330
  #  循环结束%rdx    (0x6032d0)==0x14c   (0x6032e0)=0xa8     (0x6032f0)=0x39c    (0x603300)=0x2b3    (0x603310)=0x1dd    (0x603320)=0x1bb    (0x603330)=0
  #   xn                    6                  5                     4                  3                   2                    1
  # 这里循环 (7-xn)-1次 也就是说xn=6 循环 0 次 那么最终%rdx=0x6032d0  不同的循环次数对应的不同的%rdx
  # 也就是说 输入的1--6 6个不同的数以一定的顺序输入 最终导致 (%rsp+0x20)  --  (%rsp+0x40) 6个数保存其对应的%rdx 要求是递减数列 
  #  所以找到一个递减值数列 0x39c 0x2b3 0x1dd 0x1bb  0x14c 0xa8 对应的输入 4 3 2 1 6 5 即为答案
  40117a:	83 c0 01             	add    $0x1,%eax                    # %eax =%eax +1
  40117d:	39 c8                	cmp    %ecx,%eax                    # %ecx=(%rsp+%rsi) =7-x1
  40117f:	75 f5                	jne    401176 <phase_6+0x82>        

  401181:	eb 05                	jmp    401188 <phase_6+0x94>
  401183:	ba d0 32 60 00       	mov    $0x6032d0,%edx               # %rdx=0x6032d0  7-x1<=1 那么执行这一句 否则  %rdx=0x603330
  401188:	48 89 54 74 20       	mov    %rdx,0x20(%rsp,%rsi,2)       # (%rsp+%rsi+0x20)=%rdx 这里%rsi 代表输入的第几个数 %rsi/4=n
  40118d:	48 83 c6 04          	add    $0x4,%rsi
  401191:	48 83 fe 18          	cmp    $0x18,%rsi  # 循环执行6次
  401195:	74 14                	je     4011ab <phase_6+0xb7>
  401197:	8b 0c 34             	mov    (%rsp,%rsi,1),%ecx           # %ecx=(%rsp+%rsi) =7-x1
  40119a:	83 f9 01             	cmp    $0x1,%ecx                    
  40119d:	7e e4                	jle    401183 <phase_6+0x8f>
  40119f:	b8 01 00 00 00       	mov    $0x1,%eax
  4011a4:	ba d0 32 60 00       	mov    $0x6032d0,%edx
  4011a9:	eb cb                	jmp    401176 <phase_6+0x82>


  4011ab:	48 8b 5c 24 20       	mov    0x20(%rsp),%rbx              # %rbx=(%rsp+0x20)
  4011b0:	48 8d 44 24 28       	lea    0x28(%rsp),%rax              # %rax=%rsp+0x28
  4011b5:	48 8d 74 24 50       	lea    0x50(%rsp),%rsi              # %rsi=%rsp+0x50
  4011ba:	48 89 d9             	mov    %rbx,%rcx                    # %rcx=(%rsp+0x20)

  4011bd:	48 8b 10             	mov    (%rax),%rdx                  # %rdx=(%rsp+0x28)
  4011c0:	48 89 51 08          	mov    %rdx,0x8(%rcx)               # ((%rsp+0x20)+8)=(%rsp+0x28)
  4011c4:	48 83 c0 08          	add    $0x8,%rax                    # %rax =%rsp+0x30
  4011c8:	48 39 f0             	cmp    %rsi,%rax                  
  4011cb:	74 05                	je     4011d2 <phase_6+0xde>
  4011cd:	48 89 d1             	mov    %rdx,%rcx                   # %rcx=(%rsp+0x28)
  4011d0:	eb eb                	jmp    4011bd <phase_6+0xc9>        # 循环6次

  4011d2:	48 c7 42 08 00 00 00 	movq   $0x0,0x8(%rdx)              # (%rdx+8)=0  # (%rsp+0x20)  --  (%rsp+0x40)要求是递减数列
  4011d9:	00 
  4011da:	bd 05 00 00 00       	mov    $0x5,%ebp
  4011df:	48 8b 43 08          	mov    0x8(%rbx),%rax  # %rax=(%rbx+8)=((%rsp+0x20)+8)=0x6032e0
  4011e3:	8b 00                	mov    (%rax),%eax     # %eax=((%rbx+8))
  4011e5:	39 03                	cmp    %eax,(%rbx)     # (%rbx+8) <= (%rbx) 则跳转
  4011e7:	7d 05                	jge    4011ee <phase_6+0xfa>
  4011e9:	e8 4c 02 00 00       	callq  40143a <explode_bomb>
  4011ee:	48 8b 5b 08          	mov    0x8(%rbx),%rbx  # %rbx=(%rbx+8)=((%rsp+0x20)+8)
  4011f2:	83 ed 01             	sub    $0x1,%ebp
  4011f5:	75 e8                	jne    4011df <phase_6+0xeb>
  4011f7:	48 83 c4 50          	add    $0x50,%rsp
  4011fb:	5b                   	pop    %rbx
  4011fc:	5d                   	pop    %rbp
  4011fd:	41 5c                	pop    %r12
  4011ff:	41 5d                	pop    %r13
  401201:	41 5e                	pop    %r14
  401203:	c3                   	retq   
```

