#pragma once
#include <iostream>
#include <ctime>
#include <map>
#include <unordered_map>
#include <string>
#include <cassert>
#include <stdarg.h>
#include <mutex>
#include <algorithm>
#ifdef _WIN32
#include <windows.h>
#include <thread>
#else
#include <thread.h>
#endif
using namespace std;


template<class T>
class Singleton
{
public:
	static T* GetInstance()
	{
		assert(_sInstance);
		return _sInstance;
	}
protected:
	
	static T *_sInstance;
};

template<class T>
T *Singleton<T>::_sInstance = new T;






struct PPNode//重载大于小于符号函数
{
	PPNode(const char *filename,const char *function,size_t line,const char *desc)
		:_filename(filename)
		,_function(function)
		,_line(line)
		,_desc(desc)
	{}
	string _filename;
	string _function;
	size_t _line;
	string _desc;  //附加描述信息
				   //计算PPNode的哈希
	
	
	bool operator==(const PPNode &node) const 
	{
		if (_line == node._line && _function == node._function && _filename == node._filename)
		{
			return true;
		}
		return false;
	}
};
struct PPNode_Hash
{
	static size_t BKDRHash(const char *s)
	{
		unsigned int seed = 131;
		unsigned hash = 0;
		while (*s)
		{
			hash = hash*seed + (*s++);

		}
		return (hash & 0x7FFFFFFF);//八成是想取个正数
	}
	size_t operator()(const PPNode node) const
	{
		static string hash;
		hash = node._desc;
		hash += node._function;
		return BKDRHash(hash.c_str());
	}//实现仿函数,跟unordered_map的模板类型有关
};
struct PPSection
{
	typedef long long LongType;
	LongType _totalBeginTime;
	LongType _totalCostTime;//花费时间
	LongType _totalCallCount;//调用次数
	LongType _totalRefCount;//引用计数，处理递归使用
	map<int, LongType> _beginTimeMap;
	map<int, LongType> _costTimeMap;
	map<int, LongType> _callCountMap;
	map<int, LongType> _refCountMap;
	mutex mx;
	//<id,统计信息>
	PPSection()
		:_totalBeginTime(0)
		, _totalCostTime(0)
		, _totalCallCount(0)
		, _totalRefCount(0)
	{}
	void Begin(int _id)
	{
		lock_guard<mutex> lock(mx);
		if (_id != -1)
		{
			if (_refCountMap[_id]++ == 0)
			{
				_beginTimeMap[_id] = clock();
			}
		}
		else
		{
			if (_totalRefCount++ == 0)
			{
				_totalBeginTime = clock();
			}
		}
	}
	void End(int _id)
	{
		lock_guard<mutex> lock(mx);
		if (_id != -1)
		{
			if (--_refCountMap[_id] == 0)
			{
				_costTimeMap[_id] = clock() - _beginTimeMap[_id];
			}
			++_callCountMap[_id];
		}
		
		else
		{
			if (--_totalRefCount == 0)
			{
				_totalCostTime = clock() - _totalBeginTime;
			}
			++_totalCallCount;
		}
		//分别统计线程的花费时间和总的花费时间
	}
};
//////////////////////////////////////////////////////////////////////////////
/////配置管理
enum ConfigOptions
{
	NONE = 0,
	PERFORMANCE_PROFILER = 1,//开始剖析的选项
	SAVE_TO_CONSOLE = 2,
	SAVE_TO_FILE = 4,
	SORT_BY_COSTTIME = 8,
	SORT_BY_CALLCOUNT = 16,
};
class ConfigManager :public Singleton<ConfigManager>
{
	friend class Singleton<ConfigManager>;
public:
	void SetOptions(int options)
	{
		_options = options;
	}
	void AddOptions(int options)
	{
		_options |= options;
	}
	void DeleteOptions(int options)
	{
		_options &= (~options);
	}
    int	GetOptions()
	{
		return _options;
	}
protected:
	ConfigManager()
		:_options(NONE)
	{}
	ConfigManager(ConfigManager &)
	{}
	ConfigManager &operator=(ConfigManager &)
	{}
protected:
	int _options;
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class SaveAdapter
{
public:
	virtual void Save(const char * format,...) = 0;

};
class ConsoleSaveAdapter:public SaveAdapter
{
public:
	virtual void Save(const char *format,...)
	{
		va_list va;
		va_start(va,format);
		vfprintf(stdout, format, va);
		va_end(va);
	}
};
class FileSaveAdapter:public SaveAdapter
{
public:
	FileSaveAdapter(const char *path)
	{
		_fout = fopen(path, "wb+");
	}
	~FileSaveAdapter()
	{
		fclose(_fout);
	}
	virtual void Save(const char *format, ...)
	{
		va_list va;
		va_start(va, format);
		vfprintf(_fout, format, va);
		va_end(va);
	}
protected:
	FileSaveAdapter(FileSaveAdapter &)
	{}
private:
	FILE *_fout;
};




////////////////////////////////////////////////////////////////////////////////

class PerformanceProfiler:public Singleton<PerformanceProfiler>
{
	typedef long long LongType;
	typedef unordered_map<PPNode, PPSection*, PPNode_Hash> PP_MAP;
public:
	
	PPSection* CreateSection(const char * filename, const char* function, size_t line, const char *desc)
	{
		PPNode node(filename, function, line, desc);
		PPSection *section = NULL;
		PP_MAP::iterator it = _ppMap.find(node);
		if (it != _ppMap.end())
		{
			section = it->second;
		}
		else
		{
			lock_guard<mutex> lock(mx);
			section = new PPSection;
			_ppMap.insert(pair<PPNode, PPSection*>(node, section));
		}
		return section;
	}

	struct Report
	{
		~Report()
		{
			GetInstance()->Output();
		}
	};

	

	void Output()
	{
		int options = ConfigManager::GetInstance()->GetOptions();
		if (options & SAVE_TO_FILE)
		{
			FileSaveAdapter fsa("PerFormanceProfilerRepot.txt");
			_Output(fsa);
		}
		if (options & SAVE_TO_CONSOLE)
		{
			ConsoleSaveAdapter csa;
			_Output(csa);
		}
	}
protected:
	mutex mx;
	void _Output(SaveAdapter &sa)
	{
		vector<PP_MAP::iterator> vInfos;
		int num = 1;
		PP_MAP::iterator ppit = _ppMap.begin();
		while (ppit != _ppMap.end())
		{
			const PPNode &node = ppit->first;
			PPSection *section = ppit->second;

			map<int, LongType>::iterator timeit;
			timeit = section->_costTimeMap.begin();


			/*sa.Save("No.%d , Desc :%s\n", num++, node._desc.c_str());
			sa.Save("Filename : %s , Line : %d , Function : %s\n", node._filename.c_str(), node._line, node._function.c_str());*/

			while (timeit != section->_costTimeMap.end())
			{
				int id = timeit->first;
				section->_totalCostTime += timeit->second;
				section->_totalCallCount += section->_callCountMap[timeit->first];
				/*sa.Save("Thread id : %d , costTime : %.2f , CallCount : %lld\n",id,(double)timeit->second/1000,section->_callCountMap[id]);*/
				++timeit;
			}

			
			/*sa.Save("CostTime : %.2f  , CallCount : %lld , averageTime : %.2f\n",((double)section->_totalCostTime)/1000,section->_totalCallCount, ((double)section->_totalCostTime)/ (1000*section->_totalCallCount));*/
			vInfos.push_back(ppit);
			++ppit;

		}
		struct SortByCostTime
		{
			bool operator()(PP_MAP::iterator l, PP_MAP::iterator r) const
			{
				return (l->second->_totalCostTime) < (r->second->_totalCostTime);
			}
		};
		sort(vInfos.begin(), vInfos.end(), SortByCostTime());
		for (int i = 0; i < vInfos.size(); ++i)
		{
			ppit = vInfos[i];
			const PPNode &node = ppit->first;
			PPSection *section = ppit->second;
			sa.Save("No.%d , Desc :%s\n", num++, node._desc.c_str());
			sa.Save("Filename : %s , Line : %d , Function : %s\n", node._filename.c_str(), node._line, node._function.c_str());
			map<int, LongType>::iterator timeit;
			timeit = section->_costTimeMap.begin();
			while (timeit != section->_costTimeMap.end())
			{
				int id = timeit->first;
				section->_totalCostTime += timeit->second;
				section->_totalCallCount += section->_callCountMap[timeit->first];
				/*sa.Save("Thread id : %d , costTime : %.2f , CallCount : %lld\n",id,(double)timeit->second/1000,section->_callCountMap[id]);*/
				++timeit;
			}
			sa.Save("CostTime : %.2f  , CallCount : %lld , averageTime : %.2f\n",((double)section->_totalCostTime)/1000,section->_totalCallCount, ((double)section->_totalCostTime)/ (1000*section->_totalCallCount));
		}
	}
	friend class Singleton<PerformanceProfiler>;
	PerformanceProfiler()
	{}
	PerformanceProfiler(PerformanceProfiler &)
	{}
	PerformanceProfiler &operator=(PerformanceProfiler &)
	{}
	PP_MAP _ppMap;
	
};
static  PerformanceProfiler::Report report;

static int _GetThreadId()
{
#ifdef _WIN32
	return GetCurrentThreadId();
#else
	return pthread_self();
#endif
}


#define PERFORMANCE_PROFILER_EE_ST_BEGIN(sign,desc)\
PPSection *ps##sign=NULL;\
if(ConfigManager::GetInstance()->GetOptions() & PERFORMANCE_PROFILER)\
{\
	ps##sign=PerformanceProfiler::GetInstance()->CreateSection\
	(__FILE__,__FUNCTION__,__LINE__,desc);\
	ps##sign->Begin(-1);\
}\


#define PERFORMANCE_PROFILER_EE_ST_END(sign)\
if(ps##sign)\
ps##sign->End(-1);


#define PERFORMANCE_PROFILER_EE_MT_BEGIN(sign,desc)\
PPSection *ps##sign=NULL;\
if(ConfigManager::GetInstance()->GetOptions() & PERFORMANCE_PROFILER)\
{\
	ps##sign=PerformanceProfiler::GetInstance()->CreateSection\
	(__FILE__,__FUNCTION__,__LINE__,desc);\
	ps##sign->Begin(_GetThreadId());\
}\


#define PERFORMANCE_PROFILER_EE_MT_END(sign)\
if(ps##sign)\
ps##sign->End(_GetThreadId());


//设置开启选项的宏函数，如果不设置开始剖析就无法打出结果
#define SET_CONFIG_OPTIONS(option)\
ConfigManager::GetInstance()->SetOptions(option)



//暂时不会用的宏函数
//#define PERFORMANCE_PROFILER_EE_BEGIN(sign,desc)\
//PPSection *ps##sign=PerformanceProfiler::GetInstance()->CreateSection\
//(__FILE__,__FUNCTION__,__LINE__,desc);\
//ps##sign->Begin(_GetThreadId());
//
//#define PERFORMANCE_PROFILER_EE_END(sign)\
//ps##sign->End(_GetThreadId());