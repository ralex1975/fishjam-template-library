#ifndef FTL_HOOK_API_H
#define FTL_HOOK_API_H
#pragma once

#ifndef FTL_BASE_H
#  error ftlHookAPI.h requires ftlbase.h to be included first
#endif

/******************************************************************************************************
* 有些情况下，必须打破进程的界限，访问另一个进程的地址空间，这些情况包括：
* 1.当你想要为另一个进程创建的窗口建立子类时 -- SetWindowLongPtr(hwnd,GWLP_WNDPROC,..) 改变窗口的内存块中的窗口过程地址。
* 2.当你需要调试帮助时（例如，当你需要确定另一个进程正在使用哪个DLL时）。
* 3.当你想要挂接其他进程时。
*   但需要将子类过程的代码放入目的进程的地址空间－－将DLL插入目的进程的地址空间。
*
* 注意：
*   1.如果注入的DLL生成了任何新的线程，请确保在程序退出时，退出线程。
*   2.远程注入后，需要通过进程间通信的方式交互 -- 如 FileMapping、WM_COPYDATA、Clipboard、#pragma data_seg 等
*     #pragma data_seg (".shared")
*       UINT    WM_HOOKEX = 0;
*       HHOOK   g_hHook = 0;
*       XXXX    g_MyDATA = yyy; //初始化
*     #pragma data_seg ()
*     #pragma comment(linker,"/SECTION:.shared,RWS")  //设置共享节
*   3.通常都在DLL中的 DllMain 中响应 DLL_PROCESS_ATTACH 来处理 
*       可能需要调用 DisableThreadLibraryCalls 来禁止本DLL 接收线程通知消息；
*   4.通过调用 RegisterWindowMessage 注册消息并进行共享，则 Exe 和 注入的DLL可以交互。
*       DLL 的 DllMain 中：if(WM_HOOKEX == 0) WM_HOOKEX = ::RegisterWindowMessage(TEXT("WM_HOOKEX_RK"));
*       Exe 中：
*
* 常用方法
*   a. 使用注册表来插入DLL
*      HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Windows 的 AppInit_DLLs ，包含一个DLL文件名或者一组DLL文件名
*      (用空格或逗号隔开)。列出的第一个DLL文件名可以包含一个路径 －－ 最好将你的DLL放入 Windows 的系统目录中，这样就不必设定路径。
*      系统启动时，有 User32.dll 依次调用DLL中的 DllMain(DLL_PROCESS_ATTACH)
*      不足：只能在系统重启后，加载到所有使用User32.dll 的 GUI 程序中；不支持Win98
*
*   b.使用 Windows 挂钩来插入DLL。有针对单个线程的Hook和针对整个系统的Hook(具体是哪个钩子无所谓，但一般好像是 WH_GETMESSAGE/WH_CALLWNDPROC ?)
*       系统Hook： HHOOK hHook = SetWindowsHookEx(WH_GETMESSAGE,GetMsgProc,hInstDll,0);
*       线程Hook： g_hHook = SetWindowsHookEx( WH_CALLWNDPROC,HookProc,hInstDll, GetWindowThreadProcessId(hWndParam,NULL));
*     当系统插入或者映射包含挂钩过滤器函数的DLL时，整个DLL均被映射，而不只是挂钩过滤器函数被映射。
*     这意味着DLL中包含的任何一个函数或所有函数现在都存在，并且可以从目的进程的环境下运行的线程中调用。
*     Sample：跨进程获取桌面 ListView 中各个项目的位置。
*
*   c.使用远程线程来插入DLL -- 具有更大的灵活性(DLL 中执行 DLL_PROCESS_ATTACH 通知的 DllMain 函数 )
*     CreateRemoteThread(其参数 pfnStartAddr指明线程函数的内存地址,其纤程函数的代码不能位于自己进程的地址空间中)，
*     需要该线程调用 LoadLibrary 函数来加载我们的DLL。创建一个新线程，并且使线程函数的地址成为 LoadLibraryA 或LoadLibraryW函数的地址。
*     TODO：？使用 VirtualQueryEx 函数把存放指令的页面的权限更改为可读可写可执行，再改写其内容，从而修改正在运行的程序 
*     1).使用 VirtualAllocEx 函数，分配远程进程的地址空间中的内存。
*     2).使用 WriteProcessMemory 函数，将DLL的路径名拷贝到第一个步骤中已经分配的内存中。
*     3).使用 GetProcAddress 函数，获取 LoadLibraryA 或 LoadLibraryW 函数的实地址（在Kernel32.dll中）
*     4).使用 CreateRemoteThread 函数，在远程进程中创建一个线程，它调用正确的 LoadLibrary 函数，为它传递第一个步骤中分配的内存的地址。
*        这时,DLL已经被插入远程进程的地址空间中，同时DLL的 DllMain 函数接收到一个 DLL_PROCESS_ATTACH 通知，并且能够执行需要的代码。
*        当 DllMain 函数返回时，远程线程从它对 LoadLibrary 的调用返回,使远程线程终止运行。
*        现在远程进程拥有第一个步骤中分配的内存块，而DLL则仍然保留在它的地址空间中。
*     5).调用GetExitCodeThread获取远程线程的退出码 -- 即LoadLibrary函数的返回值，表明映射的DLL的基址，可以保留并用于以后释放清除操作：
*     6).使用 VirtualFreeEx 函数，释放第一个步骤中分配的内存。
*     7).使用 GetProcAddress 函数，获得 FreeLibrary 函数的实地址。
*     8).使用 CreateRemoteThread 函数，在远程进程中创建一个线程，它调用 FreeLibrary 函数，传递远程DLL的HINSTANCE。
*
*   d.使用特洛伊DLL来插入DLL
*     取代你知道进程将要加载的DLL(自己的DLL取名为需要替换的DLL，将需要替换的DLL换成别的名字，使用 "函数转发器" 输出原始DLL的符号)
*     --不具备版本升级能力
*     或改变需要插入DLL的输入节
*   
*   e.将DLL作为调试程序来插入
*     调试程序能够对被调试的进程执行特殊的操作。当被调试进程加载时，在被调试进程的地址空间已经作好准备，
*     但是被调试进程的主线程尚未执行任何代码之前，系统将自动将这个情况通知调试程序。这时，调试程序可以强制
*     将某些代码插入被调试进程的地址空间中(比如使用 WriteProcessMemory 函数来插入)，然后使被调试进程的主线程执行该代码。
*     要求对被调试线程的 CONTEXT 结构进行操作，意味着必须编写特定CPU的代码
*   
*   f. 用 CreateProcess 插入代码 -- 需要自己的进程生成了想插入代码的新进程。
*     1).使你的进程生成暂停运行的子进程;
*     2).从.exe 模块的头文件中检索主线程的起始内存地址。
*     3).将机器指令保存在该内存地址中。
*     4).将某些硬编码的机器指令强制放入该地址中。这些指令应该调用 LoadLibrary 函数来加载DLL。
*     5).继续运行子进程的主线程，使该代码得以执行。
*     6).将原始指令重新放入起始地址。
*     7).让进程继续从起始地址开始执行，就像没有发生任何事情一样
*   
*   g. 使用 WriteProcessMemory 在远程进程中写入执行代码，并使用 CreateRemoteThread 执行。
*     1).使用 VirtualAllocEx 在远程进程中分配 插入代码 和 参数(INJDATA)的内存；
*     2).使用 WriteProcessMemory 写入"插入代码"和“参数”的拷贝；
*     3).使用 CreateRemoteThread 开始执行 插入代码；
*     4).使用 WaitForSingleObject 等待线程结束；
*     5).使用 ReadProcessMemory 或 GetExitCodeThread 获取远程线程的执行结果；
*     6).示范相关资源（VirtualFreeEx、CloseHandle）。
*     优点：
*       可以只有一个EXE(不需要DLL文件)；
*     缺点：
*       非常复杂并且容易出错。
*       远程方法只能执行 kernel32.dll 和 user32.dll 中的方法；
*       不能使用 /GZ 编译选项（Debug中的缺省值） -- 会在函数开头增加调试用的代码，在远程进程中不存在。
*       线程函数必须是 static 的(被直接调用)，或者禁止 incremental linking(通过 JMP 调用，远程进程中地址不同)
*       线程函数中的本地变量必须在一个 Page 宽度内（4K）--ESP不再直接变化，而调用函数(远程进程中地址不同)进行调整 
*       switch 中不能使用过多（超过3个）的case，最好换成 if..else if -- 超过3个时会在编译时创建使用绝对地址的跳转地址表，
*         通过偏移量跳转（提高性能），但跳转表在远程进程中不存在。
*         
*     注意：使用 VirtualAllocEx 时必须指定 PAGE_EXECUTE_READWRITE 属性 -- 能在写入代码的地方执行
*
*
******************************************************************************************************/

/******************************************************************************************************
* Hook API
*   实现方式
*     1.改引入表式 -- 易于实现，OpenProcess + WriteProcessMemory 改写函数地址
*     2.陷阱式
*   
* 已有的库
*   1.EasyHook：http://easyhook.codeplex.com/
*               https://code.google.com/p/easyhook-continuing-detours
*       使用的例子:http://blog.csdn.net/baggiowangyu/article/details/7675098
*     支持驱动中Hook，支持X64注入，且完全免费，License为GPL
*     提供了两种模式的注入管理：托管代码 和 非托管代码，通过Inline Hook 方式实现
*     示例:
*       TRACED_HOOK_HANDLE hHookCreateFileW = { NULL };  
*       ULONG HookCreateFileW_ACLEntries[1] = {0}; 
*       LhInstallHook(OriginalCreateFileW, MyCreateFileW, NULL, &hHookCreateFileW);
*       LhSetExclusiveACL(HookCreateFileW_ACLEntries, 1, hHookCreateFileW);  
*       调用函数
*       LhUninstallAllHooks();
*       LhUninstallHook(&hHookCreateFileW);  
*       delete hHookCreateFileW;  
*       LhWaitForPendingRemovals(); //等待内存释放
* 
* 
*   2.Detours：http://research.microsoft.com/en-us/projects/detours
*     微软的开源研究库，免费版本不能用于商业，商业版本大约6000RMB。支持x64和IA64等64位平台
*     编译：nmake，TODO: Debug/Release？ 可能需要更改 samples\common.mak 文件，去掉其中的 /nologo 
*     原理：在汇编层改变目标API出口和入口的一些汇编指令
*       几部分：
*         Target函数：要拦截的函数，通常为Windows的API
*         Trampoline函数(蹦床,原始被替代的代码 + JMP到Target)：Target函数的部分复制品。
*           因为Detours将会改写Target函数，所以先把Target函数的前5个字节复制保存好，
*           一方面仍然保存Target函数的过程调用语义，另一方面便于以后的恢复。然后是一个无条件转移
*         Detour函数：用来替代Target函数的截获函数
*     Detours同样提供了DLL注入的方式，它可编辑任何DLL或EXE导入表的功能，达到向存在的二进制代码中添加任意数据节表的目的
*     有效负荷(Payloads) -- 可以对Win32二进制文件添加、卸载数据节表(.detours Section)以及编辑DLL导入表
*     实现注意：
*       1.需要将DLL复制到目标进程目录 或 System32(SysWOW64 -- 64位上的32位程序时)
*       2.通过 #pragma data_seg("MyShare") + #pragma comment(linker,"/SECTION:MyShare,RWS") 将特定变量加入共享数据段
*       3.DLL_PROCESS_ATTACH + DLL_THREAD_ATTACH 时进行Hook？ 
*         DLL_PROCESS_DETACH + DLL_THREAD_DETACH 时进行UnHook？
*       4.是否需要该文件？ detoured.dll 是一个标志，帮助微软的开发人员和工具判断某个进程是否已经被detours拦截
*       5.32 <==> 64 互相插入时，会生成辅助进程(HelperProcess)？
*       6.可以静态或动态地创建：区别？ComicViewer中不能拦截视频的BitBlt？
*       A1.一定要枚举线程并调用DetourUpdateThread函数。否则可能出现很低几率的崩溃问题，这种问题很难被检查出来。
*       A2.如果拦截函数在DLL中，那么绝大多数情况下不能在Unhook之后卸载这个DLL，或者卸载存在造成崩溃的危险。
*     使用方式：
*       1.普通：DetourTransactionBegin => DetourUpdateThread => DetourAttach/DetourDetach => DetourTransactionCommit
*       2.类成员函数 -- 定义CXxxHook类(注意：尚未测试 -- http://wenku.baidu.com/view/7b441cb665ce05087632133b.html)
*         a.定义static的函数指针： static int (CXxxHook::* TrueFun)(xxxx) = &CTrueXxx::Fun;
*         b.定义 FilterFun 的成员函数，其中可以通过 (this->*TrueFun)(xxx) 的方式调用原来的实现
*         c.DetourAttach(&(PVOID&)CXxxHook::TrueFun, (PVOID)(&(PVOID&)CXxxHook::FilterFun));
*     主要的API
*       DetourCreateProcessWithDllEx -- 在启动指定进程的时候注入DLL(该DLL最少要有一个导出函数)
*       DetourIsHelperProcess -- 判断当前是否在辅助进程里,通过在DllMain中，该函数返回TRUE时，直接 return TRUE;
*       DetourRestoreAfterWith -- 恢复初始状态(在 DllMain::PROCESS_ATTACH 时调用？)
*       DetourUpdateThread -- 
*       DetourFindFunction -- 动态找到函数指针的真实地址给 TrueXxx 赋值?
*     工具:
*       SetDll.exe -- 可以在Exe文件中增加对指定DLL的引用(加入.detours节，会更改可执行文件),
*         注意DLL需要有序号为"1" 的导出函数(Makefile 中有: /export:DetourFinishHelperProcess,@1,NONAME )
*         TODO:def 文件中如何设置？
*       WithDll.exe -- 在注入指定DLL的情况下创建并运行指定进程(纯内存操作，可执行文件不受影响)
*     TODO:
*       是否需要调用 DisableThreadLibraryCalls ?
*   3.Deviare(不支持C/C++?  http://www.nektra.com/products/deviare-api-hook-windows/)
*   4.MinHook
* 
* TODO(FUAPIHook.h 中已有) -- http://wenku.baidu.com/view/5ae307f04693daef5ef73d48.html
*   Win64的：http://blog.csdn.net/zzw315/article/details/4102488
******************************************************************************************************/

/******************************************************************************************************
* 应用级的全局 Hook API：SetWindowsHookEx + Detours -- TODO: 单独的 ftlHookAPI(DUI 中 HookAPI 源码，比较)
*   缺点（？）：
*     1.需要管理员权限
*     2.不稳定，依赖于其他WindowsHook的实现(Hooks 是一个合作机制，没有任何保证，其他的实现不调用 CallNextHookEx 就不能进行处理)？
*     3.可能被反病毒软件提醒？
*   注意：对应的有 驱动级的全局 Hook API(参见 FDriverHookAPI.h )
*   钩子机制 -- 优先处理特定消息、事件的代码段，每一类Hook都是一个链表(钩子链，Hook Chains)，
*     每一项都是一个应用程序定义的钩子处理函数，每一个处理函数都有责任通过调用 CallNextHookEx 保证之前的过滤函数被调用，
*     最近安装的钩子放在链的开始，优先获得控制权。
*     
*   SetWindowsHookEx
*     系统钩子() -- 必须在DLL中，系统自动将该DLL注入受影响的进程地址空间
*     线程钩子 -- 
*    
*     IAT(对于加壳的软件无效) ? 
*     JMP 方式 -- 可以对付加壳软件   
*  
*   远程注入：
*     1.Ring3: CreateRemoteThread + LoadLibrary
******************************************************************************************************/

namespace FTL
{
}
#endif //FTL_HOOK_API_H

#ifndef USE_EXPORT
#  include "ftlHookAPI.hpp"
#endif 