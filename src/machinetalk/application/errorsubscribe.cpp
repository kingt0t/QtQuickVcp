/****************************************************************************
**
** This code was generated by a code generator based on imatix/gsl
** Any changes in this code will be lost.
**
****************************************************************************/
#include "errorsubscribe.h"
#include "debughelper.h"

#if defined(Q_OS_IOS)
namespace gpb = google_public::protobuf;
#else
namespace gpb = google::protobuf;
#endif

using namespace nzmqt;

namespace application {

/** Generic Error Subscribe implementation */
ErrorSubscribe::ErrorSubscribe(QObject *parent) :
    QObject(parent),
    m_ready(false),
    m_debugName("Error Subscribe"),
    m_socketUri(""),
    m_context(nullptr),
    m_socket(nullptr),
    m_state(Down),
    m_previousState(Down),
    m_fsm(nullptr),
    m_errorString("")
    ,m_heartbeatTimer(new QTimer(this)),
    m_heartbeatInterval(0),
    m_heartbeatLiveness(0),
    m_heartbeatResetLiveness(2)
{

    m_heartbeatTimer->setSingleShot(true);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &ErrorSubscribe::heartbeatTimerTick);

    m_fsm = new QStateMachine(this);
    QState *downState = new QState(m_fsm);
    connect(downState, &QState::entered, this, &ErrorSubscribe::fsmDownEntered, Qt::QueuedConnection);
    QState *tryingState = new QState(m_fsm);
    connect(tryingState, &QState::entered, this, &ErrorSubscribe::fsmTryingEntered, Qt::QueuedConnection);
    QState *upState = new QState(m_fsm);
    connect(upState, &QState::entered, this, &ErrorSubscribe::fsmUpEntered, Qt::QueuedConnection);
    m_fsm->setInitialState(downState);
    m_fsm->start();

    connect(this, &ErrorSubscribe::fsmDownConnect,
            this, &ErrorSubscribe::fsmDownConnectQueued, Qt::QueuedConnection);
    downState->addTransition(this, &ErrorSubscribe::fsmDownConnectQueued, tryingState);
    connect(this, &ErrorSubscribe::fsmTryingConnected,
            this, &ErrorSubscribe::fsmTryingConnectedQueued, Qt::QueuedConnection);
    tryingState->addTransition(this, &ErrorSubscribe::fsmTryingConnectedQueued, upState);
    connect(this, &ErrorSubscribe::fsmTryingDisconnect,
            this, &ErrorSubscribe::fsmTryingDisconnectQueued, Qt::QueuedConnection);
    tryingState->addTransition(this, &ErrorSubscribe::fsmTryingDisconnectQueued, downState);
    connect(this, &ErrorSubscribe::fsmUpTimeout,
            this, &ErrorSubscribe::fsmUpTimeoutQueued, Qt::QueuedConnection);
    upState->addTransition(this, &ErrorSubscribe::fsmUpTimeoutQueued, tryingState);
    connect(this, &ErrorSubscribe::fsmUpTick,
            this, &ErrorSubscribe::fsmUpTickQueued, Qt::QueuedConnection);
    upState->addTransition(this, &ErrorSubscribe::fsmUpTickQueued, upState);
    connect(this, &ErrorSubscribe::fsmUpMessageReceived,
            this, &ErrorSubscribe::fsmUpMessageReceivedQueued, Qt::QueuedConnection);
    upState->addTransition(this, &ErrorSubscribe::fsmUpMessageReceivedQueued, upState);
    connect(this, &ErrorSubscribe::fsmUpDisconnect,
            this, &ErrorSubscribe::fsmUpDisconnectQueued, Qt::QueuedConnection);
    upState->addTransition(this, &ErrorSubscribe::fsmUpDisconnectQueued, downState);

    connect(this, &ErrorSubscribe::fsmDownConnect,
            this, &ErrorSubscribe::fsmDownConnectEvent, Qt::QueuedConnection);
    connect(this, &ErrorSubscribe::fsmTryingConnected,
            this, &ErrorSubscribe::fsmTryingConnectedEvent, Qt::QueuedConnection);
    connect(this, &ErrorSubscribe::fsmTryingDisconnect,
            this, &ErrorSubscribe::fsmTryingDisconnectEvent, Qt::QueuedConnection);
    connect(this, &ErrorSubscribe::fsmUpTimeout,
            this, &ErrorSubscribe::fsmUpTimeoutEvent, Qt::QueuedConnection);
    connect(this, &ErrorSubscribe::fsmUpTick,
            this, &ErrorSubscribe::fsmUpTickEvent, Qt::QueuedConnection);
    connect(this, &ErrorSubscribe::fsmUpMessageReceived,
            this, &ErrorSubscribe::fsmUpMessageReceivedEvent, Qt::QueuedConnection);
    connect(this, &ErrorSubscribe::fsmUpDisconnect,
            this, &ErrorSubscribe::fsmUpDisconnectEvent, Qt::QueuedConnection);

    m_context = new PollingZMQContext(this, 1);
    connect(m_context, &PollingZMQContext::pollError,
            this, &ErrorSubscribe::socketError);
    m_context->start();
}

ErrorSubscribe::~ErrorSubscribe()
{
    stopSocket();

    if (m_context != nullptr)
    {
        m_context->stop();
        m_context->deleteLater();
        m_context = nullptr;
    }
}

/** Add a topic that should be subscribed **/
void ErrorSubscribe::addSocketTopic(const QString &name)
{
    m_socketTopics.insert(name);
}

/** Removes a topic from the list of topics that should be subscribed **/
void ErrorSubscribe::removeSocketTopic(const QString &name)
{
    m_socketTopics.remove(name);
}

/** Clears the the topics that should be subscribed **/
void ErrorSubscribe::clearSocketTopics()
{
    m_socketTopics.clear();
}

/** Connects the 0MQ sockets */
bool ErrorSubscribe::startSocket()
{
    m_socket = m_context->createSocket(ZMQSocket::TYP_SUB, this);
    m_socket->setLinger(0);

    try {
        m_socket->connectTo(m_socketUri);
    }
    catch (const zmq::error_t &e) {
        QString errorString;
        errorString = QString("Error %1: ").arg(e.num()) + QString(e.what());
        //updateState(SocketError, errorString); TODO
        return false;
    }

    connect(m_socket, &ZMQSocket::messageReceived,
            this, &ErrorSubscribe::processSocketMessage);


    foreach(QString topic, m_socketTopics)
    {
        m_socket->subscribeTo(topic.toLocal8Bit());
    }

#ifdef QT_DEBUG
    DEBUG_TAG(1, m_debugName, "sockets connected" << m_socketUri);
#endif

    return true;
}

/** Disconnects the 0MQ sockets */
void ErrorSubscribe::stopSocket()
{
    if (m_socket != nullptr)
    {
        m_socket->close();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
}

void ErrorSubscribe::resetHeartbeatLiveness()
{
    m_heartbeatLiveness = m_heartbeatResetLiveness;
}

void ErrorSubscribe::resetHeartbeatTimer()
{
    if (m_heartbeatTimer->isActive())
    {
        m_heartbeatTimer->stop();
    }

    if (m_heartbeatInterval > 0)
    {
        m_heartbeatTimer->setInterval(m_heartbeatInterval);
        m_heartbeatTimer->start();
    }
}

void ErrorSubscribe::startHeartbeatTimer()
{
    resetHeartbeatTimer();
}

void ErrorSubscribe::stopHeartbeatTimer()
{
    m_heartbeatTimer->stop();
}

void ErrorSubscribe::heartbeatTimerTick()
{
    m_heartbeatLiveness -= 1;
    if (m_heartbeatLiveness == 0)
    {
         if (m_state == Up)
         {
             emit fsmUpTimeout();
         }
         return;
    }
    if (m_state == Up)
    {
         emit fsmUpTick();
    }
}

/** Processes all message received on socket */
void ErrorSubscribe::processSocketMessage(const QList<QByteArray> &messageList)
{
    pb::Container &rx = m_socketRx;
    QByteArray topic;

    if (messageList.length() < 2)  // in case we received insufficient data
    {
        return;
    }

    // we only handle the first two messges
    topic = messageList.at(0);
    rx.ParseFromArray(messageList.at(1).data(), messageList.at(1).size());

#ifdef QT_DEBUG
    std::string s;
    gpb::TextFormat::PrintToString(rx, &s);
    DEBUG_TAG(3, m_debugName, "server message" << QString::fromStdString(s));
#endif

    // react to any incoming message

    if (m_state == Up)
    {
        emit fsmUpMessageReceived();
    }

    // react to ping message
    if (rx.type() == pb::MT_PING)
    {
        if (rx.has_pparams())
        {
            pb::ProtocolParameters pparams = rx.pparams();
            m_heartbeatInterval = pparams.keepalive_timer();
        }

        if (m_state == Trying)
        {
            emit fsmTryingConnected();
        }
        return; // ping is uninteresting
    }

    emit socketMessageReceived(topic, rx);
}

void ErrorSubscribe::socketError(int errorNum, const QString &errorMsg)
{
    QString errorString;
    errorString = QString("Error %1: ").arg(errorNum) + errorMsg;
    //updateState(SocketError, errorString);  TODO
}

void ErrorSubscribe::fsmDownEntered()
{
    if (m_previousState != Down)
    {
#ifdef QT_DEBUG
    DEBUG_TAG(1, m_debugName, "State DOWN");
#endif
        m_previousState = Down;
        emit stateChanged(m_state);
    }
}

void ErrorSubscribe::fsmDownConnectEvent()
{
#ifdef QT_DEBUG
    DEBUG_TAG(1, m_debugName, "Event CONNECT");
#endif

    m_state = Trying;
    startSocket();
}

void ErrorSubscribe::fsmTryingEntered()
{
    if (m_previousState != Trying)
    {
#ifdef QT_DEBUG
    DEBUG_TAG(1, m_debugName, "State TRYING");
#endif
        m_previousState = Trying;
        emit stateChanged(m_state);
    }
}

void ErrorSubscribe::fsmTryingConnectedEvent()
{
#ifdef QT_DEBUG
    DEBUG_TAG(1, m_debugName, "Event CONNECTED");
#endif

    m_state = Up;
    resetHeartbeatLiveness();
    startHeartbeatTimer();
}

void ErrorSubscribe::fsmTryingDisconnectEvent()
{
#ifdef QT_DEBUG
    DEBUG_TAG(1, m_debugName, "Event DISCONNECT");
#endif

    m_state = Down;
    stopHeartbeatTimer();
    stopSocket();
}

void ErrorSubscribe::fsmUpEntered()
{
    if (m_previousState != Up)
    {
#ifdef QT_DEBUG
    DEBUG_TAG(1, m_debugName, "State UP");
#endif
        m_previousState = Up;
        emit stateChanged(m_state);
    }
}

void ErrorSubscribe::fsmUpTimeoutEvent()
{
#ifdef QT_DEBUG
    DEBUG_TAG(1, m_debugName, "Event TIMEOUT");
#endif

    m_state = Trying;
    stopHeartbeatTimer();
    stopSocket();
    startSocket();
}

void ErrorSubscribe::fsmUpTickEvent()
{
#ifdef QT_DEBUG
    DEBUG_TAG(1, m_debugName, "Event TICK");
#endif

    m_state = Up;
    resetHeartbeatTimer();
}

void ErrorSubscribe::fsmUpMessageReceivedEvent()
{
#ifdef QT_DEBUG
    DEBUG_TAG(1, m_debugName, "Event MESSAGE RECEIVED");
#endif

    m_state = Up;
    resetHeartbeatLiveness();
    resetHeartbeatTimer();
}

void ErrorSubscribe::fsmUpDisconnectEvent()
{
#ifdef QT_DEBUG
    DEBUG_TAG(1, m_debugName, "Event DISCONNECT");
#endif

    m_state = Down;
    stopHeartbeatTimer();
    stopSocket();
}

/** start trigger */
void ErrorSubscribe::start()
{
    if (m_state == Down) {
        emit fsmDownConnect();
    }
}

/** stop trigger */
void ErrorSubscribe::stop()
{
    if (m_state == Trying) {
        emit fsmTryingDisconnect();
    }
    if (m_state == Up) {
        emit fsmUpDisconnect();
    }
}
}; // namespace application
