#pragma once

#include <WinSock2.h>
#include <windows.h>
#include <map>
#include <cassert>
#include <string>
using namespace std;

#include "ByteArray.h"

struct CMyHttpResonse
{
	string version;
	string state;
	string comment;

	map<string,string> heads;

	void* data;
	size_t size;

	CMyHttpResonse():data(0){}
	~CMyHttpResonse(){if(data) delete[] data;}
};

class CMyHttp
{
public:
	CMyHttp(const char* ip,const char* port="80")
	{
		m_ip = ip;
		m_port = port;
	}
public:
	bool Connect()
	{
		int rv;
		SOCKADDR_IN addr;

		SOCKET s = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
		assert(s!=INVALID_SOCKET);
		if(s==INVALID_SOCKET)
			throw "�����׽���ʧ��!";

		addr.sin_family = AF_INET;
		addr.sin_port = htons(atoi(m_port.c_str()));
		addr.sin_addr.s_addr = inet_addr(m_ip.c_str());
		rv = connect(s,(sockaddr*)&addr,sizeof(addr));
		if(rv == SOCKET_ERROR)
			throw "�޷����ӵ�������!";
		m_sock = s;

		return true;
	}
	bool Disconnect()
	{
		closesocket(m_sock);
		return true;
	}
	bool PostData(const char* data,size_t sz, int /*timeouts*/,CMyHttpResonse** ppr)
	{
		CMyHttpResonse* R = new CMyHttpResonse;
		if(!send(data,sz)){
			throw "δ�ܳɹ����������������!";
		}
		parseResponse(R);
		*ppr = R;
		return true;
	}

private:
	bool parseResponse(CMyHttpResonse* R)
	{
		char c;
		//��HTTP�汾
		while(recv(&c,1) && c!=' '){
			R->version += c;
		}
		//��״̬��
		while(recv(&c,1) && c!= ' '){
			R->state += c;
		}
		//��״̬˵��
		while(recv(&c,1) && c!='\r'){
			R->comment += c;
		}
		// \r�Ѷ�,����һ��\n
		recv(&c,1);
		if(c!='\n')
			throw "http error";
		//����ֵ��
		for(;;){
			string k,v;
			//����
			recv(&c,1);
			if(c=='\r'){
				recv(&c,1);
				assert(c=='\n');
				break;
			}
			else
				k += c;
			while(recv(&c,1) && c!=':'){
				k += c;
			}
			//��ֵ
			recv(&c,1);
			assert(c==' ');
			while(recv(&c,1) && c!='\r'){
				v += c;
			}
			// ���� \n
			recv(&c,1);
			assert(c=='\n');

			//���
			R->heads[k] = v;
		}
		//��������ݷ��صĻ�, ���ݴ����￪ʼ

		bool bContentLength,bTransferEncoding;
		bContentLength = R->heads.count("Content-Length")!=0;
		bTransferEncoding = R->heads.count("Transfer-Encoding")!=0;
		assert( (bContentLength && bTransferEncoding) == false);

		if(bContentLength){
			int len = atoi(R->heads["Content-Length"].c_str());
			char* data = new char[len+1]; //�����1�ֽ�,ʹ���Ϊ'\0',������ʵ�ʳ���
			memset(data,0,len+1);		//�����Բ�Ҫ
			recv(data,len);
			R->data = data;
			R->size = len;	//û�ж����1�ֽ�

			return true;
		}
		if(bTransferEncoding){
			//�ο�:http://www.w3.org/Protocols/rfc2616/rfc2616-sec3.html#sec3.6.1
			if(R->heads["Transfer-Encoding"] != "chunked")
				throw "δ����Ĵ��ͱ�������!";

			auto char_val = [](char ch)
			{
				int val = 0;
				if(ch>='0' && ch<='9') val = ch-'0'+ 0;
				if(ch>='a' && ch<='f') val = ch-'a'+10;
				if(ch>='A' && ch<='F') val = ch-'A'+10;
				return val;
			};

			int len;
			CMyByteArray bytes;

			for(;;){
				len = 0;
				//��ȡchunked���ݳ���
				while(recv(&c,1) && c!=' ' && c!='\r'){
					len = len*16 + char_val(c);
				}

				//������ǰchunked�鳤�Ȳ���
				if(c!='\r'){
					while(recv(&c,1) && c!='\r')
						;
				}
				recv(&c,1);
				assert(c=='\n');

				//�鳤��Ϊ��,chunked���ݽ���
				if(len == 0){
					break;
				}

				//���¿�ʼ��ȡchunked���ݿ����ݲ��� + \r\n
				char* p = new char[len+2];
				recv(p,len+2);

				assert(p[len+0] == '\r');
				assert(p[len+1] == '\n');

				bytes.Write(p,len);
				delete[] p;
			}
			//chunked ���ݶ�ȡ����
			recv(&c,1);
			assert(c == '\r');
			recv(&c,1);
			assert(c == '\n');

			bytes.Duplicate(&R->data);
			R->size = bytes.GetSize();
			return true;
		}
		//�����������, ˵��û����Ϣ����
		R->data = 0;
		R->size = 0;
		return true;
	}

	bool send(const void* data,size_t sz)
	{
		const char* p = (const char*)data;
		int sent = 0;
		while(sent < (int)sz){
			int cb = ::send(m_sock,p+sent,sz-sent,0);
			if(cb==0) return false;
			sent += cb;
		}
		return true;
	}

	int recv(char* buf,size_t sz)
	{
		int rv;
		size_t read=0;
		while(read != sz){
			rv = ::recv(m_sock,buf+read,sz-read,0);
			if(!rv || rv == -1){
				throw "recv() error";
			}
			read += rv;
		}
		return read; // equals to sz
	}

private:
	string m_ip;
	string m_port;
	SOCKET m_sock;
};
