#pragma once

#include "room.hpp"
#include "session.hpp"
#include "matcher.hpp"
#include "db.hpp"
#include "online.hpp"
#include "util.hpp"

#define WWWROOT "./wwwroot/" 

class gobang_server
{
private:
    std::string _web_root; // 静态资源根目录 /register.html
    wsserver_t _wssrv;
    online_manager _om;
    user_table _ut;
    room_manager _rm;
    matcher _mm;
    session_manager _sm;
private:
    void http_callback(websocketpp::connection_hdl hdl)
    {
        std::string filename = "register.html";
        std::string pathname = _web_root + filename;
        std::string body;
        file_util::read(pathname,body);
        wsserver_t::connection_ptr conn = _wssrv.get_con_from_hdl(hdl);
        conn->set_status(websocketpp::http::status_code::ok);
        conn->append_header("Content-Type","text/html");
        conn->append_header("Content-Length",std::to_string(body.size()));
        conn->set_body(body);
    }
    void wsopen_callback(websocketpp::connection_hdl hdl)
    {
        std::cout << "websocket握手成功";
    }
    void wsclose_callback(websocketpp::connection_hdl hdl)
    {
        std::cout << "websocket断开成功";
    }
    void wsmsg_callback(websocketpp::connection_hdl hdl, wsserver_t::message_ptr msg)
    {
       
    }
public:
    // 进行成员初始化以及服务器回调函数的设置
    gobang_server(const std::string &host,
               const std::string &username,
               const std::string &password,
               const std::string &dbname,
               uint16_t port = 3306,
               const std::string &wwwroot = WWWROOT)
            :_web_root(wwwroot)
            ,_ut(host,username,password,dbname,port)
            ,_rm(&_ut,&_om)
            ,_mm(&_rm,&_ut,&_om)
            ,_sm(&_wssrv)
    {
        // 设置日志等级
        _wssrv.set_access_channels(websocketpp::log::alevel::none);
        // 初始化asio调度器
        _wssrv.init_asio();
        _wssrv.set_reuse_addr(true);
        // 设置回调函数
        _wssrv.set_http_handler(std::bind(&gobang_server::http_callback, this, std::placeholders::_1));
        _wssrv.set_open_handler(std::bind(&gobang_server::wsopen_callback, this, std::placeholders::_1));
        _wssrv.set_close_handler(std::bind(&gobang_server::wsclose_callback, this, std::placeholders::_1));
        _wssrv.set_message_handler(std::bind(&gobang_server::wsmsg_callback, this, std::placeholders::_1, std::placeholders::_2));
    }
    void start(int port) // 启动服务器
    {
        // 设置监听端口
        _wssrv.listen(port);
        // 开始获取新连接
        _wssrv.start_accept();
        // 启动服务器
        _wssrv.run();
    }
};