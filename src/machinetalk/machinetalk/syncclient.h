/****************************************************************************
**
** This code was generated by a code generator based on imatix/gsl
** Any changes in this code will be lost.
**
****************************************************************************/
#ifndef SYNC_CLIENT_H
#define SYNC_CLIENT_H
#include <QObject>
#include <QStateMachine>
#include <QQmlParserStatus>
#include <nzmqt/nzmqt.hpp>
#include <machinetalk/protobuf/message.pb.h>
#include <google/protobuf/text_format.h>
#include <machinetalk/rpcclient.h>
#include <machinetalk/subscribe.h>
#include <machinetalk/publish.h>

namespace machinetalk {

class SyncClient : public QObject
,public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(bool ready READ ready WRITE setReady NOTIFY readyChanged)
    Q_PROPERTY(QString syncUri READ syncUri WRITE setSyncUri NOTIFY syncUriChanged)
    Q_PROPERTY(QString subUri READ subUri WRITE setSubUri NOTIFY subUriChanged)
    Q_PROPERTY(QString pubUri READ pubUri WRITE setPubUri NOTIFY pubUriChanged)
    Q_PROPERTY(QString debugName READ debugName WRITE setDebugName NOTIFY debugNameChanged)
    Q_PROPERTY(State connectionState READ state NOTIFY stateChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
    Q_PROPERTY(int syncHeartbeatInterval READ syncHeartbeatInterval WRITE setSyncHeartbeatInterval NOTIFY syncHeartbeatIntervalChanged)
    Q_PROPERTY(int subHeartbeatInterval READ subHeartbeatInterval WRITE setSubHeartbeatInterval NOTIFY subHeartbeatIntervalChanged)
    Q_PROPERTY(int pubHeartbeatInterval READ pubHeartbeatInterval WRITE setPubHeartbeatInterval NOTIFY pubHeartbeatIntervalChanged)
    Q_ENUMS(State)

public:
    explicit SyncClient(QObject *parent = 0);
    ~SyncClient();

    enum State {
        Down = 0,
        Trying = 1,
        Syncing = 2,
        Synced = 3,
    };

    void classBegin() {}
    /** componentComplete is executed when the QML component is fully loaded */
    void componentComplete()
    {
        m_componentCompleted = true;

        if (m_ready == true)    // the component was set to ready before it was completed
        {
            start();
        }
    }

    QString syncUri() const
    {
        return m_syncChannel->socketUri();
    }

    QString subUri() const
    {
        return m_subChannel->socketUri();
    }

    QString pubUri() const
    {
        return m_pubChannel->socketUri();
    }

    QString debugName() const
    {
        return m_debugName;
    }

    State state() const
    {
        return m_state;
    }

    QString errorString() const
    {
        return m_errorString;
    }

    int syncHeartbeatInterval() const
    {
        return m_syncChannel->heartbeatInterval();
    }

    int subHeartbeatInterval() const
    {
        return m_subChannel->heartbeatInterval();
    }

    int pubHeartbeatInterval() const
    {
        return m_pubChannel->heartbeatInterval();
    }

    bool ready() const
    {
        return m_ready;
    }

public slots:

    void setSyncUri(QString uri)
    {
        m_syncChannel->setSocketUri(uri);
    }

    void setSubUri(QString uri)
    {
        m_subChannel->setSocketUri(uri);
    }

    void setPubUri(QString uri)
    {
        m_pubChannel->setSocketUri(uri);
    }

    void setDebugName(QString debugName)
    {
        if (m_debugName == debugName)
            return;

        m_debugName = debugName;
        emit debugNameChanged(debugName);
    }

    void setSyncHeartbeatInterval(int interval)
    {
        m_syncChannel->setHeartbeatInterval(interval);
    }

    void setSubHeartbeatInterval(int interval)
    {
        m_subChannel->setHeartbeatInterval(interval);
    }

    void setPubHeartbeatInterval(int interval)
    {
        m_pubChannel->setHeartbeatInterval(interval);
    }

    void setReady(bool ready)
    {
        if (m_ready == ready)
            return;

        m_ready = ready;
        emit readyChanged(ready);

        if (m_componentCompleted == false)
        {
            return;
        }

        if (m_ready)
        {
            start();
        }
        else
        {
            stop();
        }
    }


    void sendSyncMessage(pb::ContainerType type, pb::Container &tx);

    void sendPubMessage(pb::ContainerType type, pb::Container &tx);
    void sendIncrementalUpdate(pb::Container &tx);
    void addSubTopic(const QString &name);
    void removeSubTopic(const QString &name);
    void clearSubTopics();

protected:
    void start(); // start trigger
    void stop(); // stop trigger

private:
    bool m_componentCompleted;
    bool m_ready;
    QString m_debugName;

    machinetalk::RpcClient *m_syncChannel;
    QSet<QString> m_subTopics;      // the topics we are interested in
    machinetalk::Subscribe *m_subChannel;
    machinetalk::Publish *m_pubChannel;

    State         m_state;
    State         m_previousState;
    QStateMachine *m_fsm;
    QString       m_errorString;
    // more efficient to reuse a protobuf Messages
    pb::Container m_syncRx;
    pb::Container m_syncTx;
    pb::Container m_subRx;
    pb::Container m_pubTx;

private slots:

    void startSyncChannel();
    void stopSyncChannel();
    void syncChannelStateChanged(machinetalk::RpcClient::State state);
    void processSyncChannelMessage(const pb::Container &rx);
    void sendSync();

    void startSubChannel();
    void stopSubChannel();
    void subChannelStateChanged(machinetalk::Subscribe::State state);
    void processSubChannelMessage(const QByteArray &topic, const pb::Container &rx);

    void startPubChannel();
    void stopPubChannel();

    void fsmDownEntered();
    void fsmDownStartEvent();
    void fsmTryingEntered();
    void fsmTryingSyncStateUpEvent();
    void fsmTryingStopEvent();
    void fsmSyncingEntered();
    void fsmSyncingSyncStateTryingEvent();
    void fsmSyncingSubStateUpEvent();
    void fsmSyncingStopEvent();
    void fsmSyncedEntered();
    void fsmSyncedSubStateTryingEvent();
    void fsmSyncedSyncStateTryingEvent();
    void fsmSyncedStopEvent();


signals:

    void syncUriChanged(QString uri);
    void subUriChanged(QString uri);
    void pubUriChanged(QString uri);
    void syncMessageReceived(const pb::Container &rx);
    void subMessageReceived(const QByteArray &topic, const pb::Container &rx);
    void debugNameChanged(QString debugName);
    void stateChanged(SyncClient::State state);
    void errorStringChanged(QString errorString);
    void syncHeartbeatIntervalChanged(int interval);
    void subHeartbeatIntervalChanged(int interval);
    void pubHeartbeatIntervalChanged(int interval);
    void readyChanged(bool ready);
    // fsm
    void fsmDownStart();
    void fsmDownStartQueued();
    void fsmTryingSyncStateUp();
    void fsmTryingSyncStateUpQueued();
    void fsmTryingStop();
    void fsmTryingStopQueued();
    void fsmSyncingSyncStateTrying();
    void fsmSyncingSyncStateTryingQueued();
    void fsmSyncingSubStateUp();
    void fsmSyncingSubStateUpQueued();
    void fsmSyncingStop();
    void fsmSyncingStopQueued();
    void fsmSyncedSubStateTrying();
    void fsmSyncedSubStateTryingQueued();
    void fsmSyncedSyncStateTrying();
    void fsmSyncedSyncStateTryingQueued();
    void fsmSyncedStop();
    void fsmSyncedStopQueued();
};
}; // namespace machinetalk
#endif //SYNC_CLIENT_H