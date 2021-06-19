﻿/*
 * TrainStation
 * 对应原pyETRC用于表示车站的dict
 */
#pragma once

#include <memory>
#include <utility>

#include <QTime>
#include <QDebug>

#include "data/common/stationname.h"
#include "data/rail/railstation.h"

class TrainStation
{
    /*
     * 新增 绑定到线路的车站
     */
    std::weak_ptr<RailStation> _railStation;
public:
    StationName name;
    QTime arrive,depart;
    bool business;    //是否办理业务
    QString track;
    QString note;
    TrainStation(const StationName& name_, const QTime& arrive_,
        const QTime& depart_, bool business_ = true,
        const QString& track_ = "", const QString& note_="");
    explicit TrainStation(const QJsonObject& obj);

    void fromJson(const QJsonObject& obj);
    QJsonObject toJson()const;

    inline bool isStopped()const {
        return arrive == depart;
    }

    inline std::weak_ptr<const RailStation> boundRailStation()const {
        return _railStation;
    }

    inline std::weak_ptr<RailStation> boundRailStation() {
        return _railStation;
    }

    inline void bindToRailStation(std::weak_ptr<RailStation> st) {
        _railStation = st;
    }

    inline bool isBoundToRail()const {
        return !_railStation.expired();
    }
    
    int stopSec()const;
};

QDebug operator<<(QDebug debug, const TrainStation& ts);

