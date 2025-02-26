﻿#include "intervalcounter.h"
#include <optional>

#include <data/train/trainfiltercore.h>
#include <data/train/traincollection.h>
#include <data/train/train.h>
#include <data/diagram/trainadapter.h>
#include <data/diagram/trainline.h>
#include <data/rail/railstation.h>

IntervalCounter::IntervalCounter(const TrainCollection &coll,
                                 const TrainFilterCore& filter):
    coll(coll), _filter(filter)
{

}

IntervalTrainList IntervalCounter::getIntervalTrains(
        std::shared_ptr<const Railway> rail,
        std::shared_ptr<const RailStation> from,
        std::shared_ptr<const RailStation> to)
{
    IntervalTrainList res{};
    foreach(auto train, coll.trains()){
        if (! _filter.check(train)){
            continue;
        }
        auto adp=train->adapterFor(*rail);
        if (!adp) continue;
        std::optional<std::deque<AdapterStation>::iterator> start_itr=std::nullopt;
        bool start_is_starting=false;
        foreach(auto line, adp->lines()){
            for(auto itr=line->stations().begin();
                itr!=line->stations().end();++itr){
                if (itr->railStation.lock() == from){
                    start_itr=itr;
                    start_is_starting=line->isStartingStation(itr);
                }else if(start_itr.has_value() &&
                         itr->railStation.lock() == to){
                    // 结束
                    IntervalTrainInfo info(train, &*(start_itr.value()->trainStation),
                                           &*(itr->trainStation),start_is_starting,
                                           line->isTerminalStation(itr));
                    if (checkStopBusiness(info)){
                        res.emplace_back(std::move(info));
                        start_itr=std::nullopt;
                    }
                }
            }
        }
    }
    return res;
}

IntervalTrainList IntervalCounter::getIntervalTrains(
        const QString &from, const QString &to)
{
    auto search_start = transSearchStation(from, _multiStart), search_end = transSearchStation(to, _multiEnd);
    IntervalTrainList res{};
    foreach(auto train,coll.trains()){
        if (!_filter.check(train))
            continue;
        const TrainStation* start_station=nullptr;
        bool start_is_starting=false;
        for(auto itr=train->timetable().begin();
            itr!=train->timetable().end();++itr){
            //if (itr->name.equalOrBelongsTo(from)){
            if (checkStationName(itr->name, search_start, _regexStart)){
                // 2022.04.24：允许多车站后，替代需要条件
                // 如果上一站满足停车和营业条件但本站不满足，不替换；否则替换
                auto* this_station = &*itr;
                bool this_is_starting = train->isStartingStation(itr->name);
                if (!start_station || !checkStationStopBusiness(*start_station, start_is_starting) ||
                    checkStationStopBusiness(*this_station, this_is_starting)) {
                    start_station = this_station;
                    start_is_starting = this_is_starting;
                }
            }else if(start_station &&
                    checkStationName(itr->name, search_end, _regexEnd)){    
                IntervalTrainInfo info(
                            train, start_station, &*itr, start_is_starting,
                            train->isTerminalStation(itr->name));
                if (checkStopBusiness(info)){
                    res.emplace_back(std::move(info));
                    start_station=nullptr;
                }
            }
        }
    }
    return res;
}

RailIntervalCount IntervalCounter::getIntervalCountSource(
        std::shared_ptr<const Railway> rail,
        std::shared_ptr<const RailStation> center) const
{
    RailIntervalCount res{};
    foreach(auto train,coll.trains()){
        auto adp=train->adapterFor(*rail);
        if (!adp) continue;
        const TrainStation* center_station=nullptr;
        bool center_is_start_or_end=false;
        foreach(auto line,adp->lines()){
            if (!_filter.check(train))
                continue;
            // 注意循环到的车站其实都是本线的，和PyETRC不一样。
            // 只要分清楚前后站即可，由center_station控制。
            for(auto itr=line->stations().begin();
                itr!=line->stations().end();++itr){
                if (itr->railStation.lock()==center){
                    center_station=&*(itr->trainStation);
                    center_is_start_or_end=line->isStartingStation(itr);

                }else{
                    // 非中心站
                    if (center_station
                            && checkStation(itr->railStation.lock())){
                        // 找到start->end的对
                        IntervalTrainInfo info(train,center_station,
                                               &*(itr->trainStation),
                                               center_is_start_or_end,
                                               line->isTerminalStation(itr));
                        if(checkStopBusiness(info)){
                            res[itr->railStation.lock()].add(std::move(info));
                        }
                    }
                }
            }
        }
    }
    return res;
}

RailIntervalCount IntervalCounter::getIntervalCountDrain(std::shared_ptr<const Railway> rail, std::shared_ptr<const RailStation> drain) const
{
    RailIntervalCount res{};
    foreach(auto train,coll.trains()){
        auto adp=train->adapterFor(*rail);
        if (!adp) continue;
        const TrainStation* center_station=nullptr;
        bool center_is_start_or_end=false;

        for (auto lineit=adp->lines().rbegin();
             lineit!=adp->lines().rend();++lineit){
            if (!_filter.check(train))
                continue;
            auto line=*lineit;
            for(auto itr=line->stations().rbegin();
                itr!=line->stations().rend();++itr){

                if (itr->railStation.lock()==drain){
                    center_station=&*(itr->trainStation);
                    center_is_start_or_end=line->isTerminalStation(&*itr);
                }else{
                    // 非中心站
                    if (center_station
                            && checkStation(itr->railStation.lock())){
                        // 找到start->end的对
                        IntervalTrainInfo info(train,
                                               &*(itr->trainStation),
                                               center_station,
                                               line->isStartingStation(&*itr),
                                               center_is_start_or_end);
                        if(checkStopBusiness(info)){
                            res[itr->railStation.lock()].add(std::move(info));
                        }
                    }
                }
            }
        }
    }
    return res;
}

bool IntervalCounter::checkStopBusiness(const IntervalTrainInfo &info) const
{
    return
       (!_stopOnly || (
            (info.from->isStopped() || info.isStarting) &&
            (info.to->isStopped() || info.isTerminal))) &&
       (!_businessOnly ||
        (info.from->business && info.to->business));
}

bool IntervalCounter::checkStationStopBusiness(const TrainStation& st, bool isStartEnd)
{
    return (!_stopOnly || (st.isStopped() || isStartEnd)) &&
        (!_businessOnly || st.business);
}

bool IntervalCounter::checkStationName(const StationName& name, 
    const std::vector<QRegularExpression>& std_names, bool useReg) const
{
    for (const auto& n : std_names) {
        if (useReg) {
            auto mat = n.match(name.toSingleLiteral());
            if (mat.hasMatch())
                return true;
        }
        else {
            if (name.equalOrBelongsTo(n.pattern()))
                return true;
        }
        
    }
    return false;
}

std::vector<QRegularExpression> IntervalCounter::transSearchStation(const QString& input, bool useMulti) const
{
    std::vector<QRegularExpression> res{};
    if (useMulti) {
        auto spt = input.split('|');
        foreach(const auto & s, spt) {
            res.emplace_back(s);
        }
    }
    else {
        res.emplace_back(input);
    }
    return res;
}

bool IntervalCounter::checkStation(const std::shared_ptr<const RailStation> &st) const
{
    return
            (!_passenterOnly || st->passenger) &&
            (!_freightOnly || st->freight);
}
