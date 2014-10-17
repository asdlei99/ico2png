#define _CRT_SECURE_NO_WARNINGS
#include <WinSock2.h>
#include <windows.h>
#include <string>
#include <sstream>
#include <cassert>
#include <vector>
#include <iostream>

using namespace std;

#pragma comment(lib,"ws2_32")

#include "http.h"
#include "json.h"
#include "ByteArray.h"



class CIco2Png
{
public:
	CIco2Png():m_http("184.95.57.115"){}
	~CIco2Png(){}

	bool ProcessFile(const char* fname)
	{
		void* pdata=0;
		size_t sz;
		try{
			m_file = fname;
			if(!readFile(&pdata,&sz))
				throw "���ļ�ʧ��!";

			string str1,str2,str4;
			makeHdr(sz,str1,str2,str4);

			size_t total = str1.size()+str2.size()+sz+str4.size();
			char* p = new char[total];
			memcpy(p+0,str1.c_str(),str1.size());
			memcpy(p+str1.size(),str2.c_str(),str2.size());
			memcpy(p+str1.size()+str2.size(),pdata,sz);
			memcpy(p+str1.size()+str2.size()+sz,str4.c_str(),str4.size());

			CMyHttpResonse* R;
			cout << "\n���ӷ�����...";
			m_http.Connect();
			cout << "\n�ϴ�����...";
			m_http.PostData(p,total,0,&R);
			m_http.Disconnect();

			delete[] p;

			if(R->state != "200"){
				throw "HTTP���󷵻�ʧ��!";
			}

			//json...
			vector<PngInfo> pngs;
			string jobid;
			cout << "\n����JSON����...";
			parseJson((char*)R->data,&jobid,&pngs);
			cout << "\nJson��Ϣ: �ļ���: " << pngs.size();
			if(!pngs.size()){
				cout << "\n������Ϊ��ico��ʽ���Ϸ�, ���ǲ���ת����png, ��������վ˵����Ǹ~\n" ;
			}
			delete R;

			for(auto i=pngs.begin(); i!=pngs.end(); i++){
				stringstream get;
				get << "GET /png.php?i=" << i->index << "&j=" << jobid << " HTTP/1.1\r\n"
					"Host: icotopng.com\r\n"
					"User-Agent: Mozilla/5.0 (Windows NT 6.1; rv:28.0) Gecko/20100101 Firefox/28.0\r\n"
					"Accept: image/png,image/*;q=0.8,*/*;q=0.5\r\n"
					"Accept-Language: en-US,en;q=0.5\r\n"
					"Accept-Encoding: gzip, deflate\r\n"
					"Referer: http://icotopng.com/\r\n"
					"Connection: keep-alive\r\n"
					"\r\n";

				cout << "\n�����ļ�: " << i->index ;

				string strGet(get.str());
				CMyHttpResonse* R;
				m_http.Connect();
				m_http.PostData(strGet.c_str(),strGet.size(),0,&R);
				m_http.Disconnect();

				if(R->state != "200")
					throw "http ����ʧ��!";

				string fn = m_file + '_' + i->width + 'x' + i->height + "_" + i->depth + "bit.png";
				if(!saveFile(fn.c_str(),R->data,R->size)){
					//throw "�����ļ�ʧ��!";
					cout << "�����ļ�ʧ��!";
					continue;
				}
				cout << "\n�����ļ�: " << fn;
			}
			cout << endl;
			return true;
		}
		catch(...){
			if(pdata) delete[] pdata;
			throw;
		}
		return false;
	}

private:
	bool makeHdr(size_t fsize,string& str1,string& str2,string& str4)
	{
		stringstream s1,s2,s4;
		//��2����
		s2 << "-----------------------------25998118435350\r\n"
			"Content-Disposition: form-data; name=\"files[]\"; filename=\"" << m_file << "\"\r\n"
			"Content-Type: image/x-icon\r\n"
			"\r\n";
		str2 = s2.str();

		//��4����
		s4 << "\r\n" << "-----------------------------25998118435350--\r\n";
		str4 = s4.str();

		//��1����:HTTP����ͷ
		s1 << "POST /cgi-bin/upload.pl HTTP/1.1\r\n"
			"Host: icotopng.com\r\n"
			"User-Agent: Mozilla/5.0 (Windows NT 6.1; rv:28.0) Gecko/20100101 Firefox/28.0\r\n"
			"Accept: application/json, text/javascript, */*; q=0.01\r\n"
			"Accept-Language: en-US,en;q=0.5\r\n"
			"Accept-Encoding: gzip, deflate\r\n"
			"X-Requested-With: XMLHttpRequest\r\n"
			"Referer: http://icotopng.com/\r\n"
			"Content-Length: " << str2.size()+fsize+str4.size() << "\r\n"
			"Content-Type: multipart/form-data; boundary=---------------------------25998118435350\r\n"
			"Connection: keep-alive\r\n"
			"Pragma: no-cache\r\n"
			"Cache-Control: no-cache\r\n"
			"\r\n";

		str1 = s1.str();
		return true;
	}

	bool readFile(void** ppdata,size_t* psz)
	{
		const char* fn = m_file.c_str();
		FILE* fp = fopen(fn,"rb");
		if(!fp) return false;
		fseek(fp,0,SEEK_END);

		size_t fsize = ftell(fp);
		if(!fsize) {fclose(fp); return false;}

		if(fsize > 1<<20) return false;

		char* p = new char[fsize];
		memset(p,0,fsize);

		fseek(fp,0,SEEK_SET);
		fread(p,1,fsize,fp);

		fclose(fp);

		*ppdata = p;
		*psz = fsize;

		return true;
	}

	bool saveFile(const char* fn,const void* data,size_t sz)
	{
		FILE* fp = fopen(fn,"wb");
		if(!fp) return false;

		fwrite(data,1,sz,fp);

		fclose(fp);
		return true;
	}

	bool parseJson(const char* js,string* jobid,vector<PngInfo>* pngs)
	{
		return ::ParseJson(js,jobid,pngs);
	}


private:
	CMyHttp m_http;
	string m_file;
};

void usage(bool bcmd)
{
	const char* msg = 
		"                                      Ico2Png\n"
		"    Ico2Png��һ��mini��Icoͼ��תPngͼƬ�Ĺ���, �������������κε�"
		"ͼƬ��������빤��, ���ǰ�ת������������һ����վ(http://icotopng.com),"
		"���е�����ת�����ɹ���������վ�������, ��Ico2Pngֻ��ͨ����վ������ת��"
		"�����ص����ض���.\n"
		"����: Ů������ QQ:191035066\n"
		"ʱ��: Apr' 20,2014, Sun.\n"
		"����: HTTP + C++ @ Visual Studio 2012 @ Win7\n"
		"��ҳ: http://www.cnblogs.com/memset/p/ico2png.html\n\n"
		;
	cout << msg ;

	if(!bcmd) cout << "��ʹ��������, �봫�����账����ļ�����; ֧��'*'ͨ���, �ļ��������ǰĿ¼.\n\n";
}

void get_files(const char* spec,vector<string>* files)
{
	string dir;
	const char* s1 = strrchr(spec,'\\');
	const char* s2 = strrchr(spec,'/');
	
	if(s1 && s2){
		const char* s = s1>s2 ? s1 : s2;
		dir = string(spec,int(s)-int(spec)+1);
	}
	else{
		const char* s = s1 ? s1 : s2;
		if(s) dir = string(spec,int(s)-int(spec)+1);
		else dir = "";
	}
	
	WIN32_FIND_DATA fd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	hFind = FindFirstFile(spec,&fd);
	if(hFind != INVALID_HANDLE_VALUE){
		do{
			if(fd.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_VIRTUAL))
				continue;
			string file(dir + fd.cFileName);
			files->push_back(file);
		}while(FindNextFile(hFind,&fd));
		FindClose(hFind);
	}
}

string get_real_fp(const char* fp)
{
	if(!*fp) return "";

	while(*fp == '\"') fp++;
	while(*fp == ' ')  fp++;

	const char* e = fp+strlen(fp);
	e--;
	while((*e==' ' || *e=='\"' || *e=='\n') && e>=fp) e--;

	return string(fp,e-fp+1);
}

int process(const char* spec)
{
	int rv=0;
	string rfp = get_real_fp(spec);
	vector<string> files;
	get_files(rfp.c_str(),&files);
	if(files.size() == 0){
		cout << "û���ҵ��κ��������ļ���ƥ����ļ�.\n" << endl;
		return 1;
	}else{
		CIco2Png png;
		cout << "�� " << files.size() << " ���ļ��ȴ�����..." << endl;
		for(auto i=files.begin(); i!=files.end(); i++){
			try{
				cout << "���ڴ����ļ�: " << *i << endl;
				png.ProcessFile(i->c_str());
			}
			catch(const char* e){
				cout << "�ļ�: " << *i << endl;
				cout << "����: " << e << endl;
				cout << endl;
				//rv = 1;
				rv ++;
			}
		}
		return rv;
	}
}

int main(int argc,char* argv[])
{
	int rv=0;
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2),&wsa) != 0){
		cerr << "WSAStartup ʧ��!" << endl;
		return 1;
	}

	if(argc == 1){
		usage(0);
		for(;;){
			char buf[300];
			cout << "�����ļ�:" ;
			fgets(buf,sizeof(buf),stdin);
			if(*buf == '\n') continue;

			process(buf);
			cout << endl;
		}
	}else{
		rv = process(argv[1]);
	}

	WSACleanup();
	return rv;
}

