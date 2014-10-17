#pragma once

#include <cstring>
#include <cassert>

class CMyByteArray
{
public:
	CMyByteArray():
		m_pData(0),
		m_iDataSize(0),
		m_iDataPos(0)
	{}
	~CMyByteArray()
	{
		if(m_pData){
			delete[] m_pData;
			m_pData = 0;
		}
	}
	void* GetData()
	{
		return m_pData;
	}
	size_t GetSize()
	{
		return GetSpaceUsed();
	}

	size_t Write(const void* data,size_t sz)
	{
		if(sz > GetSpaceLeft()) ReallocateDataSpace(sz);
		memcpy((char*)m_pData+GetSpaceUsed(),data,sz);
		GetSpaceUsed() += sz;
		return sz;
	}

	bool Duplicate(void** ppData)
	{
		assert(GetSize() > 0);
		char* p = new char[GetSpaceUsed()];
		memcpy(p,GetData(),GetSize());
		*ppData = p;
		return true;
	}

private:
	void*	m_pData;
	size_t	m_iDataSize;
	size_t	m_iDataPos;

private:
	size_t GetGranularity()
	{
		return (size_t)1<<20;
	}
	size_t GetSpaceLeft()
	{
		//如果m_iDataPos == m_iDataSize
		//结果正确, 但貌似表达式错误?
		// -1 + 1 = 0, size_t 得不到-1, 0x~FF + 1,也得0
		return m_iDataSize-1 - m_iDataPos + 1;
	}
	size_t& GetSpaceUsed()
	{
		//结束-开始+1
		//return m_iDataPos-1 -0 + 1;
		return m_iDataPos;
	}

	void ReallocateDataSpace(size_t szAddition)
	{
		//只有此条件才会分配空间,也即调用此函数前必须检测空间剩余
		assert(GetSpaceLeft()<szAddition);

		//计算除去剩余空间后还应增加的空间大小
		size_t left = szAddition - GetSpaceLeft();

		//按分配粒度计算块数及剩余
		size_t nBlocks = left / GetGranularity();
		size_t cbRemain = left - nBlocks*GetGranularity();
		if(cbRemain) ++nBlocks;

		//计算新空间并重新分配
		size_t newDataSize = m_iDataSize + nBlocks*GetGranularity();
		void*  pData = (void*)new char[newDataSize];

		//复制原来的数据到新空间
		//memcpy支持第3个参数传0吗?
		if(GetSpaceUsed()) memcpy(pData,m_pData,GetSpaceUsed());

		if(m_pData) delete[] m_pData;
		m_pData = pData;
		m_iDataSize = newDataSize;
	}
};