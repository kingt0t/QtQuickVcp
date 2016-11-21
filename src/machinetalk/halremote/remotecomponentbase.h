/****************************************************************************
**
** This code was generated by a code generator based on imatix/gsl
** Any changes in this code will be lost.
**
****************************************************************************/
#ifndef REMOTE_COMPONENT_BASE_H
#define REMOTE_COMPONENT_BASE_H
#include <QObject>
#include <QQmlParserStatus>
#include <nzmqt/nzmqt.hpp>
#include <machinetalk/protobuf/message.pb.h>
#include <google/protobuf/text_format.h>
#include <machinetalk/rpcclient.h>
#include <halremote/halrcompsubscribe.h>

namespace halremote {

class RemoteComponentBase : public QObject
,public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(bool ready READ ready WRITE setReady NOTIFY readyChanged)
    Q_PROPERTY(QString halrcmdUri READ halrcmdUri WRITE setHalrcmdUri NOTIFY halrcmdUriChanged)
    Q_PROPERTY(QString halrcompUri READ halrcompUri WRITE setHalrcompUri NOTIFY halrcompUriChanged)
    Q_PROPERTY(QString debugName READ debugName WRITE setDebugName NOTIFY debugNameChanged)
    Q_PROPERTY(State connectionState READ state NOTIFY stateChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
    Q_PROPERTY(int halrcmdHeartbeatInterval READ halrcmdHeartbeatInterval WRITE setHalrcmdHeartbeatInterval NOTIFY halrcmdHeartbeatIntervalChanged)
    Q_PROPERTY(int halrcompHeartbeatInterval READ halrcompHeartbeatInterval WRITE setHalrcompHeartbeatInterval NOTIFY halrcompHeartbeatIntervalChanged)
    Q_ENUMS(State)

public:
    explicit RemoteComponentBase(QObject *parent = 0);
    ~RemoteComponentBase();

    enum State {
        Down = 0,
        Trying = 1,
        Bind = 2,
        Binding = 3,
        Syncing = 4,
        Sync = 5,
        Synced = 6,
        Error = 7,
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

    QString halrcmdUri() const
    {
        return m_halrcmdChannel->socketUri();
    }

    QString halrcompUri() const
    {
        return m_halrcompChannel->socketUri();
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

    int halrcmdHeartbeatInterval() const
    {
        return m_halrcmdChannel->heartbeatInterval();
    }

    int halrcompHeartbeatInterval() const
    {
        return m_halrcompChannel->heartbeatInterval();
    }

    bool ready() const
    {
        return m_ready;
    }

public slots:

    void setHalrcmdUri(QString uri)
    {
        m_halrcmdChannel->setSocketUri(uri);
    }

    void setHalrcompUri(QString uri)
    {
        m_halrcompChannel->setSocketUri(uri);
    }

    void setDebugName(QString debugName)
    {
        if (m_debugName == debugName)
            return;

        m_debugName = debugName;
        emit debugNameChanged(debugName);
    }

    void setHalrcmdHeartbeatInterval(int interval)
    {
        m_halrcmdChannel->setHeartbeatInterval(interval);
    }

    void setHalrcompHeartbeatInterval(int interval)
    {
        m_halrcompChannel->setHeartbeatInterval(interval);
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


    void sendHalrcmdMessage(pb::ContainerType type, pb::Container &tx);
    void sendHalrcompBind(pb::Container &tx);
    void sendHalrcompSet(pb::Container &tx);
    void addHalrcompTopic(const QString &name);
    void removeHalrcompTopic(const QString &name);
    void clearHalrcompTopics();

protected:
    void noBind(); // no bind trigger
    void pinsSynced(); // pins synced trigger
    void start(); // start trigger
    void stop(); // stop trigger

private:
    bool m_componentCompleted;
    bool m_ready;
    QString m_debugName;

    machinetalk::RpcClient *m_halrcmdChannel;
    QSet<QString> m_halrcompTopics;  // the topics we are interested in
    halremote::HalrcompSubscribe *m_halrcompChannel;

    State         m_state;
    State         m_previousState;
    QString       m_errorString;
    // more efficient to reuse a protobuf Messages
    pb::Container m_halrcmdRx;
    pb::Container m_halrcmdTx;
    pb::Container m_halrcompRx;

private slots:

    void startHalrcmdChannel();
    void stopHalrcmdChannel();
    void halrcmdChannelStateChanged(machinetalk::RpcClient::State state);
    void processHalrcmdChannelMessage(const pb::Container &rx);

    void startHalrcompChannel();
    void stopHalrcompChannel();
    void halrcompChannelStateChanged(halremote::HalrcompSubscribe::State state);
    void processHalrcompChannelMessage(const QByteArray &topic, const pb::Container &rx);

    void fsmDown();
    void fsmDownEntry();
    void fsmDownExit();
    void fsmDownConnectEvent();
    void fsmTrying();
    void fsmTryingHalrcmdUpEvent();
    void fsmTryingDisconnectEvent();
    void fsmBind();
    void fsmBindHalrcompBindMsgSentEvent();
    void fsmBindNoBindEvent();
    void fsmBinding();
    void fsmBindingBindConfirmedEvent();
    void fsmBindingBindRejectedEvent();
    void fsmBindingHalrcmdTryingEvent();
    void fsmBindingDisconnectEvent();
    void fsmSyncing();
    void fsmSyncingHalrcmdTryingEvent();
    void fsmSyncingHalrcompUpEvent();
    void fsmSyncingSyncFailedEvent();
    void fsmSyncingDisconnectEvent();
    void fsmSync();
    void fsmSyncPinsSyncedEvent();
    void fsmSynced();
    void fsmSyncedEntry();
    void fsmSyncedHalrcompTryingEvent();
    void fsmSyncedHalrcmdTryingEvent();
    void fsmSyncedSetRejectedEvent();
    void fsmSyncedHalrcompSetMsgSentEvent();
    void fsmSyncedDisconnectEvent();
    void fsmError();
    void fsmErrorEntry();
    void fsmErrorDisconnectEvent();

    virtual void halrcompFullUpdateReceived(const QByteArray &topic, const pb::Container &rx) = 0;
    virtual void halrcompIncrementalUpdateReceived(const QByteArray &topic, const pb::Container &rx) = 0;
    virtual void halrcompErrorReceived(const QByteArray &topic, const pb::Container &rx) = 0;
    virtual void bindComponent() = 0;
    virtual void addPins() = 0;
    virtual void removePins() = 0;
    virtual void unsyncPins() = 0;
    virtual void setConnected() = 0;
    virtual void setError() = 0;
    virtual void setDisconnected() = 0;
    virtual void setConnecting() = 0;
    virtual void setTimeout() = 0;

signals:
    void halrcmdUriChanged(QString uri);
    void halrcompUriChanged(QString uri);
    void halrcmdMessageReceived(const pb::Container &rx);
    void halrcompMessageReceived(const QByteArray &topic, const pb::Container &rx);
    void debugNameChanged(QString debugName);
    void stateChanged(RemoteComponentBase::State state);
    void errorStringChanged(QString errorString);
    void halrcmdHeartbeatIntervalChanged(int interval);
    void halrcompHeartbeatIntervalChanged(int interval);
    void readyChanged(bool ready);
    // fsm
    void fsmDownEntered(QPrivateSignal);
    void fsmDownExited(QPrivateSignal);
    void fsmDownConnect(QPrivateSignal);
    void fsmTryingEntered(QPrivateSignal);
    void fsmTryingExited(QPrivateSignal);
    void fsmTryingHalrcmdUp(QPrivateSignal);
    void fsmTryingDisconnect(QPrivateSignal);
    void fsmBindEntered(QPrivateSignal);
    void fsmBindExited(QPrivateSignal);
    void fsmBindHalrcompBindMsgSent(QPrivateSignal);
    void fsmBindNoBind(QPrivateSignal);
    void fsmBindingEntered(QPrivateSignal);
    void fsmBindingExited(QPrivateSignal);
    void fsmBindingBindConfirmed(QPrivateSignal);
    void fsmBindingBindRejected(QPrivateSignal);
    void fsmBindingHalrcmdTrying(QPrivateSignal);
    void fsmBindingDisconnect(QPrivateSignal);
    void fsmSyncingEntered(QPrivateSignal);
    void fsmSyncingExited(QPrivateSignal);
    void fsmSyncingHalrcmdTrying(QPrivateSignal);
    void fsmSyncingHalrcompUp(QPrivateSignal);
    void fsmSyncingSyncFailed(QPrivateSignal);
    void fsmSyncingDisconnect(QPrivateSignal);
    void fsmSyncEntered(QPrivateSignal);
    void fsmSyncExited(QPrivateSignal);
    void fsmSyncPinsSynced(QPrivateSignal);
    void fsmSyncedEntered(QPrivateSignal);
    void fsmSyncedExited(QPrivateSignal);
    void fsmSyncedHalrcompTrying(QPrivateSignal);
    void fsmSyncedHalrcmdTrying(QPrivateSignal);
    void fsmSyncedSetRejected(QPrivateSignal);
    void fsmSyncedHalrcompSetMsgSent(QPrivateSignal);
    void fsmSyncedDisconnect(QPrivateSignal);
    void fsmErrorEntered(QPrivateSignal);
    void fsmErrorExited(QPrivateSignal);
    void fsmErrorDisconnect(QPrivateSignal);
};
}; // namespace halremote
#endif //REMOTE_COMPONENT_BASE_H
