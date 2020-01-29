#include <windows.h>
#include <stdio.h>

//此处显式加载dll函数将wav格式转换为可支持东进语音卡的pcm格式
//要求原数据格式为8k否则可能会出现失真情况 具体参数列表,返回值及其他功能函数
//请参照DBDK编程参考手册（模拟中继语音分册）声音转换函数部分
typedef int (*AddFunc)(char *,char *);
int trans(){
    HMODULE hDll=LoadLibrary("djcvt.dll");
    AddFunc trs=(AddFunc)GetProcAddress(hDll,"PcmtoWave");
    return trs("welcome","temps");
    //FreeLibrary(hDll);
};
//----------------------------------------------------------------



int main()
{
    //cout << "Hello world!" << endl;
    printf("%d",trans());

    return 0;
}
