#include <windows.h>
#include <stdio.h>

//�˴���ʽ����dll������wav��ʽת��Ϊ��֧�ֶ�����������pcm��ʽ
//Ҫ��ԭ���ݸ�ʽΪ8k������ܻ����ʧ����� ��������б�,����ֵ���������ܺ���
//�����DBDK��̲ο��ֲᣨģ���м������ֲᣩ����ת����������
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
