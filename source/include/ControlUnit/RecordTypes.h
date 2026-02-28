#pragma once

#include <string>
#include <vector>

#include <meojson/json.hpp>

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

enum class RecordType
{
    invalid,
    connect,
    click,
    swipe,
    multi_swipe,
    touch_down,
    touch_move,
    touch_up,
    click_key,
    input_text,
    screencap,
    start_app,
    stop_app,
    key_down,
    key_up,
    scroll,
};

struct RecordLine
{
    RecordType type = RecordType::invalid;
    int64_t timestamp = 0;
    bool success = false;
    int cost = 0;

    MEO_JSONIZATION(type, timestamp, success, cost);
};

struct RecordConnect
{
    std::string version;
    std::string uuid;
    int width = 0;
    int height = 0;

    MEO_JSONIZATION(version, uuid, MEO_OPT width, MEO_OPT height);
};

struct RecordClick
{
    int x = 0;
    int y = 0;

    MEO_JSONIZATION(x, y);
};

struct RecordSwipe
{
    int x1 = 0;
    int y1 = 0;
    int x2 = 0;
    int y2 = 0;
    int duration = 0;
    int starting = 0;

    MEO_JSONIZATION(x1, y1, x2, y2, duration, MEO_OPT starting);
};

struct RecordMultiSwipe
{
    std::vector<RecordSwipe> swipes;

    MEO_JSONIZATION(swipes);
};

struct RecordTouch
{
    int contact = 0;
    int x = 0;
    int y = 0;
    int pressure = 0;

    MEO_JSONIZATION(contact, x, y, pressure);
};

struct RecordKey
{
    int keycode = 0;

    MEO_JSONIZATION(keycode);
};

struct RecordInputText
{
    std::string text;

    MEO_JSONIZATION(MEO_KEY("input_text") text);
};

struct RecordApp
{
    std::string package;

    MEO_JSONIZATION(package);
};

struct RecordScreencap
{
    std::string path;

    MEO_JSONIZATION(path);
};

struct RecordScroll
{
    int dx = 0;
    int dy = 0;

    MEO_JSONIZATION(dx, dy);
};

template <typename ParamT>
inline json::value make_record_json(const RecordLine& line, const ParamT& param)
{
    json::value j = line;
    json::value pj = param;
    for (auto& [k, v] : pj.as_object()) {
        j[k] = std::move(v);
    }
    return j;
}

inline json::value make_record_json(const RecordLine& line)
{
    return json::value(line);
}

MAA_CTRL_UNIT_NS_END
