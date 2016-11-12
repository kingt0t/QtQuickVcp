/****************************************************************************
**
** This code was generated by a code generator based on imatix/gsl
** Any changes in this code will be lost.
**
****************************************************************************/
#include "halrcompsubscribe.h"
#include "debughelper.h"

#if defined(Q_OS_IOS)
namespace gpb = google_public::protobuf;
#else
namespace gpb = google::protobuf;
#endif

using namespace nzmqt;

namespace halremote {

/** Generic Halrcomp Subscribe implementation */
HalrcompSubscribe::HalrcompSubscribe(QObject *parent) :
    QObject(parent),
    m_ready(false),
    m_debugName("Halrcomp Subscribe"),
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
    connect(m_heartbeatTimer, &QTimer::timeout, this, &HalrcompSubscribe::heartbeatTimerTick);

    m_fsm = new QStateMachine(this);
    QState *downState = new QState(m_fsm);
    connect(downState, &QState::entered, this, &HalrcompSubscribe::fsmDownEntered, Qt::QueuedConnection);
    QState *tryingState = new QState(m_fsm);
    connect(tryingState, &QState::entered, this, &HalrcompSubscribe::fsmTryingEntered, Qt::QueuedConnection);
    QState *upState = new QState(m_fsm);
    connect(upState, &QState::entered, this, &HalrcompSubscribe::fsmUpEntered, Qt::QueuedConnection);
    m_fsm->setInitialState(downState);
    m_fsm->start();

    connect(this, &HalrcompSubscribe::fsmDownConnect,
            this, &HalrcompSubscribe::fsmDownConnectQueued, Qt::QueuedConnection);
    downState->addTransition(this, &HalrcompSubscribe::fsmDownConnectQueued, tryingState);
    connect(this, &HalrcompSubscribe::fsmTryingConnected,
            this, &HalrcompSubscribe::fsmTryingConnectedQueued, Qt::QueuedConnection);
    tryingState->addTransition(this, &HalrcompSubscribe::fsmTryingConnectedQueued, upState);
    connect(this, &HalrcompSubscribe::fsmTryingDisconnect,
            this, &HalrcompSubscribe::fsmTryingDisconnectQueued, Qt::QueuedConnection);
    tryingState->addTransition(this, &HalrcompSubscribe::fsmTryingDisconnectQueued, downState);
    connect(this, &HalrcompSubscribe::fsmUpTimeout,
            this, &HalrcompSubscribe::fsmUpTimeoutQueued, Qt::QueuedConnection);
    upState->addTransition(this, &HalrcompSubscribe::fsmUpTimeoutQueued, tryingState);
    connect(this, &HalrcompSubscribe::fsmUpTick,
            this, &HalrcompSubscribe::fsmUpTickQueued, Qt::QueuedConnection);
    upState->addTransition(this, &HalrcompSubscribe::fsmUpTickQueued, upState);
    connect(this, &HalrcompSubscribe::fsmUpMessageReceived,
            this, &HalrcompSubscribe::fsmUpMessageReceivedQueued, Qt::QueuedConnection);
    upState->addTransition(this, &HalrcompSubscribe::fsmUpMessageReceivedQueued, upState);
    connect(this, &HalrcompSubscribe::fsmUpDisconnect,
            this, &HalrcompSubscribe::fsmUpDisconnectQueued, Qt::QueuedConnection);
    upState->addTransition(this, &HalrcompSubscribe::fsmUpDisconnectQueued, downState);

    connect(this, &HalrcompSubscribe::fsmDownConnect,
            this, &HalrcompSubscribe::fsmDownConnectEvent, Qt::QueuedConnection);
    connect(this, &HalrcompSubscribe::fsmTryingConnected,
            this, &HalrcompSubscribe::fsmTryingConnectedEvent, Qt::QueuedConnection);
    connect(this, &HalrcompSubscribe::fsmTryingDisconnect,
            this, &HalrcompSubscribe::fsmTryingDisconnectEvent, Qt::QueuedConnection);
    connect(this, &HalrcompSubscribe::fsmUpTimeout,
            this, &HalrcompSubscribe::fsmUpTimeoutEvent, Qt::QueuedConnection);
    connect(this, &HalrcompSubscribe::fsmUpTick,
            this, &HalrcompSubscribe::fsmUpTickEvent, Qt::QueuedConnection);
    connect(this, &HalrcompSubscribe::fsmUpMessageReceived,
            this, &HalrcompSubscribe::fsmUpMessageReceivedEvent, Qt::QueuedConnection);
    connect(this, &HalrcompSubscribe::fsmUpDisconnect,
            this, &HalrcompSubscribe::fsmUpDisconnectEvent, Qt::QueuedConnection);

    m_context = new PollingZMQContext(this, 1);
    connect(m_context, &PollingZMQContext::pollError,
            this, &HalrcompSubscribe::socketError);
    m_context->start();
}

HalrcompSubscribe::~HalrcompSubscribe()
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
void HalrcompSubscribe::addSocketTopic(const QString &name)
{
    m_socketTopics.insert(name);
}

/** Removes a topic from the list of topics that should be subscribed **/
void HalrcompSubscribe::removeSocketTopic(const QString &name)
{
    m_socketTopics.remove(name);
}

/** Clears the the topics that should be subscribed **/
void HalrcompSubscribe::clearSocketTopics()
{
    m_socketTopics.clear();
}

/** Connects the 0MQ sockets */
bool HalrcompSubscribe::startSocket()
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
            this, &HalrcompSubscribe::processSocketMessage);


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
void HalrcompSubscribe::stopSocket()
{
    if (m_socket != nullptr)
    {
        m_socket->close();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
}

void HalrcompSubscribe::resetHeartbeatLiveness()
{
    m_heartbeatLiveness = m_heartbeatResetLiveness;
}

void HalrcompSubscribe::resetHeartbeatTimer()
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

void HalrcompSubscribe::startHeartbeatTimer()
{
    resetHeartbeatTimer();
}

void HalrcompSubscribe::stopHeartbeatTimer()
{
    m_heartbeatTimer->stop();
}

void HalrcompSubscribe::heartbeatTimerTick()
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
void HalrcompSubscribe::processSocketMessage(const QList<QByteArray> &messageList)
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
        return; // ping is uninteresting
    }

    // react to halrcomp full update message
    if (rx.type() == pb::MT_HALRCOMP_FULL_UPDATE)
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
    }

    emit socketMessageReceived(topic, rx);
}

void HalrcompSubscribe::socketError(int errorNum, const QString &errorMsg)
{
    QString errorString;
    errorString = QString("Error %1: ").arg(errorNum) + errorMsg;
    //updateState(SocketError, errorString);  TODO
}

void HalrcompSubscribe::fsmDownEntered()
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

void HalrcompSubscribe::fsmDownConnectEvent()
{
#ifdef QT_DEBUG
    DEBUG_TAG(1, m_debugName, "Event CONNECT");
#endif

    m_state = Trying;
    startSocket();
}

void HalrcompSubscribe::fsmTryingEntered()
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

void HalrcompSubscribe::fsmTryingConnectedEvent()
{
#ifdef QT_DEBUG
    DEBUG_TAG(1, m_debugName, "Event CONNECTED");
#endif

    m_state = Up;
    resetHeartbeatLiveness();
    startHeartbeatTimer();
}

void HalrcompSubscribe::fsmTryingDisconnectEvent()
{
#ifdef QT_DEBUG
    DEBUG_TAG(1, m_debugName, "Event DISCONNECT");
#endif

    m_state = Down;
    stopHeartbeatTimer();
    stopSocket();
}

void HalrcompSubscribe::fsmUpEntered()
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

void HalrcompSubscribe::fsmUpTimeoutEvent()
{
#ifdef QT_DEBUG
    DEBUG_TAG(1, m_debugName, "Event TIMEOUT");
#endif

    m_state = Trying;
    stopHeartbeatTimer();
    stopSocket();
    startSocket();
}

void HalrcompSubscribe::fsmUpTickEvent()
{
#ifdef QT_DEBUG
    DEBUG_TAG(1, m_debugName, "Event TICK");
#endif

    m_state = Up;
    resetHeartbeatTimer();
}

void HalrcompSubscribe::fsmUpMessageReceivedEvent()
{
#ifdef QT_DEBUG
    DEBUG_TAG(1, m_debugName, "Event MESSAGE RECEIVED");
#endif

    m_state = Up;
    resetHeartbeatLiveness();
    resetHeartbeatTimer();
}

void HalrcompSubscribe::fsmUpDisconnectEvent()
{
#ifdef QT_DEBUG
    DEBUG_TAG(1, m_debugName, "Event DISCONNECT");
#endif

    m_state = Down;
    stopHeartbeatTimer();
    stopSocket();
}

/** start trigger */
void HalrcompSubscribe::start()
{
    if (m_state == Down) {
        emit fsmDownConnect();
    }
}

/** stop trigger */
void HalrcompSubscribe::stop()
{
    if (m_state == Trying) {
        emit fsmTryingDisconnect();
    }
    if (m_state == Up) {
        emit fsmUpDisconnect();
    }
}
}; // namespace halremote
