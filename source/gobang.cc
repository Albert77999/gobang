#include "room.hpp"
#include "session.hpp"
#include "matcher.hpp"

#define HOST "127.0.0.1"
#define USER "root"
#define PASS "Ch_031014"
#define DBNAME "gobang"

void mysql_test()
{
    MYSQL *mysql = mysql_util::mysql_create(HOST, USER, PASS, DBNAME);
    if (!mysql)
        return ;
    const char *sql = "insert into stu values(null,'小明',18,88,56,76);";
    bool ret = mysql_util::mysql_exec(mysql, sql);
    if (ret == false)
        return ;
    mysql_util::mysql_destroy(mysql);
}

void json_test()
{
    Json::Value root;
    std::string body;
    root["姓名"] = "小明";
    root["年龄"] = 18;
    root["成绩"].append(98);
    root["成绩"].append(88.5);
    root["成绩"].append(82);
    json_util::serialize(root,body);
    DLOG("%s",body.c_str());
    Json::Value val;
    json_util::deserialize(body,val);
    //5.逐个元素地访问Json::Value中的数据
    std::cout<<"姓名"<<root["姓名"].asString()<<std::endl;
    std::cout<<"年龄"<<root["年龄"].asInt()<<std::endl;
    int size = root["成绩"].size();
    for(int i = 0;i < size;i++)
    {
        std::cout<<"成绩"<<root["成绩"][i].asFloat()<<std::endl;
    }
}
void str_test()
{
    std::string str = "123,232,,,456";
    std::vector<std::string> arr;
    string_util::split(str,",",arr);
    for(auto e : arr)
        DLOG("%s",e.c_str());
}
void file_test()
{
    std::string filename = "./Makefile";
    std::string body;
    file_util::read(filename,body);
    std::cout<<body<<std::endl;
}
void db_test()
{
    user_table ut(HOST, USER, PASS, DBNAME);
    Json::Value user;
    // user["username"] = "xiaoming";
    // user["password"] = "123123";
    ut.win(3);
    bool ret = ut.select_by_id(3,user);
    std::string body;
    json_util::serialize(user,body);
    std::cout<<body<<std::endl;
}
void online_test()
{
    online_manager om;
    int id = 2;
    wsserver_t::connection_ptr conn;
    om.enter_game_hall(id,conn);
    if(om.is_in_game_hall(id))
    {
        DLOG("in game hall");
    }
    else
    {
        DLOG("not in game hall");
    }
    om.exit_game_hall(id);
    if(om.is_in_game_hall(id))
    {
        DLOG("in game hall");
    }
    else
    {
        DLOG("not in game hall");
    }
}
int main()
{
    user_table ut(HOST,USER,PASS,DBNAME,3306);
    online_manager om;
    room_manager rm(&ut,&om);
    matcher mt(&rm,&ut,&om);
    return 0;
}