﻿#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <QTableView>

#include <memory>
#include "data/rail/rail.h"
#include "model/rail/rulermodel.h"

/**
 * @brief The RulerWidget class
 * 标尺编辑的面板。与pyETRC的实现比较类似，但不做那么多按钮。
 */
class RulerWidget : public QWidget
{
    Q_OBJECT;
    std::shared_ptr<Ruler> ruler;
    RulerModel* model;

    QLineEdit* edName;
    QCheckBox* ckDiff;
    QTableView* table;
    bool updating = false;
public:
    explicit RulerWidget(std::shared_ptr<Ruler> ruler_,
                         QWidget *parent = nullptr);
    void refreshData();
    //void setRuler(std::shared_ptr<Ruler> ruler);
    auto getRuler(){return ruler;}
    auto getModel(){return model;}

protected:
    virtual void focusInEvent(QFocusEvent* e)override;

private:
    void initUI();

private slots:
    void actApply();
    void actCancel();
    void onDiffChanged(bool on);

signals:
    void actChangeRulerData(std::shared_ptr<Ruler> ruler, std::shared_ptr<Railway> nr);
    void focusInRuler(std::shared_ptr<Ruler>ruler);

};

