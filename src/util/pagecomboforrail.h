﻿#pragma once

#include <QComboBox>

class DiagramPage;
class Railway;
class Diagram;
class PageComboForRail : public QComboBox
{
    Q_OBJECT
    Diagram& diagram;
    std::shared_ptr<Railway> _railway{};
    std::shared_ptr<DiagramPage> _page{};
public:
    PageComboForRail(Diagram& diagram_, QWidget* parent=nullptr);
    PageComboForRail(Diagram& diagram_, std::shared_ptr<Railway> railway,
                     QWidget* parent=nullptr);
    auto page(){return _page;}

    /**
     * @brief pageIndex
     * @return  当前所选page在diagram中的下标。如果没有，返回-1
     */
    int pageIndex()const;
    void refreshData();

    static int dlgGetPageIndex(Diagram& diagram, std::shared_ptr<Railway> rail,
                        QWidget* parent, const QString& title, const QString& prompt);

private:
    void setupItems();
signals:
    void pageIndexChanged(int i);
public slots:
    void setRailway(const std::shared_ptr<Railway>& railway);
private slots:
    void onIndexChanged(int i);
};

