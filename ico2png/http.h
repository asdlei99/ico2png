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
			throw "建立套接字失败!";

		addr.sin_family = AF_INET;
		addr.sin_port = htons(atoi(m_port.c_str()));
		addr.sin_addr.s_addr = inet_addr(m_ip.c_str());
		rv = connect(s,(sockaddr*)&addr,sizeof(addr));
		if(rv == SOCKET_ERROR)
			throw "无法连接到服务器!";
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
			throw "未能成功向服务器发送数据!";
		}
		parseResponse(R);
		*ppr = R;
		return true;
	}

private:
	bool parseResponse(CMyHttpResonse* R)
	{
		char c;
		//读HTTP版本
		while(recv(&c,1) && c!=' '){
			R->version += c;
		}
		//读状态号
		while(recv(&c,1) && c!= ' '){
			R->state += c;
		}
		//读状态说明
		while(recv(&c,1) && c!='\r'){
			R->comment += c;
		}
		// \r已读,还有一个\n
		recv(&c,1);
		if(c!='\n')
			throw "http error";
		//读键值对
		for(;;){
			string k,v;
			//读键
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
			//读值
			recv(&c,1);
			assert(c==' ');
			while(recv(&c,1) && c!='\r'){
				v += c;
			}
			// 跳过 \n
			recv(&c,1);
			assert(c=='\n');

			//结果
			R->heads[k] = v;
		}
		//如果有数据返回的话, 数据从这里开始

		bool bContentLength,bTransferEncoding;
		bContentLength = R->heads.count("Content-Length")!=0;
		bTransferEncoding = R->heads.count("Transfer-Encoding")!=0;
		assert( (bContentLength && bTransferEncoding) == false);

		if(bContentLength){
			int len = atoi(R->heads["Content-Length"].c_str());
			char* data = new char[len+1]; //多余的1字节,使结果为'\0',但返回实际长度
			memset(data,0,len+1);		//这句可以不要
			recv(data,len);
			R->data = data;
			R->size = len;	//没有多余的1字节

			return true;
		}
		if(bTransferEncoding){
			//参考:http://www.w3.org/Protocols/rfc2616/rfc2616-sec3.html#sec3.6.1
			if(R->heads["Transfer-Encoding"] != "chunked")
				throw "未处理的传送编码类型!";

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
				//读取chunked数据长度
				while(recv(&c,1) && c!=' ' && c!='\r'){
					len = len*16 + char_val(c);
				}

				//结束当前chunked块长度部分
				if(c!='\r'){
					while(recv(&c,1) && c!='\r')
						;
				}
				recv(&c,1);
				assert(c=='\n');

				//块长度为零,chunked数据结束
				if(len == 0){
					break;
				}

				//以下开始读取chunked数据块数据部分 + \r\n
				char* p = new char[len+2];
				recv(p,len+2);

				assert(p[len+0] == '\r');
				assert(p[len+1] == '\n');

				bytes.Write(p,len);
				delete[] p;
			}
			//chunked 数据读取结束
			recv(&c,1);
			assert(c == '\r');
			recv(&c,1);
			assert(c == '\n');

			bytes.Duplicate(&R->data);
			R->size = bytes.GetSize();
			return true;
		}
		//如果到了这里, 说明没有消息数据
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
