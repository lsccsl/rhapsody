1.如何在linux下面编译
  tar xvzf rhapsody_2007_08_23.tar.gz
　然后进入解压后的目录,直接运行make即可
  生成的lib库在 /bin目录下面 /test目录下面为测试代码  

2.window下如何编译
 用vs 2003打开Rhapsody.sln即可运行编译

3.其它
 test_arm44b0 为uclinux+arm44b0平台的测试程序
 test_linux 为linux下面的测试程序
 test_win32_release.exe 与 test_win32_debug.exe为window下面的测试程序

 /utility/myconfig.h　这个文件中这个宏#define MEM_LEAK_CHECK //内存泄漏检测

4.编译不过的问题。
  由于有些源码是在windows下编辑的,在一些早期版本的gcc编译器(arm-elf-gcc 2.93)可能格式认不到
请用 dos2unix进行格式转换(dos2unix xxx.c)，即可正常编译

have fun!

/*change log*/
2010 9.25
  修改消息队列mymsgque通知机制,linux下面是事件是脉冲式的,有时会通知不到读取进程
  修改linux版本mylistener通知管道的读取,全改成非阻塞的.
  其它一些bug fix

2009 2.6
  修改b树平衡操作过程个,个别assert语句写得不合理，导致debug模式下运动会出错。对release版本没有影响。

2008 9.24
  优化b树提交时的代码,缩短提交时间.
  mutex在同一线程占用的情况下，补充计数功能，允许同一线程在不解锁的情况下多次占有锁.
  修改linux下面线程退出时内存泄漏的问题，在uclinux下面测试,发现不pthread_join会导致内存泄漏，why?总之把它改了就是.

2008 4.11
  添加基于b树索引算法的数据库代码,详见storage/目录下面的 btree.c与pager.c
  pager.c：外存页缓存管理模块，负责页缓存管理，jouranl备份日志，保证在意外断电的情况下，数据库中数据的完整性.
           以及外存空闲页的管理
  btree.c：基于于b树的索引算法,提供键值与用户数据对的映射存储
  目前高层的数据库接口即xdb.h里的函数尚未封装。但b树索引算法在windows平台上经过详细的测试了.

2007 10.17
 添加B树
 修正定时器的一个bug(在回调函数里做添加定时器会死锁)

2007 11.14
 修改myevent_pipe.c使之支持多个线程等待一个信号量
 修改mylisterner_linux.c在回调函数里监听fd会死锁的问题


 
　