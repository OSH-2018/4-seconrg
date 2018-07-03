# Meltdown漏洞和spectre漏洞报告 
陆万航 PB16110766 使用迟交机会
## 实验环境：
* 参考了https://github.com/paboldin/meltdown-exploit 上的melt.sh，可以达到获取内核信息的效果
* 由于虚拟机（linux18.04）会受到宿主机（windows 10，已安装meltdown）的影响，经反复尝试仍未能成功关闭meltdown补丁，所以借用了同学的ubuntu 16.04，内核版本Linux4.10.0-28，未安装meltdown补丁。
## Meltdown漏洞原理：
* CPU的乱序执行
在计算机组成原理的课程中提到过，为了能够最大限度地利用资源，不让其空闲，所以常常采用乱序执行的策略来提高CPU的利用率，提高其工作效率，即指令实际执行的顺序和程序员编写出的程序有所不同，这也是meltdown漏洞产生的根源。在遇到异常指令时，在CPU检测到异常并跳转到指定地址执行中断服务程序前，出现中断的指令后面的几条指令实际上也已经被执行，只是在之后的执行过程中，这些数值都被舍弃了。例如在MIPS指令集体系中，产生的异常直到MEM段才被提交，即CPU在MEM段才能检测到异常，此时接下来的几条与异常无关的指令均被执行。
* Cache和meltdown
为了提高访问内存的速度，现代计算机常采用cache策略，即将使用率较高的内存内容存放在cache中来提高访存速度，常见的cache替换算法有FIFO、LRU、LFU等，但是无论是那种算法，最近被使用过的块（部分教材中称页）最容易被写进cache，当异常产生时，由于CPU的乱序执行，导致出现异常的代码后面的几条代码也被执行，虽然事后这些代码执行的结果会被取消，但是对应的写入cache的内容却不会被修改。所以，攻击者可以在这些代码中对系统内核或者其他内存空间进行访问，从而让该页被cache，通过检测每一页的读取速度，即采用Flush+ Reload策略，便可以确定被cache的页的地址，从而确定页的内容，该方法一次可以读取一个字节，通过反复循环执行即可窃取整个内核信息。
（https://blog.csdn.net/y734564892/article/details/79019084）
 ![image](https://github.com/OSH-2018/4-seconrg/blob/master/download.png)
上图说明了利用CPU乱序执行来实现攻击的大致过程，在该流程图结束后，通过对内存的每一块进行访问并检测访问时间，最短的块即为被写入cache的块，从而可以确定m的值，页就获得了非法地址a中的一个字节的数据m。

## 实验步骤：
编写了可以获取内核的程序，主要分为以下几个部分：
* 主函数main：
根据命令行传入的参数，来攻击对应的内存位置，并获取对应的信息
* attack函数：
攻击程序的主体，通过使用内联汇编来实现上图中对于代码的攻击。
* 时间获取函数readtime：
通过记录读内存前的时间和读内存后的时间，将两者相减后比较，得到最小值，即确定上图中m的值。

## 实验结果：
通过实验，成功窃取了内核信息，如下图所示：
![image](https://github.com/OSH-2018/4-seconrg/blob/master/result1.jpg)
![image](https://github.com/OSH-2018/4-seconrg/blob/master/result2.jpg)
 
 
## 实验分析：
实验中采用的meltdown攻击手段，实际上是利用的CPU执行时提高效率的算法，即Tomasulo算法，所以严格来讲，该漏洞并非操作系统层面的漏洞，而是属于CPU设计上的缺陷，所以在纠正上，最简单的方法是不采用乱序执行的方法，让CPU顺序执行汇编指令，但这种做法会严重降低CPU的执行效率；第二种方法是改变cache的替换策略，即异常处理的时候flush掉cache，但是这么做会使cache的效率降低，因此代价较大。所以目前来讲，从硬件层面来解决meltdown漏洞难度较大，不太现实，因此目前针对meltdown的补丁都是在软件层面来防止meltdown攻击。
Meltdown和Spectre漏洞从本质上体现了效率和安全之间的矛盾，所以，长远的解决方案是更新指令集的架构，以包括对于处理器的安全属性的明确指导，并且，CPU的实现也必须同时更新。
本次实验难度较低，网上有较多代码和介绍，但详细阅读https://meltdownattack.com/ 上的两篇论文所花费的时间较多，因此实验整体耗费时间较多。

## 关于spectre
Spectre和Meltdown漏洞的相似度很高，但仍然存在着一些不同之处，通过阅读相关的论文，对于Specture漏洞也有所了解：
### Spectre 的原理：
#### CPU的分支预测
为了提高CPU的效率，现代高速处理器采用分支预测的手段，即通过预测未来分支发生/不发生来防止分支指令造成CPU的利用率下降。
#### Spectre攻击方式
与Meltdown类似，Spectre的原理是，当CPU发现分支预测错误时会丢弃分支执行的结果，并恢复CPU的状态，但是不会恢复一些其他微体系结构的状态（比如cache），利用这一点可以窃取非法地址中的数据，从而获得用户名、密码等关键数据。
在具体实现过程上，spectre和meltdown类似，都是利用临时指令（transient instructions），让CPU执行一些原本不会被执行的指令。 但是具体操作上，spectre比meltdown要灵活，攻击者通过训练分支预测器来使CPU总是进行错误的分支预测，从而执行临时指令，改变对应的cache，并由此窃取对应的数据。 
#### 和Meltdown的对比：
* 相同点：
均利用了CPU的漏洞来进行旁路攻击，通过执行临时指令来改变cache，从而窃取信息。
* 不同点：
Spectre利用的是分支预测、而meltdown利用的是乱序执行，通过异常来执行临时指令。
Spectre漏洞在ARM、AMD等处理器上也存在，而meltdown主要是在intel处理器上存在。
Spectre的攻击手段较为灵活，相较于meltdown难以防御，目前针对meltdown的漏洞补丁仍无法防御spectre。



参考资料：
https://github.com/IAIK/meltdown 
https://blog.csdn.net/qq_25827741/article/details/78994970
https://github.com/paboldin/meltdown-exploit
https://meltdownattack.com/


