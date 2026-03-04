#pragma once
#include "util.hpp"
#include <mutex>
#include <assert.h>

class user_table
{
private:
    MYSQL *_mysql;
    std::mutex _mutex;

public:
    user_table(const std::string &host,
               const std::string &username,
               const std::string &password,
               const std::string &dbname,
               uint16_t port = 3306)
    {
        _mysql = mysql_util::mysql_create(host, username, password, dbname,port);
        assert(_mysql);
    }
    ~user_table()
    {
        mysql_util::mysql_destroy(_mysql);
    }
    bool insert(Json::Value &user)
    {
#define INSERT "insert into user values(null,'%s',sha2('%s',256),1000,0,0);"
        if (user["username"].isNull() || user["password"].isNull())
        {
            DLOG("password or username error");
            return false;
        }
        char sql[4096] = {0};
        sprintf(sql, INSERT, user["username"].asCString(), user["password"].asCString());
        bool ret = mysql_util::mysql_exec(_mysql, sql);
        if (ret == false)
        {
            DLOG("insert value failed");
            return false;
        }
        return true;
    }
    bool login(Json::Value &user)
    {
#define LOGINTO "select id,score,total_count,win_count from user where username='%s'and password=SHA2('%s',256);"
        if (user["username"].isNull() || user["password"].isNull())
        {
            DLOG("password or username error");
            return false;
        }
        char sql[4096] = {0};
        sprintf(sql, LOGINTO, user["username"].asCString(), user["password"].asCString());

        MYSQL_RES *res = NULL;
        {
            std::unique_lock<std::mutex> lock(_mutex);

            bool ret = mysql_util::mysql_exec(_mysql, sql);
            if (ret == false)
            {
                DLOG("login failed");
                return false;
            }
            res = mysql_store_result(_mysql);
        }
        if(res == NULL)
        {
            DLOG("no result in mysql");
            return false;
        }
        int row_num = mysql_num_rows(res);
        if (row_num != 1)
        {
            DLOG("user has same row");
            mysql_free_result(res);
            return false;
        }
        MYSQL_ROW row = mysql_fetch_row(res);
        user["id"] = std::stoi(row[0]);
        user["score"] = std::stoi(row[1]);
        user["total_count"] = std::stoi(row[2]);
        user["win_count"] = std::stoi(row[3]);
        mysql_free_result(res);
        return true;
    }
    bool select_by_name(const std::string &name, Json::Value &user)
    {
#define SELECT_BY_NAME "select id,score,total_count,win_count from user where username = '%s';"
        char sql[4096];
        sprintf(sql, SELECT_BY_NAME, name.c_str());
        bool ret = mysql_util::mysql_exec(_mysql, sql);
        if (ret == false)
        {
            DLOG("select by name failed");
            return false;
        }
        MYSQL_RES *res = mysql_store_result(_mysql);
        if(res == NULL)
        {
            DLOG("no result in mysql");
            return false;
        }
        int row_num = mysql_num_rows(res);
        if (row_num != 1)
        {
            DLOG("user has same row");
            mysql_free_result(res);
            return false;
        }
        MYSQL_ROW row = mysql_fetch_row(res);
        user["username"] = name;
        user["id"] = std::stoi(row[0]);
        user["score"] = std::stoi(row[1]);
        user["total_count"] = std::stoi(row[2]);
        user["win_count"] = std::stoi(row[3]);
        mysql_free_result(res);
        return true;
    }
    bool select_by_id(const int id, Json::Value &user)
    {
#define SELECT_BY_ID "select username,score,total_count,win_count from user where id = %d;"
        char sql[4096];
        sprintf(sql, SELECT_BY_ID, id);
        bool ret = mysql_util::mysql_exec(_mysql, sql);
        if (ret == false)
        {
            DLOG("select by id failed");
            return false;
        }
        MYSQL_RES *res = mysql_store_result(_mysql);
        if(res == NULL)
        {
            DLOG("no result in mysql");
            return false;
        }
        int row_num = mysql_num_rows(res);
        if (row_num != 1)
        {
            DLOG("user has same row");
            mysql_free_result(res);
            return false;
        }
        MYSQL_ROW row = mysql_fetch_row(res);
        user["id"] = id;
        user["username"] = row[0];
        user["score"] = std::stoi(row[1]);
        user["total_count"] = std::stoi(row[2]);
        user["win_count"] = std::stoi(row[3]);
        mysql_free_result(res);
        return true;
    }
    bool win(const int id)
    {
#define UPDATE_WIN "update user set score=score+30,total_count=total_count+1,win_count=win_count+1 where id=%d;"
        char sql[4096];
        sprintf(sql, UPDATE_WIN, id);
        bool ret = mysql_util::mysql_exec(_mysql, sql);
        if (ret == false)
        {
            DLOG("win failed");
            return false;
        }
        return true;
    }
    bool lose(const int id)
    {
#define UPDATE_LOSE "update user set score=score-30,total_count=total_count+1 where id=%d;"
        char sql[4096];
        sprintf(sql, UPDATE_LOSE, id);
        bool ret = mysql_util::mysql_exec(_mysql, sql);
        if (ret == false)
        {
            DLOG("lose failed");
            return false;
        }
        return true;
    }
};