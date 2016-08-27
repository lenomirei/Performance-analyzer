#pragma once
#include <iostream>
#include <ctime>
#include <map>
#include <unordered_map>
#include <string>
#include <cassert>
#include <stdarg.h>
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
	LongType _beginTime;
	LongType _costTime;//花费时间
	LongType _callCount;//调用次数
	PPSection()
		:_costTime(0)
		,_beginTime(0)
		,_callCount(0)
	{}
	void Begin()
	{
		_beginTime = clock();
	}
	void End()
	{
		_costTime += clock() - _beginTime;
		++_callCount;
		//cout << _costTime << endl;
	}
};

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
	FileSaveAdapter(FileSaveAdapter &);
private:
	FILE *_fout;
};




////////////////////////////////////////////////////////////////////////////////

class PerformanceProfiler:public Singleton<PerformanceProfiler>
{
	
	typedef unordered_map<PPNode, PPSection*, PPNode_Hash> PP_MAP;
public:
	
	PPSection* CreateSeciton(const char * filename, const char* function, size_t line, const char *desc)
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
		ConsoleSaveAdapter csa;
		_Output(csa);
		FileSaveAdapter fsa("PerFormanceProfilerRepot.txt");
		_Output(csa);

	}
protected:
	
	void _Output(SaveAdapter &sa)
	{
		int num = 1;
		PP_MAP::iterator ppit = _ppMap.begin();
		while (ppit != _ppMap.end())
		{
			const PPNode &node = ppit->first;
			PPSection *section = ppit->second;
			sa.Save("No.%d , Desc :%s",num++,node._desc.c_str());
			sa.Save("Filename : %s , Line : %d , Function : %s", num++, node._filename.c_str(), node._line, node._function.c_str());
			sa.Save("CostTime : %.2f  , CallCount : %lld ",((double)section->_costTime)/1000,section->_callCount);
			++ppit;
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

#define PERFORMANCE_PROFILER_EE_BEGIN(sign,desc)\
PPSection *ps##sign=PerformanceProfiler::GetInstance()->CreateSeciton\
(__FILE__,__FUNCTION__,__LINE__,desc);\
ps##sign->Begin();

#define PERFORMANCE_PROFILER_EE_END(sign)\
ps##sign->End();