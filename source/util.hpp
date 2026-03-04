#pragma once
#include "logger.hpp"
#include <mysql/mysql.h>
#include <jsoncpp/json/json.h>
#include <sstream>
#include <string>
#include <memory>
#include <iostream>
#include <vector>
#include <fstream>
class mysql_util
{
public:
    static MYSQL *mysql_create(const std::string &host,
                               const std::string &username,
                               const std::string &password,
                               const std::string &dbname,
                               uint16_t port = 3306)
    {
        // 1.初始化mysql句柄
        MYSQL *mysql = mysql_init(NULL);
        if (mysql == NULL)
        {
            ELOG("mysql init failed");
            return NULL;
        }
        // 2.连接服务器
        if (mysql_real_connect(mysql, host.c_str(), username.c_str(), password.c_str(), dbname.c_str(), port, NULL, 0) == NULL)
        {
            ELOG("connect mysql server failed :%s", mysql_error(mysql));
            mysql_close(mysql);
            return NULL;
        }
        // 3.设置客户端字符集
        if (mysql_set_character_set(mysql, "utf8") != 0)
        {
            ELOG("set client character failed:%s", mysql_error(mysql));
            mysql_close(mysql);
            return NULL;
        }
        return mysql;
    }
    static bool mysql_exec(MYSQL *mysql, const std::string &sql)
    {
        // 5.执行sql语句
        int ret = mysql_query(mysql, sql.c_str());//线程安全009
        if (ret != 0)
        {
            ELOG("%s", sql.c_str());
            ELOG("mysql query failed:%s", mysql_error(mysql));
            //mysql_close(mysql);
            return false;
        }
        // // 6.如果sql语句是查询语句，则需要保存结果到本地
        // MYSQL_RES *res = mysql_store_result(mysql);
        // if (res == NULL)
        // {
        //     mysql_close(mysql);
        //     return false;
        // }
        return true;
    }
    static void mysql_destroy(MYSQL *mysql)
    {
        if (mysql != NULL)
        {
            mysql_close(mysql);
        }
        return;
    }
};

class json_util
{
public:
    static bool serialize(const Json::Value &root, std::string &str)
    {
        // 2.实例化一个StreamWriterBuilder工厂类对象
        Json::StreamWriterBuilder swb;
        swb["emitUTF8"] = true;
        // 3.通过StreamWriterBuilder工厂类对象，生产一个StreamWriter对象
        std::unique_ptr<Json::StreamWriter> sw(swb.newStreamWriter());
        // 4.使用StreamWriter对象，对Json::Value中存储的数据进行序列化
        std::stringstream ss;
        int ret = sw->write(root, &ss);
        if (ret != 0)
        {
            ELOG("Serialize failed");
            return false;
        }

        str = ss.str();

        return true;
    }
    static bool deserialize(const std::string &str, Json::Value &root)
    {
        // 1.实例化一个CharReaderBuilder工厂类对象
        Json::CharReaderBuilder crb;
        // 2.使用CharReaderBuilder工厂类生产一个CharReader对象
        std::unique_ptr<Json::CharReader> cr(crb.newCharReader());
        // 3.定义一个Json::Value对象存储解析后的数据
        std::string err;
        // 4.使用CharReader对象进行json格式字符串的str的反序列化
        bool ret = cr->parse(str.c_str(), str.c_str() + str.size(), &root, &err);
        if (ret == false)
        {
            ELOG("Json deseriliaze failed:%s",err.c_str());
            return false;
        }
        // // 5.逐个元素地访问Json::Value中的数据
        // std::cout << "姓名" << root["姓名"].asString() << std::endl;
        // std::cout << "年龄" << root["年龄"].asInt() << std::endl;
        // int size = root["成绩"].size();
        // for (int i = 0; i < size; i++)
        // {
        //     std::cout << "成绩" << root["成绩"][i].asFloat() << std::endl;
        // }
        return true;
    }
};

class string_util
{
public:
    static int split(const std::string &src,const std::string &sep,std::vector<std::string> &res)
    {
        //123,345,,,456
        size_t pos,idx = 0;
        while(idx < src.size())
        {
            pos = src.find(sep,idx);
            if(pos == std::string::npos)
            {
                //没有找到，字符串中没有间隔字符了，跳出循环
                res.push_back(src.substr(idx));
                break;
            }
            if(pos == idx) 
            {
                idx += sep.size();
                continue;
            }
            res.push_back(src.substr(idx,pos - idx));
            idx = pos + sep.size();
        }
        return res.size();
    }
};

class file_util
{
public:
    static bool read(const std::string &filename,std::string &body)
    {
        //1.打开文件
        std::ifstream ifs(filename,std::ios::binary);
        if(ifs.is_open() == false)
        {
            ELOG("%s file open failed",filename.c_str());
            return false;
        }
        //2.获取文件大小
        size_t fsize = 0;
        ifs.seekg(0,std::ios::end);
        fsize = ifs.tellg();
        ifs.seekg(0,std::ios::beg);
        body.resize(fsize);
        //3.将文件所有数据读取出来
        ifs.read(&body[0],fsize);
        if(ifs.good() == false)
        {
            ELOG("read %s file failed",filename.c_str());
            ifs.close();
            return false;
        }
        //4.关闭文件
        ifs.close();
        return true;
    }
};