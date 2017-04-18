/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmlprofilerstatisticsmodel.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilerdatamodel.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QHash>
#include <QSet>
#include <QString>
#include <QPointer>

#include <functional>

namespace QmlProfiler {

class QmlProfilerStatisticsModel::QmlProfilerStatisticsModelPrivate
{
public:
    QHash<int, QmlProfilerStatisticsModel::QmlEventStats> data;
    QHash<int, QmlProfilerStatisticsModel::QmlEventStats> workingSet;


    QPointer<QmlProfilerStatisticsRelativesModel> childrenModel;
    QPointer<QmlProfilerStatisticsRelativesModel> parentsModel;

    QmlProfilerModelManager *modelManager;

    int modelId;

    QList<RangeType> acceptedTypes;
    QSet<int> eventsInBindingLoop;
    QHash<int, QString> notes;

    QStack<QmlEvent> callStack;
    QStack<QmlEvent> compileStack;
    qint64 qmlTime = 0;
    qint64 lastEndTime = 0;
    QHash <int, QVector<qint64> > durations;
};

QmlProfilerStatisticsModel::QmlProfilerStatisticsModel(QmlProfilerModelManager *modelManager,
                                               QObject *parent) :
    QObject(parent), d(new QmlProfilerStatisticsModelPrivate)
{
    d->modelManager = modelManager;
    d->callStack.push(QmlEvent());
    d->compileStack.push(QmlEvent());
    connect(modelManager, &QmlProfilerModelManager::stateChanged,
            this, &QmlProfilerStatisticsModel::dataChanged);
    connect(modelManager->notesModel(), &Timeline::TimelineNotesModel::changed,
            this, &QmlProfilerStatisticsModel::notesChanged);
    d->modelId = modelManager->registerModelProxy();

    d->acceptedTypes << Compiling << Creating << Binding << HandlingSignal << Javascript;

    modelManager->announceFeatures(Constants::QML_JS_RANGE_FEATURES,
                                   [this](const QmlEvent &event, const QmlEventType &type) {
        loadEvent(event, type);
    }, [this]() {
        finalize();
    });
}

QmlProfilerStatisticsModel::~QmlProfilerStatisticsModel()
{
    delete d;
}

void QmlProfilerStatisticsModel::restrictToFeatures(qint64 features)
{
    bool didChange = false;
    for (int i = 0; i < MaximumRangeType; ++i) {
        RangeType type = static_cast<RangeType>(i);
        quint64 featureFlag = 1ULL << featureFromRangeType(type);
        if (Constants::QML_JS_RANGE_FEATURES & featureFlag) {
            bool accepted = features & featureFlag;
            if (accepted && !d->acceptedTypes.contains(type)) {
                d->acceptedTypes << type;
                didChange = true;
            } else if (!accepted && d->acceptedTypes.contains(type)) {
                d->acceptedTypes.removeOne(type);
                didChange = true;
            }
        }
    }
    if (!didChange || d->modelManager->state() != QmlProfilerModelManager::Done)
        return;

    clear();
    d->modelManager->qmlModel()->replayEvents(d->modelManager->traceTime()->startTime(),
                                              d->modelManager->traceTime()->endTime(),
                                              std::bind(&QmlProfilerStatisticsModel::loadEvent,
                                                        this, std::placeholders::_1,
                                                        std::placeholders::_2));
    finalize();
    notesChanged(-1); // Reload notes
}

const QHash<int, QmlProfilerStatisticsModel::QmlEventStats> &QmlProfilerStatisticsModel::getData() const
{
    return d->data;
}

const QVector<QmlEventType> &QmlProfilerStatisticsModel::getTypes() const
{
    return d->modelManager->qmlModel()->eventTypes();
}

const QHash<int, QString> &QmlProfilerStatisticsModel::getNotes() const
{
    return d->notes;
}

void QmlProfilerStatisticsModel::clear()
{
    d->data.clear();
    d->eventsInBindingLoop.clear();
    d->notes.clear();
    d->callStack.clear();
    d->compileStack.clear();
    d->callStack.push(QmlEvent());
    d->compileStack.push(QmlEvent());
    d->qmlTime = 0;
    d->lastEndTime = 0;
    d->durations.clear();
    if (!d->childrenModel.isNull())
        d->childrenModel->clear();
    if (!d->parentsModel.isNull())
        d->parentsModel->clear();
}

void QmlProfilerStatisticsModel::setRelativesModel(QmlProfilerStatisticsRelativesModel *relative,
                                                   QmlProfilerStatisticsRelation relation)
{
    if (relation == QmlProfilerStatisticsParents)
        d->parentsModel = relative;
    else
        d->childrenModel = relative;
}

QmlProfilerModelManager *QmlProfilerStatisticsModel::modelManager() const
{
    return d->modelManager;
}

void QmlProfilerStatisticsModel::dataChanged()
{
    if (d->modelManager->state() == QmlProfilerModelManager::ClearingData)
        clear();
}

void QmlProfilerStatisticsModel::notesChanged(int typeIndex)
{
    const QmlProfilerNotesModel *notesModel = d->modelManager->notesModel();
    if (typeIndex == -1) {
        d->notes.clear();
        for (int noteId = 0; noteId < notesModel->count(); ++noteId) {
            int noteType = notesModel->typeId(noteId);
            if (noteType != -1) {
                QString &note = d->notes[noteType];
                if (note.isEmpty()) {
                    note = notesModel->text(noteId);
                } else {
                    note.append(QStringLiteral("\n")).append(notesModel->text(noteId));
                }
            }
        }
    } else {
        d->notes.remove(typeIndex);
        const QVariantList changedNotes = notesModel->byTypeId(typeIndex);
        if (!changedNotes.isEmpty()) {
            QStringList newNotes;
            for (QVariantList::ConstIterator it = changedNotes.begin(); it !=  changedNotes.end();
                 ++it) {
                newNotes << notesModel->text(it->toInt());
            }
            d->notes[typeIndex] = newNotes.join(QStringLiteral("\n"));
        }
    }

    emit notesAvailable(typeIndex);
}

void QmlProfilerStatisticsModel::loadEvent(const QmlEvent &event, const QmlEventType &type)
{
    if (!d->acceptedTypes.contains(type.rangeType()))
        return;

    QStack<QmlEvent> &stack = type.rangeType() == Compiling ? d->compileStack : d->callStack;
    switch (event.rangeStage()) {
    case RangeStart:
        // binding loop detection: check whether event is already in stack
        if (type.rangeType() == Binding || type.rangeType() == HandlingSignal) {
            for (int ii = 1; ii < stack.size(); ++ii) {
                if (stack.at(ii).typeIndex() == event.typeIndex()) {
                    d->eventsInBindingLoop.insert(event.typeIndex());
                    break;
                }
            }
        }
        stack.push(event);
        break;
    case RangeEnd: {
        // update stats

        QmlEventStats *stats = &d->data[event.typeIndex()];
        qint64 duration = event.timestamp() - stack.top().timestamp();
        stats->duration += duration;
        stats->durationSelf += duration;
        if (duration < stats->minTime)
            stats->minTime = duration;
        if (duration > stats->maxTime)
            stats->maxTime = duration;
        stats->calls++;
        // for median computing
        d->durations[event.typeIndex()].append(duration);
        // qml time computation
        if (event.timestamp() > d->lastEndTime) { // assume parent event if starts before last end
            d->qmlTime += duration;
            d->lastEndTime = event.timestamp();
        }

        stack.pop();

        if (stack.count() > 1)
            d->data[stack.top().typeIndex()].durationSelf -= duration;
        break;
    }
    default:
        return;
    }

    if (!d->childrenModel.isNull())
        d->childrenModel->loadEvent(type.rangeType(), event);
    if (!d->parentsModel.isNull())
        d->parentsModel->loadEvent(type.rangeType(), event);
}


void QmlProfilerStatisticsModel::finalize()
{
    // post-process: calc mean time, median time, percentoftime
    for (QHash<int, QmlEventStats>::iterator it = d->data.begin(); it != d->data.end(); ++it) {
        QmlEventStats* stats = &it.value();
        if (stats->calls > 0)
            stats->timePerCall = stats->duration / (double)stats->calls;

        QVector<qint64> eventDurations = d->durations[it.key()];
        if (!eventDurations.isEmpty()) {
            Utils::sort(eventDurations);
            stats->medianTime = eventDurations.at(eventDurations.count()/2);
        }

        stats->percentOfTime = stats->duration * 100.0 / d->qmlTime;
        stats->percentSelf = stats->durationSelf * 100.0 / d->qmlTime;
    }

    // set binding loop flag
    foreach (int typeIndex, d->eventsInBindingLoop)
        d->data[typeIndex].isBindingLoop = true;

    // insert root event
    QmlEventStats rootEvent;
    rootEvent.duration = rootEvent.minTime = rootEvent.maxTime = rootEvent.timePerCall
                       = rootEvent.medianTime = d->qmlTime + 1;
    rootEvent.durationSelf = 1;
    rootEvent.calls = 1;
    rootEvent.percentOfTime = 100.0;
    rootEvent.percentSelf = 1.0 / rootEvent.duration;

    d->data.insert(-1, rootEvent);

    if (!d->childrenModel.isNull())
        d->childrenModel->finalize(d->eventsInBindingLoop);
    if (!d->parentsModel.isNull())
        d->parentsModel->finalize(d->eventsInBindingLoop);

    emit dataAvailable();
}

int QmlProfilerStatisticsModel::count() const
{
    return d->data.count();
}

QmlProfilerStatisticsRelativesModel::QmlProfilerStatisticsRelativesModel(
        QmlProfilerModelManager *modelManager, QmlProfilerStatisticsModel *statisticsModel,
        QmlProfilerStatisticsRelation relation, QObject *parent) :
    QObject(parent), m_relation(relation)
{
    QTC_CHECK(modelManager);
    m_modelManager = modelManager;

    QTC_CHECK(statisticsModel);
    statisticsModel->setRelativesModel(this, relation);

    // Load the child models whenever the parent model is done to get the filtering for JS/QML
    // right.
    connect(statisticsModel, &QmlProfilerStatisticsModel::dataAvailable,
            this, &QmlProfilerStatisticsRelativesModel::dataAvailable);
}

const QmlProfilerStatisticsRelativesModel::QmlStatisticsRelativesMap &
QmlProfilerStatisticsRelativesModel::getData(int typeId) const
{
    QHash <int, QmlStatisticsRelativesMap>::ConstIterator it = m_data.find(typeId);
    if (it != m_data.end()) {
        return it.value();
    } else {
        static const QmlStatisticsRelativesMap emptyMap;
        return emptyMap;
    }
}

const QVector<QmlEventType> &QmlProfilerStatisticsRelativesModel::getTypes() const
{
    return m_modelManager->qmlModel()->eventTypes();
}

void QmlProfilerStatisticsRelativesModel::loadEvent(RangeType type, const QmlEvent &event)
{
    QStack<Frame> &stack = (type == Compiling) ? m_compileStack : m_callStack;

    switch (event.rangeStage()) {
    case RangeStart:
        stack.push({event.timestamp(), event.typeIndex()});
        break;
    case RangeEnd: {
        int parentTypeIndex = stack.count() > 1 ? stack[stack.count() - 2].typeId : -1;
        int relativeTypeIndex = (m_relation == QmlProfilerStatisticsParents) ? parentTypeIndex :
                                                                               event.typeIndex();
        int selfTypeIndex = (m_relation == QmlProfilerStatisticsParents) ? event.typeIndex() :
                                                                           parentTypeIndex;

        QmlStatisticsRelativesMap &relativesMap = m_data[selfTypeIndex];
        QmlStatisticsRelativesMap::Iterator it = relativesMap.find(relativeTypeIndex);
        if (it != relativesMap.end()) {
            it.value().calls++;
            it.value().duration += event.timestamp() - stack.top().startTime;
        } else {
            QmlStatisticsRelativesData relative = {
                event.timestamp() - stack.top().startTime,
                1,
                false
            };
            relativesMap.insert(relativeTypeIndex, relative);
        }
        stack.pop();
        break;
    }
    default:
        break;
    }
}

void QmlProfilerStatisticsRelativesModel::finalize(const QSet<int> &eventsInBindingLoop)
{
    for (auto map = m_data.begin(), mapEnd = m_data.end(); map != mapEnd; ++map) {
        auto itemEnd = map->end();
        foreach (int typeIndex, eventsInBindingLoop) {
            auto item = map->find(typeIndex);
            if (item != itemEnd)
                item->isBindingLoop = true;
        }
    }
}

QmlProfilerStatisticsRelation QmlProfilerStatisticsRelativesModel::relation() const
{
    return m_relation;
}

int QmlProfilerStatisticsRelativesModel::count() const
{
    return m_data.count();
}

void QmlProfilerStatisticsRelativesModel::clear()
{
    m_data.clear();
    m_callStack.clear();
    m_compileStack.clear();
}

} // namespace QmlProfiler
