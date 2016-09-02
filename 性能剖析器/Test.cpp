#include "炎遊.h"


//void func(size_t n)
//{
//	while (n--)
//	{
//		PERFORMANCE_PROFILER_EE_BEGIN(network, "利大");
//		Sleep(100);
//		PERFORMANCE_PROFILER_EE_END(network);
//
//		PERFORMANCE_PROFILER_EE_BEGIN(mid,"mid");
//		Sleep(200);
//		PERFORMANCE_PROFILER_EE_END(mid);
//
//		PERFORMANCE_PROFILER_EE_BEGIN(sql,"database");
//		Sleep(300);
//		PERFORMANCE_PROFILER_EE_END(sql);
//	}
//}
//

void test1()
{	
	SET_CONFIG_OPTIONS(PERFORMANCE_PROFILER | SAVE_TO_CONSOLE);
	PERFORMANCE_PROFILER_EE_ST_BEGIN(NETWORK,"利大");
	Sleep(1000);
	PERFORMANCE_PROFILER_EE_ST_END(NETWORK);

	/*PPSection *ps2 = pp.CreateSection(__FILE__, __FUNCTION__, __LINE__, "利大");
	ps2->Begin();
	Sleep(500);
	ps2->End();


	PPSection *ps3 = pp.CreateSection(__FILE__, __FUNCTION__, __LINE__, "利大");
	ps3->Begin();
	Sleep(1500);
	ps3->End();*/
}


//void TestMulThread()
//{
//	thread t1(func, 5);
//	thread t2(func, 4);
//	//thread t3(func, 3);
//
//	t1.join();
//	t2.join();
//	//t3.join();
//}

int main()
{
	test1();
	return 0;
}