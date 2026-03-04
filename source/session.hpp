#pragma once
#include <websocketpp/server.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include "logger.hpp"
#include <unordered_map>
#include <functional>
typedef websocketpp::server<websocketpp::config::asio> wsserver_t;
typedef enum
{
    UNLOGIN,
    LOGIN
} ss_statu;

class session
{
private:
    int _ssid;
    int _uid;
    ss_statu _statu;
    wsserver_t::timer_ptr _tp;
public:
    session(int ssid)
        :_ssid(ssid)
    {
        DLOG("SESSION %p 被创建!",this);
    }
    ~session()
    {
        DLOG("SESSION %p 被释放!",this);
    }
    void set_statu(ss_statu statu) {_statu = statu;}
    int ssid() {return _ssid;}
    void set_user(int uid) {_uid = uid;}
    int get_user() {return _uid;}
    bool is_login() {return _statu == LOGIN;}
    void set_timer(wsserver_t::timer_ptr tp) {_tp = tp;}
    wsserver_t::timer_ptr &get_timer() {return _tp;}
};

#define SESSION_TIMEOUT 30000
#define SESSION_FOREVER -1

using session_ptr = std::shared_ptr<session>;

class session_manager
{
private:
    int _next_ssid;
    std::mutex _mutex;
    std::unordered_map<int,session_ptr> _session;
    wsserver_t *_server;
public:
    session_manager(wsserver_t *server)
        :_next_ssid(1)
        ,_server(server)
    {
        DLOG("session管理器初始化完毕");
    }
    ~session_manager()
    {
        DLOG("session管理器即将销毁");
    }
    session_ptr create_session(ss_statu statu)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        session_ptr ssp = std::make_shared<session>(_next_ssid);
        ssp->set_statu(statu);
        _session.insert({_next_ssid,ssp});
        _next_ssid++;
        return ssp;
    }
    void append_session(const session_ptr &ssp)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _session.insert({ssp->ssid(),ssp});
    }
    session_ptr get_session_by_ssid(int ssid)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _session.find(ssid);
        if(it == _session.end())
            return session_ptr();
        return it->second;
    }
    void remove_session(int ssid)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _session.erase(ssid);
    }
    void set_session_expire_time(int ssid,int ms)
    {
        session_ptr ssp = get_session_by_ssid(ssid);
        wsserver_t::timer_ptr tp = ssp->get_timer();
        //4种情况
        //1.在session永久存在的情况下，设置永久存在
        if(tp.get() == nullptr && ms == SESSION_FOREVER)
            return;
        //2.在session永久存在的情况下，设置指定时间之后被删除的定时任务
        else if(tp.get() == nullptr && ms != SESSION_FOREVER)
        {
            wsserver_t::timer_ptr tmp_tp = _server->set_timer(ms,std::bind(&session_manager::remove_session,this,ssid));
            ssp->set_timer(tmp_tp);
        }
        //3.在session设置了定时删除的情况下，将session设置为永久存在
        else if(tp.get() != nullptr && ms == SESSION_FOREVER)
        {
            tp->cancel();
            _server->set_timer(0,std::bind(&session_manager::append_session,this,ssp));
            ssp->set_timer(wsserver_t::timer_ptr());
        }
        else
        {
            tp->cancel();
            ssp->set_timer(wsserver_t::timer_ptr());
            _server->set_timer(0,std::bind(&session_manager::append_session,this,ssp));
            wsserver_t::timer_ptr tmp_tp = _server->set_timer(ms,std::bind(&session_manager::remove_session,this,ssid));
            ssp->set_timer(tmp_tp);
        }
    }
};