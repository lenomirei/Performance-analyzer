#include "��ͷ.h"
#ifdef _WIN32
#include <windows.h>
#endif




void test1()
{	
	PERFORMANCE_PROFILER_EE_BEGIN(NETWORK,"����");
	Sleep(1000);
	PERFORMANCE_PROFILER_EE_END(NETWORK);

	/*PPSection *ps2 = pp.CreateSeciton(__FILE__, __FUNCTION__, __LINE__, "����");
	ps2->Begin();
	Sleep(500);
	ps2->End();


	PPSection *ps3 = pp.CreateSeciton(__FILE__, __FUNCTION__, __LINE__, "����");
	ps3->Begin();
	Sleep(1500);
	ps3->End();*/
}

int main()
{
	test1();
	return 0;
}