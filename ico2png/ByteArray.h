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
		//���m_iDataPos == m_iDataSize
		//�����ȷ, ��ò�Ʊ��ʽ����?
		// -1 + 1 = 0, size_t �ò���-1, 0x~FF + 1,Ҳ��0
		return m_iDataSize-1 - m_iDataPos + 1;
	}
	size_t& GetSpaceUsed()
	{
		//����-��ʼ+1
		//return m_iDataPos-1 -0 + 1;
		return m_iDataPos;
	}

	void ReallocateDataSpace(size_t szAddition)
	{
		//ֻ�д������Ż����ռ�,Ҳ�����ô˺���ǰ������ռ�ʣ��
		assert(GetSpaceLeft()<szAddition);

		//�����ȥʣ��ռ��Ӧ���ӵĿռ��С
		size_t left = szAddition - GetSpaceLeft();

		//���������ȼ��������ʣ��
		size_t nBlocks = left / GetGranularity();
		size_t cbRemain = left - nBlocks*GetGranularity();
		if(cbRemain) ++nBlocks;

		//�����¿ռ䲢���·���
		size_t newDataSize = m_iDataSize + nBlocks*GetGranularity();
		void*  pData = (void*)new char[newDataSize];

		//����ԭ�������ݵ��¿ռ�
		//memcpy֧�ֵ�3��������0��?
		if(GetSpaceUsed()) memcpy(pData,m_pData,GetSpaceUsed());

		if(m_pData) delete[] m_pData;
		m_pData = pData;
		m_iDataSize = newDataSize;
	}
};