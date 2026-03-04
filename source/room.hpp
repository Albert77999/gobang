#pragma once

#include "online.hpp"
#include "db.hpp"
#include "util.hpp"
#include "logger.hpp"

#define BOARD_ROW 15
#define BOARD_COL 15
#define CHESS_WHITE 1
#define CHESS_BLACK 2

typedef enum
{
    GAME_START,
    GAME_OVER
} room_statu;

class room
{
private:
    int check_win(int row, int col, int color)
    {
        // 横行、纵列、正斜、反斜
        if (five(row, col, 0, 1, color) ||
            five(row, col, 1, 0, color) ||
            five(row, col, -1, 1, color) ||
            five(row, col, 1, 1, color))
        {
            return color == CHESS_WHITE ? _white_id : _black_id;
        }
        return 0;
    }
    bool five(int row, int col, int row_off, int col_off, int color)
    {
        // row、col是下棋的位置,row_off col_off是偏移量,也是方向
        int count = 1;
        int search_row = row + row_off;
        int search_col = col + col_off;
        while (search_row >= 0 && search_row < BOARD_ROW &&
               search_col >= 0 && search_col < BOARD_COL &&
               _board[search_row][search_col] == color)
        {
            count++;
            search_row += row_off;
            search_col += col_off;
        }
        search_row = row - row_off;
        search_col = col - col_off;
        while (search_row >= 0 && search_row < BOARD_ROW &&
               search_col >= 0 && search_col < BOARD_COL &&
               _board[search_row][search_col] == color)
        {
            count++;
            search_row -= row_off;
            search_col -= col_off;
        }
        return count >= 5;
    }

private:
    int _room_id;
    room_statu _statu;
    int _player_count;
    int _white_id;
    int _black_id;
    user_table *_tb_user;
    online_manager *_online_user;
    std::vector<std::vector<int>> _board;

public:
    room(int room_id, user_table *tb_user, online_manager *online_user)
        : _room_id(room_id), _statu(GAME_START), _player_count(0), _tb_user(tb_user), _online_user(online_user), _board(BOARD_ROW, std::vector<int>(BOARD_COL, 0))
    {
        DLOG("%d 房间创建成功", _room_id);
    }
    ~room()
    {
        DLOG("%d 房间号销毁成功", _room_id);
    }
    int id() { return _room_id; }
    room_statu statu() { return _statu; }
    int player_count() { return _player_count; }
    void add_white_user(int uid)
    {
        _white_id = uid;
        _player_count++;
    }
    void add_black_user(int uid)
    {
        _black_id = uid;
        _player_count++;
    }
    int get_white_user() { return _white_id; }
    int get_black_user() { return _black_id; }
    // 处理请求
    void handle_request(Json::Value &req)
    {
        Json::Value json_resp;
        // 1.检验房间号是否匹配
        int room_id = req["room_id"].asInt();
        if (_room_id != room_id)
        {
            json_resp["optype"] = req["optype"].asString();
            json_resp["result"] = false;
            json_resp["reason"] = "房间号不匹配";
            broadcast(json_resp);
            return;
        }
        // 2.根据不同的请求类型调用不同的函数
        if (req["optype"].asString() == "put_chess")
        {
            json_resp = handle_chess(req);
            if (json_resp["winner"].asInt() != 0)
            {
                int winner_id = json_resp["winner"].asInt();
                int loser_id = winner_id == _white_id ? _black_id : _white_id;
                _tb_user->win(winner_id);
                _tb_user->lose(loser_id);
                _statu = GAME_OVER;
            }
        }
        else if (req["optype"].asString() == "chat")
        {
            json_resp = handle_chat(req);
        }
        else
        {
            json_resp["optype"] = req["optype"].asString();
            json_resp["result"] = false;
            json_resp["reason"] = "未知请求类型";
        }
        broadcast(json_resp);
    }

    Json::Value handle_chat(Json::Value &req)
    {
        Json::Value json_resp = req;
        // 检测消息中是否包含敏感词
        std::string msg = req["message"].asString();
        size_t pos = msg.find("垃圾");
        if (pos != std::string::npos)
        {
            json_resp["result"] = false;
            json_resp["reason"] = "消息中包含敏感词,不能发送";
            return json_resp;
        }
        // 广播消息--返回消息
        json_resp["result"] = true;
        return json_resp;
    }

    Json::Value handle_chess(Json::Value &req)
    {
        Json::Value json_resp = req;
        // 判断房间中两个玩家是否都在线，任意一个不在线就是另一个胜利
        int cur_uid = req["uid"].asInt();
        int chess_row = req["row"].asInt();
        int chess_col = req["col"].asInt();
        if (_online_user->is_in_game_room(_white_id) == false)
        {
            json_resp["result"] = true;
            json_resp["reason"] = "运气真好,对方掉线,不战而胜";
            json_resp["winner"] = _black_id;
            return json_resp;
        }
        if (_online_user->is_in_game_room(_black_id) == false)
        {
            json_resp["result"] = true;
            json_resp["reason"] = "运气真好,对方掉线,不战而胜";
            json_resp["winner"] = _white_id;
            return json_resp;
        }
        // 获取走棋位置,判断当前走棋位置是否合理（位置是否已经被占用）
        if (_board[chess_row][chess_col] != 0)
        {
            json_resp["result"] = false;
            json_resp["reason"] = "当前位置已经有了其他棋子!";
            return json_resp;
        }
        // 判断是否有玩家胜利
        int cur_color = cur_uid == _white_id ? CHESS_WHITE : CHESS_BLACK;
        _board[chess_row][chess_col] = cur_color;
        int winner_id = check_win(chess_row, chess_col, cur_color);
        if (winner_id != 0)
        {
            json_resp["reason"] = "五星连珠,胜利";
        }
        json_resp["optype"] = "put_chess";
        json_resp["result"] = true;
        json_resp["winner"] = winner_id;
        return json_resp;
    }

    void handle_exit(int uid)
    {
        // 如果是下棋中退出，则对方获胜；否则就是正常退出
        Json::Value json_resp;
        if (_statu == GAME_START)
        {
            int winner_id = (uid == _white_id) ? _black_id : _white_id;
            int loser_id = uid;
            _tb_user->win(winner_id);
            _tb_user->lose(loser_id);

            json_resp["optype"] = "put_chess";
            json_resp["result"] = true;
            json_resp["reason"] = "对方掉线,不战而胜";
            json_resp["room_id"] = _room_id;
            json_resp["uid"] = uid;
            json_resp["row"] = -1;
            json_resp["col"] = -1;
            json_resp["winner"] = winner_id;
            broadcast(json_resp);
        }
        // 房间中玩家数量--
        _player_count--;
    }

    void broadcast(Json::Value &rsp)
    {
        // 1.对要响应的信息进行序列化，将Json::Value中的数据序列转化为json字符串
        std::string body;
        json_util::serialize(rsp, body);
        // 2.获取房间中所有用户的连接
        // 3.逐个进行发送响应信息
        wsserver_t::connection_ptr wccon = _online_user->get_conn_from_room(_white_id);
        if (wccon.get() != nullptr)
        {
            wccon->send(body);
        }
        wsserver_t::connection_ptr bccon = _online_user->get_conn_from_room(_black_id);
        if (bccon.get() != nullptr)
        {
            bccon->send(body);
        }
    }
};

using room_ptr = std::shared_ptr<room>;

class room_manager
{
private:
    int _next_rid;
    std::mutex _mutex;
    user_table *_tb_user;
    online_manager *_online_user;
    std::unordered_map<int,room_ptr> _rooms;
    std::unordered_map<int,int> _users;
public:
    room_manager(user_table *tb_user,online_manager *online_user)
        :_tb_user(tb_user)
        ,_online_user(online_user)
        ,_next_rid(1)
    {
        DLOG("房间管理化模块初始化完毕");
    }
    ~room_manager()
    {
        DLOG("房间管理化模块即将销毁");
    }
    room_ptr create_room(int uid1,int uid2)
    {
        //两个用户在游戏大厅中进行对战匹配，成功后创建房间
        //1.检验两个用户是否都在游戏大厅中，只有都在才需要创建房间
        if(_online_user->is_in_game_hall(uid1) == false)
            return room_ptr();
        if(_online_user->is_in_game_hall(uid2) == false)
            return room_ptr();
        //2.创建房间，将用户信息添加到房间中
        std::unique_lock<std::mutex> lock(_mutex);
        room_ptr rp = std::make_shared<room>(_next_rid,_tb_user,_online_user);
        rp->add_white_user(uid1);
        rp->add_black_user(uid2);
        //3.将房间信息管理起来
        _rooms.insert({_next_rid,rp});
        _users.insert({uid1,_next_rid});
        _users.insert({uid2,_next_rid});
        _next_rid++;
        //4.返回房间信息
        return rp;
    }
    room_ptr get_room_by_rid(int rid)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _rooms.find(rid);
        if(it == _rooms.end())
            return room_ptr();
        return it->second;
    }
    room_ptr get_room_by_uid(int uid)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        auto uit = _users.find(uid);
        if(uit == _users.end())
            return room_ptr();
        int rid = uit->second;
        auto rit = _rooms.find(rid);
        if(rit == _rooms.end())
            return room_ptr();
        return rit->second;
    }
    void remove_room(int rid)
    {
        room_ptr rp = get_room_by_rid(rid);
        if(rp.get() == nullptr)
            return;
        int uid1 = rp->get_white_user();
        int uid2 = rp->get_black_user();
        std::unique_lock<std::mutex> lock(_mutex);
        _users.erase(uid1);
        _users.erase(uid2);
        _rooms.erase(rid);
    }
    void remove_room_user(int uid)
    {
        room_ptr rp = get_room_by_uid(uid);
        if(rp.get() == nullptr)
            return;
        rp->handle_exit(uid);
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _users.erase(uid);
        }
        if(rp->player_count() == 0)
        {
            remove_room(rp->id());
        }
    }
};