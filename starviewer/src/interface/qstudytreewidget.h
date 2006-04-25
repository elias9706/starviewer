
/***************************************************************************
 *   Copyright (C) 2005 by marc                                            *
 *   marc@localhost                                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef UDGQSTUDYTREEWIDGET_H
#define UDGQSTUDYTREEWIDGET_H

#include "ui_qstudytreewidgetbase.h"
#include <QWidget>
#include <QMenu>
#include <QString>
#include "study.h"
#include "series.h"
#include "pacsparameters.h"
#include "studylist.h"

namespace udg {

/** Aquesta classe �s un widget millorar i modificat del TreeVidget, que permet mostrar estudis i s�ries d'una manera organitzada i f�cilment
@author marc
*/


class QStudyTreeWidget : public QWidget , private Ui::QStudyTreeWidgetBase{
Q_OBJECT
public:

    QStudyTreeWidget( QWidget *parent = 0 );
    
    void insertSeries(Series *);
    void insertStudyList(StudyList *);
    void removeStudy(QString StudyUID);
 
    void setSortColumn(int);
    
    QString getSelectedStudyPacsAETitle();
    QString getSelectedStudyUID();
    QString getSelectedSeriesUID();
    
    void saveColumnsWidth();
    
    ~QStudyTreeWidget();
    
    
protected:
    void contextMenuEvent(QContextMenuEvent *event);

signals :
    void expand(QString,QString);
    void retrieve();
    void delStudy();
    void view();
    void addSeries(Series *serie);
    void clearSeriesListWidget();
    void selectedSeriesList(QString); 

public slots:
    void selectedSeriesIcon(QString );
    
    void clicked(QTreeWidgetItem *,int);
    void doubleClicked(QTreeWidgetItem *,int);
    void clear( );
    void retrieveStudy( );
    void deleteStudy( );
    void viewStudy( );

protected:

private :

    QMenu m_popUpMenu;
    QString m_parentName;

    QIcon m_openFolder, m_closeFolder, m_iconSeries;
    QString m_oldStudyUID;
    std::string m_oldPacsAETitle,m_OldInstitution;
    
    void createConnections( );
    void setWidthColumns( );
    void createPopupMenu( );
    void insertStudy(Study *);
    void setSeriesToSeriesListWidget(QTreeWidgetItem *item);
    void expand(QTreeWidgetItem *);

    QString formatName(const std::string);
    QString formatAge(const std::string);
    QString formatDate(const std::string);
    QString formatHour(const std::string);
};

};

#endif
