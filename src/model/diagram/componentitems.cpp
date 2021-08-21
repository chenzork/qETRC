﻿#include "componentitems.h"

navi::DiagramItem::DiagramItem(Diagram& diagram) :
	AbstractComponentItem(0), _diagram(diagram),
	railList(std::make_unique<RailwayListItem>(diagram, this)),
	pageList(std::make_unique<PageListItem>(diagram, this)),
	trainList(std::make_unique<TrainListItem>(diagram.trainCollection(), this))
{
}

navi::AbstractComponentItem* navi::DiagramItem::child(int i)
{
	switch (i) {
	case RowRailways:return railList.get();
	case RowPages:return pageList.get();
	case RowTrains:return trainList.get();
	default:return nullptr;
	}
}

navi::RailwayListItem::RailwayListItem(Diagram& diagram, DiagramItem* parent):
	AbstractComponentItem(0,parent), _diagram(diagram)
{
	for (int i = 0; i < _diagram.railwayCount();i++) {
		auto p = _diagram.railways().at(i);
		_rails.emplace_back(std::make_unique<RailwayItem>(p, i, this));
	}
}

navi::AbstractComponentItem* navi::RailwayListItem::child(int i)
{
	if (i >= 0 && i < _rails.size())
		return _rails.at(i).get();
	return nullptr;
}

QString navi::RailwayListItem::data(int i) const
{
	switch (i) {
	case ColItemName:return QObject::tr("基线数据");
	case ColItemNumber:return QString::number(_diagram.railways().size());
	default:return "";
	}
}

void navi::RailwayListItem::appendRailways(const QList<std::shared_ptr<Railway>>& rails) 
{
	int row = _diagram.railwayCount();
	for (auto p = rails.begin(); p != rails.end(); ++p) {
		_diagram.railways().append(*p);
		_rails.emplace_back(std::make_unique<RailwayItem>(*p, row++, this));
    }
}

void navi::RailwayListItem::appendRailway(std::shared_ptr<Railway> rail)
{
    _diagram.railways().append(rail);
    int row = _diagram.railwayCount();
    _rails.emplace_back(std::make_unique<RailwayItem>(rail,row,this));
}

void navi::RailwayListItem::removeTailRailways(int cnt)
{
	for (int i = 0; i < cnt; i++) {
		_rails.pop_back();
		_diagram.railways().pop_back();
    }
}

void navi::RailwayListItem::removeRailwayAt(int i)
{
    _diagram.removeRailwayAt(i);
    auto p=_rails.begin();
    std::advance(p,i);
    p=_rails.erase(p);
    for(;p!=_rails.end();++p){
        (*p)->setRow((*p)->row()-1);
    }
}

navi::PageListItem::PageListItem(Diagram& diagram, DiagramItem* parent):
	AbstractComponentItem(1,parent),_diagram(diagram)
{
	int row = 0;
	for (auto p: _diagram.pages()) {
		_pages.emplace_back(std::make_unique<PageItem>(p, row++, this));
	}
}

navi::AbstractComponentItem* navi::PageListItem::child(int i)
{
	if (i >= 0 && i < _pages.size())
		return _pages.at(i).get();
	return nullptr;
}

QString navi::PageListItem::data(int i)const
{
	switch (i) {
	case ColItemName:return QObject::tr("运行图视窗");
	case ColItemNumber:return QString::number(_diagram.pages().size());
	default:return "";
    }
}

void navi::PageListItem::resetChildren()
{
    _pages.clear();
    int row=0;
    for(auto p:_diagram.pages()){
        _pages.emplace_back(std::make_unique<PageItem>(p,row++,this));
    }
}

void navi::PageListItem::insertPage(std::shared_ptr<DiagramPage> page, int i)
{
    _diagram.pages().insert(i,page);
    auto p=_pages.begin();
    std::advance(p,i);
    p=_pages.insert(p,std::make_unique<PageItem>(page,i,this));
    for(++p;p!=_pages.end();++p){
        (*p)->setRow((*p)->row()+1);
    }

}

void navi::PageListItem::removePageAt(int i)
{
    _diagram.pages().removeAt(i);
    auto p=_pages.begin();
    std::advance(p,i);
    p=_pages.erase(p);
    for(;p!=_pages.end();++p){
        (*p)->setRow((*p)->row()-1);
    }
}

navi::AbstractComponentItem* navi::RailwayItem::child(int i)
{
    if(i<static_cast<int>( _rulers.size()))
        return _rulers.at(i).get();
    else
        return _forbids.at(i-_rulers.size()).get();
}

int navi::RailwayItem::childCount() const
{
    return static_cast<int>(_rulers.size()+_forbids.size());
}

navi::RailwayItem::RailwayItem(std::shared_ptr<Railway> rail, int row, RailwayListItem* parent):
	AbstractComponentItem(row,parent),_railway(rail)
{
    int i=0;
    foreach(auto ruler,_railway->rulers()){
        _rulers.emplace_back(std::make_unique<RulerItem>(ruler,i++,this));
    }
    rail->ensureForbids(Forbid::FORBID_COUNT);
    foreach(auto forbid,_railway->forbids()){
        _forbids.emplace_back(std::make_unique<ForbidItem>(forbid,_railway,i++,this));
    }
}

QString navi::RailwayItem::data(int i) const
{
	switch (i) {
	case ColItemName:return _railway->name();
	default:return "";
    }
}

void navi::RailwayItem::onRulerInsertedAt(int i)
{
    auto p=_rulers.begin();
    std::advance(p,i);
    p=_rulers.emplace(p, std::make_unique<RulerItem>(_railway->getRuler(i),i,this));
    for(++p;p!=_rulers.end();++p){
        (*p)->rowRef()++;
    }
}

void navi::RailwayItem::onRulerRemovedAt(int i)
{
    auto p=_rulers.begin();
    std::advance(p,i);
    p=_rulers.erase(p);
    for(;p!=_rulers.end();++p){
        (*p)->rowRef()--;
    }
}

int navi::RailwayItem::getRulerRow(std::shared_ptr<const Ruler> ruler)
{
    return ruler->index();
}

int navi::RailwayItem::getForbidRow(std::shared_ptr<const Forbid> forbid)
{
    return static_cast<int>( _rulers.size() + forbid->index());
}

navi::PageItem::PageItem(std::shared_ptr<DiagramPage> page, int row, PageListItem* parent):
	AbstractComponentItem(row,parent),_page(page)
{

}

QString navi::PageItem::data(int i) const
{
	switch (i) {
	case ColItemName:return _page->name();
	case ColItemNumber:return QString::number(_page->railwayCount());
	case ColDescription:return _page->railNameString();
	default:return "";
	}
}

navi::TrainListItem::TrainListItem(TrainCollection& coll, DiagramItem* parent):
	AbstractComponentItem(2,parent),_coll(coll)
{
	int row = 0;
	for (auto p : _coll.trains()) {
		_trains.emplace_back(std::make_unique<TrainModelItem>(p, row++, this));
	}
}

QString navi::TrainListItem::data(int i) const
{
	switch (i) {
	case ColItemName:return QObject::tr("列车");
	case ColItemNumber:return QString::number(_trains.size());  
	default:return "";
	}
}

void navi::TrainListItem::resetChildren()
{
	_trains.clear();   //删除child
	int row = 0;
	for (auto p : _coll.trains()) {
		_trains.emplace_back(std::make_unique<TrainModelItem>(p, row++, this));
    }
}

void navi::TrainListItem::removeTrainAt(int i)
{
    _coll.takeTrainAt(i);
    auto p=_trains.begin();
    std::advance(p,i);
    p=_trains.erase(p);
    for(;p!=_trains.end();++p){
        (*p)->setRow((*p)->row()-1);
    }
}

void navi::TrainListItem::undoRemoveTrainAt(std::shared_ptr<Train> train, int i)
{
    _coll.insertTrainForUndo(i,train);
    auto p=_trains.begin();
    std::advance(p,i);
    p=_trains.insert(p,std::make_unique<TrainModelItem>(train,i,this));
    for(++p;p!=_trains.end();++p){
        (*p)->setRow((*p)->row()+1);
    }
}

void navi::TrainListItem::undoAddNewTrain()
{
	_coll.takeLastTrain();
	_trains.pop_back();
}

void navi::TrainListItem::addNewTrain(std::shared_ptr<Train> train)
{
	_coll.appendTrain(train);
    int row = static_cast<int>( _trains.size());
	_trains.emplace_back(std::make_unique<TrainModelItem>(train, row, this));
}

navi::AbstractComponentItem* navi::TrainListItem::child(int i)
{
    if (i >= 0 && i < static_cast<int>( _trains.size()))
		return _trains[i].get();
	return nullptr;
}

navi::TrainModelItem::TrainModelItem(std::shared_ptr<Train> train, int row, TrainListItem* parent):
	AbstractComponentItem(row,parent), _train(train)
{
}

QString navi::TrainModelItem::data(int i) const
{
	switch (i) {
	case ColItemName:return _train->trainName().full();
	case ColDescription:return _train->startEndString();
	default:return {};
	}
}

navi::RulerItem::RulerItem(std::shared_ptr<Ruler> ruler, int row, RailwayItem *parent):
    AbstractComponentItem(row, parent), _ruler(ruler)
{

}

QString navi::RulerItem::data(int i) const
{
    switch (i){
    case ColItemName: return QObject::tr("标尺|%1").arg(_ruler->name());
    default:return {};
    }
}

navi::ForbidItem::ForbidItem(std::shared_ptr<Forbid> forbid,std::shared_ptr<Railway> railway,
                             int row, RailwayItem *parent):
    AbstractComponentItem(row, parent), _railway(railway),_forbid(forbid)
{

}

QString navi::ForbidItem::data(int i) const
{
    switch (i){
    case ColItemName: return QObject::tr("%1天窗").arg(_forbid->name());
    default:return {};
    }
}
