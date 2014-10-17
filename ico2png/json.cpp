#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <map>
#include <vector>
#include <cassert>
#include <string>
#include <cctype>
using namespace std;

#include "json.h"

class Json;

char* json_val(Json** js,char* data);
char* json_str(Json** js,char* data);
char* json_obj(Json** js,char* data);
char* json_arr(Json** js,char* data);
char* json_num(Json** js,char* data);
char* read_int(Json** js,char* data,string* i);
char* read_str(Json** js,char* data,string* str);

class Json{
public:
	enum TYPE{kString,kObject,kArray,kNumber,kOther,kPair};
	TYPE type;
};

class JsonString : public Json
{
public:
	JsonString(){type = kString;}
	string key;
};

class JsonArray : public Json
{
public:
	JsonArray(){type= Json::kArray;}
	typedef vector<Json*> JsonPtr;
	JsonPtr Ptrs;
};

typedef map<string,Json*> JsonKV;

class JsonKeyVal : public Json
{
public:
	JsonKeyVal(){type = Json::kPair;}
	JsonKV Kvs;
};

class JsonObject : public Json
{
public:
	JsonObject(){type = Json::kObject;}
	typedef vector<JsonKeyVal*> JsonObj;
	JsonObj objs;
};

class JsonNumber : public Json
{
public:
	JsonNumber(){type = Json::kNumber;}
	string num;
};

class JsonOther : public Json
{
public:
	JsonOther(){type = Json::kOther;}
	string oth;
};

char* skipws(char* s)
{
	while(*s==' ' || *s=='\n' || *s=='r' || *s=='\t')
		s++;
	return s;
}

char* read_str(char* data,string* str)
{
	string& s = *str;
	++data;
	while(*data && *data!='\"'){
		s += *data++;
	}
	++data; // skip "
	return data;
}

char* read_int(char* data,string* i)
{
	string& s = *i;
	while(*data && isdigit(*data)){
		s += *data++;
	}
	return data;
}

char* json_arr(Json** js,char* data)
{
	JsonArray* arr = new JsonArray;
	*js = arr;
	data=skipws(++data);
	for(;;){
		Json* j;
		data = skipws(json_val(&j,data));
		arr->Ptrs.push_back(j);
		if(*data == ']'){
			++data;
			break;
		}
		if(*data == ','){
			data = skipws(++data);
			continue;
		}
	}
	return data;
}

char* json_obj(Json** js,char* data)
{
	JsonObject* obj = new JsonObject;
	*js = obj;
	assert(*data == '{');
	data = skipws(++data);
	for(;;){
		if(*data == '}'){
			++data;
			break;
		}
		JsonKeyVal* kv = new JsonKeyVal;
		JsonString* str;
		Json* val;
		assert(*data == '\"');
		data = skipws(json_str((Json**)&str,data));
		assert(*data == ':');
		data = skipws(json_val(&val,++data));

		kv->Kvs[str->key] = val;
		delete str;

		obj->objs.push_back(kv);

		if(*data == ',') {
			data=skipws(++data); 
			continue;
		}
	}
	return data;
}

char* json_str(Json** js,char* data)
{
	assert(*data == '\"');
	JsonString* str = new JsonString;
	*js = str;
	string s;
	data = read_str(data,&s);
	//cout << s << endl;

	str->key = s;
	return data;
}

char* json_num(Json** js,char* data)
{
	JsonNumber* num = new JsonNumber;
	*js = num;
	string i;
	data = read_int(data,&i);
	//cout << i << endl;
	num->num = i;
	return data;
}

char* json_oth(Json** js,char* data)
{
	JsonOther* oth = new JsonOther;
	*js = oth;
	string i;
	while(isalnum(*data)){
		i += *data;
		++data;
	}
	//cout << i << endl;
	oth->oth = i;
	return data;
}

char* json_val(Json** js,char* data)
{
	data = skipws(data);
	if(*data == '{'){
		data = json_obj(js,data);
	}
	else if(*data == '['){
		data = json_arr(js,data);
	}
	else if(*data == '\"'){
		data = json_str(js,data);
	}
	else if(isdigit(*data)){
		data = json_num(js,data);
	}
	else if(isalpha(*data)){
		data = json_oth(js,data);
	}

	return data;
}


bool readFile(const char* fn,void** ppdata,size_t* psz)
{
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

#if 0

void visit(Json* js,int indent)
{
	if(js->type == Json::kArray){
		JsonArray* p = (JsonArray*)js;
		for(auto i=p->Ptrs.begin(); i!=p->Ptrs.end(); i++){
			cout << "array:";
			visit(*i,indent+4);
		}
	}
	else if(js->type == Json::kNumber){
		JsonNumber* p = (JsonNumber*)js;
		cout << string(indent,32);
		cout << "num: " << p->num << endl;
	}
	else if(js->type == Json::kObject){
		JsonObject* p = (JsonObject*)js;
		for(auto i=p->objs.begin(); i!=p->objs.end(); i++){
			visit(*i,indent+4);
		}
	}
	else if(js->type == Json::kOther){
		JsonOther* p = (JsonOther*)js;
		cout << string(indent,32);
		cout << "oth: " << p->oth << endl;
	}
	else if(js->type == Json::kPair){
		JsonKeyVal* p = (JsonKeyVal*)js;
		for(auto i=p->Kvs.begin(); i!=p->Kvs.end(); i++){
			cout << string(indent,32);
			cout << "key: " << i->first << endl;
			visit(i->second,indent+4);
		}
	}
	else if(js->type == Json::kString){
		JsonString* p = (JsonString*)js;
		cout << string(indent,32);
		cout << p->key << endl;
	}
}

int main()
{
	char* pdata;
	size_t sz;
	if(!readFile("C:\\Users\\YangTao\\Desktop\\json.txt",(void**)&pdata,&sz))
		return 0;

	Json* js;

	json_val(&js,pdata);

	visit(js,0);

	return 0;



}
#endif

bool ParseJson(const char* jsrc,string* jobid,vector<PngInfo>* pngs)
{
	Json* js;
	try{
		json_val(&js,(char*)jsrc);
		if(js->type != Json::kArray)
			throw "json数据错误!";
		auto pArr = static_cast<JsonArray*>(js);
		auto pObj1 = (JsonObject*)pArr->Ptrs[0];
		JsonKeyVal* ji = pObj1->objs[0];
		*jobid = ((JsonString*)(ji->Kvs["jobid"]))->key;
		//for(auto s=pObj1->objs.begin();s!=pObj1->objs.end(); s++)
		JsonKeyVal* pkv = pObj1->objs[pObj1->objs.size()-1];
		if(pkv->type != Json::kPair)
			throw "Json数据错误!";
		JsonKV& kv = pkv->Kvs;
		JsonArray* pArrIcons = (JsonArray*)kv["icons"];
		//如果没有正确地处理, 则没有icons
		if(!pArrIcons) return true;
		for(auto i=pArrIcons->Ptrs.begin(); i!=pArrIcons->Ptrs.end(); i++){
			JsonObject* pobj = (JsonObject*)*i;
			string index  = ((JsonNumber*)pobj->objs[0]->Kvs["index"])->num;
			string width  = ((JsonNumber*)pobj->objs[1]->Kvs["width"])->num;
			string height = ((JsonNumber*)pobj->objs[2]->Kvs["height"])->num;
			string depth  = ((JsonNumber*)pobj->objs[3]->Kvs["bit_depth"])->num;
			pngs->push_back(PngInfo(index,width,height,depth));
		}

		return true;
	}
	catch(...){
		throw;
	}
	return false;
}
