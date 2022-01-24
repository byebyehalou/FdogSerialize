#ifndef FDOGSERIALIZE_H
#define FDOGSERIALIZE_H

#include "fdogserializebase.h"
#include "behavior.h"
#include "macrodefinition.h"
#include <map>
#include <vector>
#include <regex>
#include <algorithm>
using namespace fsj;

using namespace std;

static vector<string> baseType1 = {
        "bool", "bool*"
        "char", "unsigned char", "char*", "unsigned char*",
        "int", "unsigned int", "int*", "unsigned int*",
        "short", "unsigned short", "short*", "unsigned short*",
        "long", "unsigned long int", "long*", "unsigned long*",
        "long long", "unsigned long long", "long long*", "unsigned long long*",
        "float", "double", "long double"
};

//有符号类型应该拥有正负号，正号忽视
static map<string, string> baseRegex = {
            {"bool", "(\\d+)"},
            {"char", "(\\d+)"}, {"unsigned char", "(\\d+)"}, 
            {"int", "(\\d+)"}, {"unsigned int", "(\\d+)"},
            {"short", "(\\d+)"}, {"unsigned short", "(\\d+)"}, 
            {"long", "(\\d+)"}, {"unsigned long", "(\\d+)"}, 
            {"float", "(\\d+.\\d+)"}, 
            {"double", "(\\d+.\\d+)"},
            {"long double", "(\\d+.\\d+)"},
            {"char*", "\"(.*?)\""},
            //{"int_array", "(\\[(.*?)\\])"},
};

static map<int, string> complexRegex = {
    {5, "std::vector<(.*?),"},
    {6, "std::map<(.*?),(.*?),"},
    {7, "std::__cxx11::list<(.*?),"},
    {4, ""},
};


//用于数组整体提取
const string arrayRegex = "(\\[(.*?)\\])";
//匹配数组
const string patternArray = "((A)(\\d+)_\\d?(\\D+))";

//匹配别名
const string patterAlias = "(\\(A:(.*?)\\))";

//用于结构体整体提取
const string objectRegex = "(\\{(.*?)\\})";
//匹配结构体
const string patternObject = "((\\d+)(\\D+))";

enum ObjectType{
    OBJECT_BASE = 1,
    OBJECT_STRUCT,
    OBJECT_CLASS,
    OBJECT_ARRAY,
    OBJECT_VECTOR,
    OBJECT_MAP,
    OBJECT_LIST,
};
enum MemberType{
    MEMBER_BASE = 1,
    MEMBER_STRUCT,
    MEMBER_CLASS,
    MEMBER_ARRAY,
    MEMBER_VECTOR,
    MEMBER_MAP,
    MEMBER_LIST,
};

/***********************************
*   存储结构体结构体类型，以及元信息
************************************/
typedef struct ObjectInfo{
    string objectType;  //结构体类型 字符串表示
    int objectTypeInt;  //结构体类型 数值表示
    int objectSize;     //结构体大小
    vector<MetaInfo *> metaInfoObjectList;  //结构体元信息 
}ObjectInfo;

/***********************************
*   存储成员类型，数组大小
************************************/
struct memberAttribute {
    string valueType;
    int valueTypeInt; //类型 数值表示
    int ArraySize;
};

class FdogSerialize {

    private:
    static mutex * mutex_serialize;
    static FdogSerialize * fdogserialize;
    vector<ObjectInfo *> objectInfoList;
    vector<MetaInfo *> baseInfoList;

    FdogSerialize();
    ~FdogSerialize();

    public:
    //获取实例
    static FdogSerialize * Instance();

    //添加objectinfo
    void addObjectInfo(ObjectInfo * objectinfo);

    //获取对应Info
    ObjectInfo & getObjectInfo(string objectName);

    //获取对于Info 针对基础类型获取
    MetaInfo * getMetaInfo(string TypeName);

    //设置别名
    void setAliasName(string Type, string Name, string AliasName);

    //设置是否忽略该字段序列化
    void setIgnoreField(string Type, string Name);

    //一次性设置多个别名
    template<class T, class ...Args>
    void setAliasNameAll();

    //一次性设置忽略多个字段序列化
    template<class T, class ...Args>
    void setIgnoreField();

    //获取成员属性
    memberAttribute getMemberAttribute(string key);

    //获取object类型
    int getObjectTypeInt(string objectName, string typeName);

    //获取基础类型 只有base和struct两种
    ObjectInfo getObjectInfoByType(string typeName, int objectTypeInt);

    //通过宏定义加载的信息获取
    int getObjectTypeByObjectInfo(string objectName);

    //判断是否是基础类型
    bool isBaseType(string typeName);

    //判断是否为vector类型
    bool isVectorType(string objectName, string typeName);

    //判断是否为map类型
    bool isMapType(string objectName, string typeName);

    //判断是否是list类型
    bool isListType(string objectName, string typeName);

    //判断是否是结构体类型
    bool isStructType(string objectName, string typeName);

    //判断是否是数组
    bool isArrayType(string objectName, string typeName);

    //解析数组
    vector<string> CuttingArray(string data);

    //根据复合类型获取struct

    //序列化
    template<typename T>
    void Serialize(string & json_, T & object_, string name = ""){
        //通过传进来的T判断是什么复合类型，ObjectInfo只保存结构体,如果是NULL可以确定传进来的不是struct类型
        ObjectInfo objectinfo = FdogSerialize::Instance()->getObjectInfo(abi::__cxa_demangle(typeid(T).name(),0,0,0));
        //获取的只能是结构体的信息，无法知道是什么复合类型，尝试解析类型 objectType其实是一个结构体类型名称
        int objectType = getObjectTypeInt(objectinfo.objectType, abi::__cxa_demangle(typeid(T).name(),0,0,0));
        if(objectinfo.objectType == "NULL" && objectType != OBJECT_BASE && objectType != OBJECT_STRUCT){
            //说明不是struct类型和base类型尝试，尝试解析类型
            objectinfo = getObjectInfoByType(abi::__cxa_demangle(typeid(T).name(),0,0,0), objectType);
            objectType = getObjectTypeInt(objectinfo.objectType, abi::__cxa_demangle(typeid(T).name(),0,0,0));
        }
        int sum = objectinfo.metaInfoObjectList.size();
        int i = 1;
        //获取到的objectType才是真正的类型，根据这个类型进行操作
        switch(objectType){
            case OBJECT_BASE:
                {
                    //从这里走的都是数组类型的数据，不需要一个一个等于
                    //cout << "begin base---" << abi::__cxa_demangle(typeid(object_).name(),0,0,0) << endl;
                    MetaInfo * metainfo1 = getMetaInfo(abi::__cxa_demangle(typeid(object_).name(),0,0,0));
                    FdogSerializeBase::Instance()->BaseToJsonA(json_, metainfo1, object_);
                    //removeLastComma(json_);
                }
                break;
            case OBJECT_STRUCT:
            cout << "begin struct---" << endl;
            for(auto metainfoObject : objectinfo.metaInfoObjectList){
                
                string json_s;
                if(metainfoObject->memberTypeInt == MEMBER_BASE){
                    FdogSerializeBase::Instance()->BaseToJson(json_s, metainfoObject, object_);
                    json_ = json_ + json_s;
                }
                Serialize_type_judgment_all;
                if(i == sum){
                    removeLastComma(json_);
                }
                json_s = "";
                i++;
            }
            break;
            case OBJECT_VECTOR:
                // for(int i =0; i < size; i++){
                //     //等于 NULL 说明是基础类型
                //     if(objectinfo.objectType == "NULL"){
                //         string typeName = abi::__cxa_demangle(typeid(object_).name(),0,0,0);
                //         string typeName_2;
                //         smatch result;
                //         regex pattern(complexRegex[5]);
                //         if(regex_search(typeName, result, pattern)){
                //             typeName_2 = result.str(1).c_str();
                //         }
                //         MetaInfo * metainfo1 = getMetaInfo(typeName_2);
                //         if( metainfo1->memberType == "bool"){
                //             Serialize(json_, *(bool *)(ptr + (metainfo1->memberTypeSize * i)));
                //         }
                //         if( metainfo1->memberType == "short"){
                //             Serialize(json_, *(short *)(ptr + (metainfo1->memberTypeSize * i)));
                //         }
                //         if( metainfo1->memberType == "int"){
                //             Serialize(json_, *(int *)(ptr + (metainfo1->memberTypeSize * i)));
                //         }
                //         if( metainfo1->memberType == "long"){
                //             Serialize(json_, *(long *)(ptr + (metainfo1->memberTypeSize * i)));
                //         }
                //         if( metainfo1->memberType == "long long"){
                //             Serialize(json_, *(long long *)(ptr + (metainfo1->memberTypeSize * i)));
                //         }
                //         if( metainfo1->memberType == "float"){
                //             Serialize(json_, *(float *)(ptr + (metainfo1->memberTypeSize * i)));
                //         }
                //     }else{
                //         //这里需要人为添加了
                //         // if( objectinfo.objectType == "float"){
                //         //     Serialize(json_, *(float *)(ptr + (metainfo1->memberTypeSize * i)));
                //         // }
                //     }
                // }
                // removeLastComma(json_);
                // json_ = name + ":" + "[" + json_ + "]";
            break;
            case OBJECT_MAP:
            break;
            case OBJECT_LIST:
                // T listObject = object_;
                // for(int i =0; i < size; i++){
                //     //等于 NULL 说明是基础类型
                //     if(objectinfo.objectType == "NULL"){
                //         string typeName = abi::__cxa_demangle(typeid(object_).name(),0,0,0);
                //         string typeName_2;
                //         smatch result;
                //         regex pattern(complexRegex[7]);
                //         if(regex_search(typeName, result, pattern)){
                //             typeName_2 = result.str(1).c_str();
                //         }
                //         MetaInfo * metainfo1 = getMetaInfo(typeName_2);
                //         if( metainfo1->memberType == "bool"){
                //             Serialize(json_, *(bool *)(ptr + (metainfo1->memberTypeSize * i)));
                //         }
                //         if( metainfo1->memberType == "short"){
                //             Serialize(json_, *(short *)(ptr + (metainfo1->memberTypeSize * i)));
                //         }
                //         if( metainfo1->memberType == "int"){
                //             Serialize(json_, *(int *)(ptr + (metainfo1->memberTypeSize * i)));
                //         }
                //         if( metainfo1->memberType == "long"){
                //             Serialize(json_, *(long *)(ptr + (metainfo1->memberTypeSize * i)));
                //         }
                //         if( metainfo1->memberType == "long long"){
                //             Serialize(json_, *(long long *)(ptr + (metainfo1->memberTypeSize * i)));
                //         }
                //         if( metainfo1->memberType == "float"){
                //             Serialize(json_, *(float *)(ptr + (metainfo1->memberTypeSize * i)));
                //         }
                //     }else{
                //         //这里需要人为添加了
                //         // if( objectinfo.objectType == "float"){
                //         //     Serialize(json_, *(float *)(ptr + (metainfo1->memberTypeSize * i)));
                //         // }
                //     }
                // }
                // removeLastComma(json_);
                // json_ = name + ":" + "[" + json_ + "]";
            break;
            case OBJECT_ARRAY:
            break;
            case OBJECT_CLASS:
            break;
        }
    }

    template<typename T>
    void FSerialize(string & json_, T & object_, string name = ""){
        Serialize(json_, object_, name);
        json_ = "{" + json_ + "}";
        //这里需要判断类型
    }

    //基于基础类型 for range最快
    template<typename T>
    void FSerializeV(string & json_, T & object_, string name = ""){
        for(auto & object_one : object_){
            json_ = json_ + "{";
            Serialize(json_, object_);
            json_ = json_ + "},";
        }
        removeLastComma(json_);
        json_ = "{" + name + ":[" + json_ + "]}";
    }

    template<typename T>
    void FSerializeL(string & json_, T & object_, string name = ""){
        for(auto & object_one : object_){
            //这里需要对类型做
            json_ = json_ + "{";
            Serialize(json_, object_one);
            json_ = json_ + "},";
        }
        removeLastComma(json_);
        json_ = "{" + name + ":[" + json_ + "]}";
    }

    template<typename T>
    void FSerializeS(string & json_, T & object_, string name = ""){
        for(auto & object_one : object_){
            json_ = json_ + "{";
            Serialize(json_, object_);
            json_ = json_ + "},";
        }
        removeLastComma(json_);
        json_ = "{" + name + ":[" + json_ + "]}";
    }

    template<typename T>
    void FSerializeM(string & json_, T & object_, string name = ""){
        typename T::iterator it;

        it = object_.begin();

        while(it != object_.end())
        {
            json_ = json_ + "\"" + it->first + "\"" + ":{";
            Serialize(json_, it->second);
            json_ = json_ + "},";
            it ++;         
        }
        removeLastComma(json_);
        json_ = "{" + name + ":[" + json_ + "]}";
    }

    //反序列化
    template<typename T>
    void DesSerialize(T & object_, string & json_){
        //cout << "地址：--" << &object_ << endl;
        ObjectInfo & objectinfo = FdogSerialize::Instance()->getObjectInfo(abi::__cxa_demangle(typeid(T).name(),0,0,0));
        int objectType = getObjectTypeInt(objectinfo.objectType, abi::__cxa_demangle(typeid(T).name(),0,0,0));
        for(auto metainfoObject : objectinfo.metaInfoObjectList){
            //通过正则表达式获取对应的json
            cout << "总类型：" << objectType << "--当前成员：" << metainfoObject->memberName << " 类型为：" << metainfoObject->memberType << endl;
            smatch result;
            string regex_key = "(\"" + metainfoObject->memberName +"\")";
            string regex_value = baseRegex[metainfoObject->memberType];
            if(regex_value == ""){
                if(objectType == OBJECT_STRUCT){
                    regex_value = objectRegex;
                }
                if(objectType == OBJECT_ARRAY){
                    regex_value = arrayRegex;
                }
                if(objectType == OBJECT_ARRAY){
                    regex_value = arrayRegex;
                }
            }
            regex pattern(regex_key + ":" +regex_value);
            if(regex_search(json_, result, pattern)){
                string value = result.str(2).c_str();
                if(metainfoObject->memberTypeInt == MEMBER_BASE){
                    cout << "类型：" << metainfoObject->memberType << endl;
                    FdogSerializeBase::Instance()->JsonToBase(object_, metainfoObject, value);
                }
                DesSerialize_type_judgment_all;
            }
        }
    }

    template<typename T>
    void FDesSerialize(T & object_, string & json_){
        DesSerialize(object_, json_);
    }

    template<typename T>
    void FDesSerializeV(T & object_, string & json_){
        for(auto & object_one : object_){
            
        }
    }

    template<typename T>
    void FDesSerializeL(T & object_, string & json_){
        for(auto & object_one : object_){
            
        }
    }

    template<typename T>
    void FDesSerializeM(T & object_, string & json_){
        for(auto & object_one : object_){
            
        }
    }

    template<typename T>
    void FDesSerializeS(T & object_, string & json_){
        for(auto & object_one : object_){
            
        }
    }

};

void * getInstance();

#endif