#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include <jsoncpp/json/json.h>

//使用jsoncpp库进行多个数据对象的序列化

std::string serilaize()
{
    //1.将需要进行序列化的数据，存储在Json::Value对象中
    Json::Value root;
    root["姓名"] = "小明";
    root["年龄"] = 18;
    root["成绩"].append(98);
    root["成绩"].append(88.5);
    root["成绩"].append(82);
    //2.实例化一个StreamWriterBuilder工厂类对象
    Json::StreamWriterBuilder swb;
    swb["emitUTF8"] = true;
    //3.通过StreamWriterBuilder工厂类对象，生产一个StreamWriter对象
    Json::StreamWriter *sw = swb.newStreamWriter();
    //4.使用StreamWriter对象，对Json::Value中存储的数据进行序列化
    std::stringstream ss;
    int ret = sw->write(root,&ss);
    if(ret != 0)
    {
        std::cout<<"Serialize failed"<<std::endl;
        exit(-1);
    }
    std::cout<<ss.str()<<std::endl;
    delete sw;
    return ss.str();
}

void deserialize(const std::string &str)
{
    //1.实例化一个CharReaderBuilder工厂类对象
    Json::CharReaderBuilder crb;
    //2.使用CharReaderBuilder工厂类生产一个CharReader对象
    Json::CharReader *cr = crb.newCharReader();
    //3.定义一个Json::Value对象存储解析后的数据
    Json::Value root;
    std::string err;
    //4.使用CharReader对象进行json格式字符串的str的反序列化
    bool ret = cr->parse(str.c_str(),str.c_str() + str.size(),&root,&err);
    if(ret == false)
    {
        std::cout<<"Json deseriliaze failed, "<<"err: "<<err;
        return;
    }
    //5.逐个元素地访问Json::Value中的数据
    std::cout<<"姓名"<<root["姓名"].asString()<<std::endl;
    std::cout<<"年龄"<<root["年龄"].asInt()<<std::endl;
    int size = root["成绩"].size();
    for(int i = 0;i < size;i++)
    {
        std::cout<<"成绩"<<root["成绩"][i].asFloat()<<std::endl;
    }
    delete cr;
}   

int main()
{
    std::string str = serilaize();
    deserialize(str);
    return 0;
}