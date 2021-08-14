﻿#include "qetimedelegate.h"

#include <QTimeEdit>

QETimeDelegate::QETimeDelegate(QObject *parent, const QString &format):
    QStyledItemDelegate(parent),_format(format)
{

}

QWidget *QETimeDelegate::createEditor(QWidget *parent,
                                      const QStyleOptionViewItem &option,
                                      const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
     auto* ed=new QTimeEdit(parent);
     setupEditor(ed);
     return ed;
}

void QETimeDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QTimeEdit* ed=static_cast<QTimeEdit*>(editor);
    ed->setTime(index.data(Qt::EditRole).toTime());
}

void QETimeDelegate::setModelData(QWidget *editor,
                                  QAbstractItemModel *model,
                                  const QModelIndex &index) const
{
    auto* ed=static_cast<QTimeEdit*>(editor);
    model->setData(index,ed->time(),Qt::EditRole);
}

QString QETimeDelegate::displayText(const QVariant &value, const QLocale &locale) const
{
    Q_UNUSED(locale);
    return value.toTime().toString(_format);
}

void QETimeDelegate::setupEditor(QTimeEdit *ed) const
{
    ed->setDisplayFormat(_format);
    ed->setWrapping(true);
}
