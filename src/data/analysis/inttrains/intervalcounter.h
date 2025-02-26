﻿#pragma once

#include "intervaltraininfo.h"
#include <vector>
#include <QString>
#include <QRegularExpression>

class StationName;
class Railway;
class TrainCollection;
class RailStation;
/**
 * @brief The IntervalCounter class
 * 2022.04.14 区间对数统计
 * pyETRC.data.Graph.getIntervalCount_faster()
 * 这里用类来实现，参照GreedyPainter，
 * 主要目的是防止在核心数据类Railway/Diagram里面塞太多东西。
 *
 * 各方法原则上按照函数设计；类里面仅包含一些配置数据。
 * 整个功能逻辑暂时参照pyETRC设计。
 */

class TrainFilterCore;
class IntervalCounter
{
    const TrainCollection& coll;
    bool _businessOnly=false,_stopOnly=false;
    // 这两个仅用来处理区间对数表
    bool _passenterOnly=false,_freightOnly=false;
    const TrainFilterCore& _filter;
    // 区间车次表多选车站
    bool _multiStart=false, _multiEnd=false;
    bool _regexStart = false, _regexEnd = false;
public:
    IntervalCounter(const TrainCollection& coll, const TrainFilterCore& filter);
    const auto& filter()const{return _filter;}
    bool businessOnly()const{return _businessOnly;}
    bool stopOnly()const{return _stopOnly;}
    void setBusinessOnly(bool on){_businessOnly=on;}
    void setStopOnly(bool on){_stopOnly=on;}
    void setPassengerOnly(bool on){_passenterOnly=on;}
    void setFreightOnly(bool on){_freightOnly=on;}
    void setMultiStart(bool on) { _multiStart = on; }
    void setMultiEnd(bool on) { _multiEnd = on; }
    void setRegexStart(bool on) { _regexStart = on; }
    void setRegexEnd(bool on) { _regexEnd = on; }

    /**
     * @brief getIntervalTrains
     * pyETRC.data.Graph.getIntervalTrains()
     * 区间车次表。注意算法变了。现在依赖于TrainAdapter计算。
     * 此版本和pyETRC的一致，只考虑同一条线路的区间，
     * 好处是可以根据Adapter截断，并且只需要按Adapter顺序遍历一次。
     *
     * 注意：要求Adapter中的TrainLine是按顺序排列的；
     * 每个车次仅考虑一组情况：最后一次出现的from和第一个出现的to之间的。
     * 反复横跳的，暂时不管。
     */
    IntervalTrainList getIntervalTrains(
            std::shared_ptr<const Railway> rail,
            std::shared_ptr<const RailStation> from,
            std::shared_ptr<const RailStation> to);

    /**
     * @brief getIntervalTrains
     * 区间车次表（全局）
     * 仅用站名判定的版本。需要全局遍历列车时刻表，而不管线路问题。
     * 暂定使用equalOrBelongTo()判定站名，即支持域解析符
     * 2022.04.24：入参改为QString，考虑多车站选择情况
     */
    IntervalTrainList getIntervalTrains(
            const QString& from,
            const QString& to
            );

    /**
     * @brief getIntervalCount  pyETRC.data.Graph.getIntervalCount_faster
     * 获取指定线路的区间对数统计信息。
     * 与pyETRC不同的是，这里同时返回所有的车次。
     * @param rail  线路
     * @param center  中心站：作为发站或者到站
     * @return
     */
    RailIntervalCount getIntervalCountSource(
            std::shared_ptr<const Railway> rail,
            std::shared_ptr<const RailStation> source
            )const;

    /**
     * @brief getIntervalCountDrain
     * 以给定站为到站的版本，实际上就是反向遍历
     * @param rail
     * @param drain
     * @return
     */
    RailIntervalCount getIntervalCountDrain(
            std::shared_ptr<const Railway> rail,
            std::shared_ptr<const RailStation> drain
            )const;

    /**
     * 确定指定站是否要符合办客站/办货站限制
     * （是否要显示出来）
     */
    bool checkStation(const std::shared_ptr<const RailStation>& st)const;

private:
    /**
     * @brief checkStopBusiness
     * 判定某待生成的事件是否符合营业站/始发终到站的约束。
     */
    bool checkStopBusiness(const IntervalTrainInfo& info)const;

    bool checkStationStopBusiness(const TrainStation& st, bool isStartEnd);

    bool checkStationName(const StationName& name, const std::vector<QRegularExpression>& std_names, bool useReg)const;

    /**
     * 2022.05.06：改为正则表达式的列表。
     * 如果不启用正则，就直接按pattern()解释为站名。
     */
    std::vector<QRegularExpression> transSearchStation(const QString& input, bool useMulti)const;

};

