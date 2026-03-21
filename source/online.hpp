#pragma once
#include "util.hpp"

#include <mutex>
#include <unordered_map>
#include <websocketpp/server.hpp>
#include <websocketpp/config/asio_no_tls.hpp>

typedef websocketpp::server<websocketpp::config::asio> wsserver_t;

class online_manager
{
private:
    std::mutex _mutex;
    // 用于建立游戏大厅用户的用户ID与通信连接的关系
    std::unordered_map<int, wsserver_t::connection_ptr> _hall_user;
    // 用于建立游戏房间用户的用户ID与通信连接的关系
    std::unordered_map<int, wsserver_t::connection_ptr> _room_user;

public:
    // websocket连接建立的时候才会加入游戏大厅 & 游戏房间在线用户管理
    void enter_game_hall(int uid, wsserver_t::connection_ptr &conn)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _hall_user[uid] = conn;
    }
    void enter_game_room(int uid, wsserver_t::connection_ptr &conn)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _room_user[uid] = conn;
    }
    // websocket连接断开的时候才会移除游戏大厅 & 游戏房间在线用户管理
    void exit_game_hall(int uid)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _hall_user.erase(uid);
    }
    void exit_game_room(int uid)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _room_user.erase(uid);
    }
    // 判断当前用户是否在游戏大厅/游戏房间
    bool is_in_game_hall(int uid)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _hall_user.find(uid);
        if (it == _hall_user.end())
            return false;
        return true;
    }
    bool is_in_game_room(int uid)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _room_user.find(uid);
        if (it == _room_user.end())
            return false;
        return true;
    }
    // 通过用户ID在游戏大厅/游戏房间用户管理中获取对应的通信连接
    wsserver_t::connection_ptr get_conn_from_hall(int uid)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _hall_user.find(uid);
        if(it == _hall_user.end())
            return wsserver_t::connection_ptr();
        return it->second;
    }
    wsserver_t::connection_ptr get_conn_from_room(int uid)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _room_user.find(uid);
        if(it == _room_user.end())
            return wsserver_t::connection_ptr();
        return it->second;
    }
};