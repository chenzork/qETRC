﻿#include "train.h"
#include <QDebug>

#include "data/rail/rail.h"
#include "data/diagram/trainline.h"
#include "data/diagram/trainadapter.h"
#include "data/train/traintype.h"


Train::Train(const TrainName &trainName,
             const StationName &starting,
             const StationName &terminal,
             TrainPassenger passenger):
    _trainName(trainName),_starting(starting),_terminal(terminal),
    _passenger(passenger),_show(true),_autoLines(true)
{

}

Train::Train(const QJsonObject &obj, TypeManager& manager)
{
    fromJson(obj, manager);
}

Train::Train(const Train& another):
    _trainName(another._trainName),_starting(another._starting),_terminal(another._terminal),
    _type(another._type),_pen(another._pen),
    _passenger(another._passenger),_show(another._show),
    _timetable(another._timetable),
    _autoLines(another._autoLines)
{
    //TrainAdapter not copied
}

Train::Train(Train&& another) noexcept:
    _trainName(std::move(another._trainName)), _starting(std::move(another._starting)),
    _terminal(std::move(another._terminal)),
    _type(std::move(another._type)), _pen(std::move(another._pen)),
    _passenger(another._passenger), _show(another._show),
    _timetable(std::move(another._timetable)),
    _autoLines(another._autoLines)
{
    //TrainAdapter not moved
}

void Train::fromJson(const QJsonObject &obj, TypeManager& manager)
{
    const QJsonArray& archeci=obj.value("checi").toArray();
    _trainName.fromJson(archeci);

    setType(obj.value("type").toString(), manager);
    _starting=StationName::fromSingleLiteral( obj.value("sfz").toString());
    _terminal=StationName::fromSingleLiteral(obj.value("zdz").toString());
    _show=obj.value("shown").toBool();
    const auto& valpass = obj.value("passenger");
    if(valpass.isBool())
        _passenger = static_cast<TrainPassenger>(obj.value("passenger").toBool());
    else
        _passenger = static_cast<TrainPassenger>(obj.value("passenger").toInt());
    const QJsonArray& artable=obj.value("timetable").toArray();
    _autoLines = obj.value("autoItem").toBool(true);

    for (auto p=artable.cbegin();p!=artable.cend();++p){
        _timetable.emplace_back(p->toObject());
    }
    const QJsonObject ui = obj.value("UI").toObject();
    if (!ui.isEmpty()) {
        QPen pen = QPen(QColor(ui.value("Color").toString()), ui.value("LineWidth").toDouble(1.0),
            static_cast<Qt::PenStyle>(ui.value("LineStyle").toInt(Qt::SolidLine)));
        if (pen.width() == 0) {
            //线宽为0表示采用默认
            pen.setWidthF(_type->pen().widthF());
        }
        _pen = pen;
    }
}

QJsonObject Train::toJson() const
{
    QJsonArray ar;
    for(const auto& p:_timetable){
        ar.append(p.toJson());
    }
    
    QJsonObject obj{
        {"checi",_trainName.toJson()},
        {"type",_type->name()},
        {"sfz",_starting.toSingleLiteral()},
        {"zdz",_terminal.toSingleLiteral()},
        {"shown",_show},
        {"passenger",static_cast<int>(_passenger)},
        {"timetable",ar},
        {"autoItem",_autoLines}
    };

    QJsonObject ui;
    if (_pen.has_value()) {
        const auto& p = _pen.value();
        ui.insert("Color", p.color().name());
        ui.insert("LineWidth", p.widthF());
        ui.insert("LineStyle", p.style());   //新增
    }
    obj.insert("UI", ui);
    
    if (!_autoLines) {
        //QJsonArray items;
        //for (auto p = _lines.begin(); p != _lines.end(); ++p) {
        //    items.append((*p)->toJson());
        //}
        //obj.insert("itemInfo", items);
    }
    return obj;
}

void Train::setType(const QString& _typeName, TypeManager& manager)
{
    _type = manager.findOrCreate(_typeName);
}

const QPen& Train::pen() const
{
    //注意不能用value_or，因为返回的是值类型。
    if (_pen.has_value())
        return _pen.value();
    else
        return type()->pen();
}

void Train::appendStation(const StationName &name,
                          const QTime &arrive, const QTime &depart,
                          bool business, const QString &track, const QString &note)
{
    _timetable.emplace_back(name, arrive, depart, business, track, note);
}

void Train::prependStation(const StationName& name, 
    const QTime& arrive, const QTime& depart,
    bool business, const QString& track, const QString& note)
{
    _timetable.emplace_front(name, arrive, depart, business, track, note);
}

typename Train::StationPtr
    Train::findFirstStation(const StationName& name)
{
    auto p = _timetable.begin();
    for (; p != _timetable.end(); ++p) {
        if (p->name == name)
            break;
    }
    return p;
}

QList<typename Train::StationPtr> Train::findAllStations(const StationName& name)
{
    QList<StationPtr> res;
    for (auto p = _timetable.begin(); p != _timetable.end(); ++p) {
        if (p->name == name) {
            res.append(p);
        }
    }
    return res;
}

Train::ConstStationPtr Train::findFirstStation(const StationName& name) const
{
    auto p = _timetable.begin();
    for (; p != _timetable.end(); ++p) {
        if (p->name == name)
            break;
    }
    return p;
}

QList<Train::ConstStationPtr> Train::findAllStations(const StationName& name) const
{
    QList<ConstStationPtr> res;
    for (auto p = _timetable.begin(); p != _timetable.end(); ++p) {
        if (p->name == name) {
            res.append(p);
        }
    }
    return res;
}

Train::StationPtr Train::findFirstGeneralStation(const StationName &name)
{
    auto p=findFirstStation(name);
    if(p!=nullStation())
        return p;
    for(p=_timetable.begin();p!=_timetable.end();++p){
        if (p->name.equalOrBelongsTo(name))
            break;
    }
    return p;
}

QList<Train::StationPtr> Train::findAllGeneralStations(const StationName &name)
{
    QList<StationPtr> res;
    for (auto p = _timetable.begin(); p != _timetable.end(); ++p) {
        if (p->name.equalOrBelongsTo(name)) {
            res.append(p);
        }
    }
    return res;
}

std::shared_ptr<TrainAdapter> Train::bindToRailway(Railway& railway, const Config& config)
{
    //2021.06.24 新的实现 基于Adapter
    for (auto p = _adapters.begin(); p != _adapters.end(); ++p) {
        if (&((*p)->railway()) == &railway)
            return *p;
    }
    auto adp = std::make_shared<TrainAdapter>(*this, railway, config);
    if (!adp->isNull()) {
        _adapters.append(adp);
        return adp;
    }
    return nullptr;
}


std::shared_ptr<TrainAdapter> Train::updateBoundRailway(Railway& railway, const Config& config)
{
    //2021.06.24  基于Adapter新的实现
    //2021.07.04  TrainLine里面有Adapter的引用。不要move assign，直接删了重来好了
    for (auto p = _adapters.begin(); p != _adapters.end(); ++p) {
        if (&((*p)->railway()) == &railway) {
            _adapters.erase(p);
            break;
        }
    }
    //没找到，则创建
    auto p = std::make_shared<TrainAdapter>(*this, railway, config);
    if (!p->isNull()) {
        _adapters.append(p);
        return p;
    }
    return nullptr;
}

void Train::unbindToRailway(const Railway& railway)
{
    for (auto p = _adapters.begin(); p != _adapters.end(); ++p) {
        if (&((*p)->railway()) == &railway) {
            _adapters.erase(p);
            return;
        }
    }
}




#if 0
void Train::bindToRailway(std::shared_ptr<Railway> railway)
{
    if (_boundRail.lock() == railway)
        return;
    else
        unbindToRailway();
    _boundRail=railway;
    auto last=nullStation();   //上一个成功绑定的车站
    std::shared_ptr<RailStation> raillast;
    auto p=_timetable.begin();
    Direction locDir=Direction::Undefined;
    int cnt=0;   //绑定计数器
    for(;p!=nullStation();++p){
        //在Railway中找车站是常数复杂度
        auto railst=railway->stationByGeneralName(p->name);
        if(railst){
            //非空指针表示搜索成功。现在考虑是否绑定
            //目前唯一阻止绑定的事由是经由方向不对
            //原则上，这是不大可能发生的事情；因此只有显示知道行别不对时，才拒绝绑定
            if (locDir==Direction::Undefined ||
                    railst->isDirectionVia(locDir)){
                //成功绑定到车站
                cnt++;
                p->bindToRailStation(railst);
                //现在：计算当前区间上下行情况
                if (raillast){
                    locDir=railway->gapDirection(raillast,railst);
                }
                if (cnt==2){
                    //第二个站时，还是检查一下第一个站的绑定是否合适
                    if(!raillast->isDirectionVia(locDir)){
                        //发现方向不对，撤销绑定
                        //相当于当前是第一个
                        cnt--;
                        last->unbindToRailStation();
                    }
                }
                raillast=railst;
                last=p;
            }
        }
    }
}

void Train::updateBoundRailway(std::shared_ptr<Railway> railway)
{
    unbindToRailway();
    bindToRailway(railway);
}

void Train::unbindToRailway()
{
    _boundRail.reset();
    for(auto& p:_timetable){
        p.unbindToRailStation();
    }
}

Train::StationPtr Train::prevBoundStation(StationPtr st)
{
    //注意reverse_iterator的各种坑
    auto s = std::make_reverse_iterator(st);
    for (; s != _timetable.rend(); ++s) {
        if (s->isBoundToRail()) {
            return --(s.base());
        }
    }
    return nullStation();
}

Train::StationPtr Train::nextBoundStation(StationPtr st)
{
    for (++st; !isNullStation(st); ++st) {
        if (st->isBoundToRail())
            return st;
    }
    return nullStation();
}

Train::ConstStationPtr Train::prevBoundStation(ConstStationPtr st) const
{
    auto s = std::make_reverse_iterator(st);
    for (; s != _timetable.rend(); ++s) {
        if (s->isBoundToRail()) {
            return --(s.base());
        }
    }
    return nullStation();
}

Train::ConstStationPtr Train::nextBoundStation(ConstStationPtr st) const
{
    for (++st; !isNullStation(st); ++st) {
        if (st->isBoundToRail())
            return st;
    }
    return nullStation();
}

Direction Train::stationDirection(const StationName& station)
{
    if (!isBoundToRailway())
        return Direction::Undefined;
    auto p = findFirstGeneralStation(station);
    if (isNullStation(p))
        return Direction::Undefined;
    else
        return stationDirection(p);
}

Direction Train::stationDirection(ConstStationPtr station)
{
    if (isNullStation(station) || !isBoundToRailway() ||
        !station->isBoundToRail())
        return Direction::Undefined;
    std::shared_ptr<Railway> rail = _boundRail.lock();
    auto prev = prevBoundStation(station);
    if (!isNullStation(prev)) {
        Direction d = rail->gapDirection(prev->boundRailStation().lock(),
            station->boundRailStation().lock());
        if (DirFunc::isValid(d))
            return d;
    }
    //只要没有返回就是左区间判定失败，改为右区间
    auto next = nextBoundStation(station);
    if (!isNullStation(next)) {
        Direction d = rail->gapDirection(station->boundRailStation().lock(),
            next->boundRailStation().lock());
        return d;
    }
    return Direction::Undefined;
}
#endif

Train Train::translation(TrainName name, int sec)
{
    Train train(*this);   //copy construct
    train.setTrainName(name);
    for(auto& p:train._timetable){
        p.arrive=p.arrive.addSecs(sec);
        p.depart=p.depart.addSecs(sec);
    }
    return train;
}

#if 0
void Train::removeNonLocal()
{
    if (!isBoundToRailway())
        return;
    auto p = _timetable.begin();
    while (p != _timetable.end()) {
        if (!p->isBoundToRail()) {
            auto p0 = p;
            ++p;
            _timetable.erase(p0);
        }
        else {
            ++p;
        }
    }
}
#endif

void Train::jointTrain(Train&& train, bool former)
{
    constexpr int max_cross_depth = 10;
    auto& table2 = train.timetable();
    if (former) {
        auto p1 = _timetable.begin();
        auto p2 = table2.rbegin();
        ++p1; ++p2;
        bool crossed = false;
        int cnt;
        for (cnt = 1; cnt < max_cross_depth; cnt++) {
            if (std::equal(_timetable.begin(), p1, p2.base(), table2.end(),TrainStation::nameEqual)) {
                crossed = true;
                break;
            }
            if (p1 == _timetable.end() || p2 == table2.rend())
                break;
            ++p1; ++p2;
        }
        if (crossed) {
            for (int i = 0; i < cnt; i++) {
                table2.pop_back();
            }
        }
        //交叉判定结束
        while (!table2.empty()) {
            TrainStation ts = std::move(table2.back());
            table2.pop_back();
            _timetable.push_front(std::move(ts));
        }
    }
    else {
        auto p1 = _timetable.rbegin();
        auto p2 = table2.begin();
        ++p1; ++p2;
        int cnt;
        bool cross = false;
        for (cnt = 1; cnt < max_cross_depth; cnt++) {
            if (std::equal(p1.base(), _timetable.end(), table2.begin(), p2, TrainStation::nameEqual)) {
                cross = true;
                break;
            }
            if (p1 == _timetable.rend() || p2 == table2.end())
                break;
            ++p1; ++p2;
        }
        if (cross)
            for (int i = 0; i < cnt; i++)
                table2.pop_front();
        while (!table2.empty()) {
            TrainStation st = std::move(table2.front());
            table2.pop_front();
            _timetable.push_back(std::move(st));
        }
    }
}

void Train::show() const
{
    _trainName.show();
    for(const auto& p:_timetable){
        qDebug() << p << Qt::endl;
    }
}

void Train::intervalExchange(Train& train2, StationPtr start1, StationPtr end1, 
    StationPtr start2, StationPtr end2)
{
    auto& table2 = train2._timetable;
    ++end1; ++end2;

    std::list<TrainStation> tmp;
    tmp.splice(tmp.begin(), _timetable, start1, end1);
    _timetable.splice(end1, table2, start2, end2);
    table2.splice(end2, tmp);
}

void Train::setRouting(std::weak_ptr<Routing> rout, std::list<RoutingNode>::iterator node)
{
    _routing = rout;
    _routingNode = node;
}

const AdapterStation* Train::boundTerminal() const
{
    if (_timetable.empty())return nullptr;
    auto lastIter = _timetable.end(); --lastIter;
    for (auto p : _adapters) {
        auto* last = p->lastStation();
        if (last && last->trainStation == lastIter)
            return last;
    }
    return nullptr;
}

std::shared_ptr<RailStation> Train::boundTerminalRail() const
{
    auto* last = boundTerminal();
    if (last)
        return last->railStation.lock();
    return nullptr;
}

const AdapterStation* Train::boundStarting() const
{
    if (_timetable.empty())
        return nullptr;
    for (auto p : _adapters) {
        auto* first = p->firstStation();
        if (first && first->trainStation == _timetable.begin())
            return first;
    }
    return nullptr;
}

std::shared_ptr<RailStation> Train::boundStartingRail() const
{
    auto* first = boundStarting();
    if (first)
        return first->railStation.lock();
    return nullptr;
}

double Train::localMile() 
{
    if (_locMile.has_value())
        return _locMile.value();
    double res = 0;
    for (auto p : _adapters)
        res += p->totalMile();
    _locMile = res;
    return res;
}

int Train::localSecs() 
{
    auto&& t = localRunStaySecs();
    return t.first + t.second;
}

std::pair<int, int> Train::localRunStaySecs()
{
    if (_locRunSecs.has_value())
        return std::make_pair(_locRunSecs.value(), _locStaySecs.value());
    int run = 0, stay = 0;
    for (auto p : _adapters) {
        auto d = p->runStaySecs();
        run += d.first;
        stay += d.second;
    }
    _locRunSecs = run;
    _locStaySecs = stay;
    return std::make_pair(run, stay);
}

int Train::localRunSecs()
{
    return localRunStaySecs().first;
}

double Train::localTraverseSpeed() 
{
    double mile = localMile();
    int secs = localSecs();
    //secs如果是0则得inf，C++中不会出问题
    return mile / secs * 3600;  
}

double Train::localTechSpeed()
{
    return localMile() / localRunSecs() * 3600;
}

void Train::invalidateTempData()
{
    _locMile = std::nullopt;
    _locRunSecs = std::nullopt;
    _locStaySecs = std::nullopt;
}



bool Train::ltName(const std::shared_ptr<const Train>& t1, const std::shared_ptr<const Train>& t2)
{
    return t1->trainName().full() < t2->trainName().full();
}

bool Train::ltStarting(const std::shared_ptr<const Train>& t1, const std::shared_ptr<const Train>& t2)
{
    return t1->starting() < t2->starting();
}

bool Train::ltTerminal(const std::shared_ptr<const Train>& t1, const std::shared_ptr<const Train>& t2)
{
    return t1->terminal() < t2->terminal();
}

bool Train::ltShow(const std::shared_ptr<const Train>& t1, const std::shared_ptr<const Train>& t2)
{
    return t1->isShow() < t2->isShow();
}

bool Train::ltTravSpeed(const std::shared_ptr<Train>& t1, const std::shared_ptr<Train>& t2)
{
    return t1->localTraverseSpeed() < t2->localTraverseSpeed();
}

bool Train::ltTechSpeed(const std::shared_ptr<Train>& t1, const std::shared_ptr<Train>& t2)
{
    return t1->localTechSpeed() < t2->localTechSpeed();
}

bool Train::ltMile(const std::shared_ptr<Train>& t1, const std::shared_ptr<Train>& t2)
{
    return t1->localMile() < t2->localMile();
}

bool Train::ltType(const std::shared_ptr<const Train>& t1, const std::shared_ptr<const Train>& t2)
{
    return t1->type()->name() < t2->type()->name();
}

bool Train::gtName(const std::shared_ptr<const Train>& t1, const std::shared_ptr<const Train>& t2)
{
    return t1->trainName().full() > t2->trainName().full();
}

bool Train::gtStarting(const std::shared_ptr<const Train>& t1, const std::shared_ptr<const Train>& t2)
{
    return t1->starting() > t2->starting();
}

bool Train::gtTerminal(const std::shared_ptr<const Train>& t1, const std::shared_ptr<const Train>& t2)
{
    return t1->terminal() > t2->terminal();
}

bool Train::gtShow(const std::shared_ptr<const Train>& t1, const std::shared_ptr<const Train>& t2)
{
    return t1->isShow() > t2->isShow();
}

bool Train::gtType(const std::shared_ptr<const Train>& t1, const std::shared_ptr<const Train>& t2)
{
    return t1->type()->name() > t2->type()->name();
}

bool Train::gtTravSpeed(const std::shared_ptr<Train>& t1, const std::shared_ptr<Train>& t2)
{
    return t1->localTraverseSpeed() > t2->localTraverseSpeed();
}

bool Train::gtTechSpeed(const std::shared_ptr<Train>& t1, const std::shared_ptr<Train>& t2)
{
    return t1->localTechSpeed() > t2->localTechSpeed();
}

bool Train::gtMile(const std::shared_ptr<Train>& t1, const std::shared_ptr<Train>& t2)
{
    return t1->localMile() > t2->localMile();
}
