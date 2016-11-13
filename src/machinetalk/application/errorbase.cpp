/****************************************************************************
**
** This code was generated by a code generator based on imatix/gsl
** Any changes in this code will be lost.
**
****************************************************************************/
#include "errorbase.h"
#include "debughelper.h"

#if defined(Q_OS_IOS)
namespace gpb = google_public::protobuf;
#else
namespace gpb = google::protobuf;
#endif

using namespace nzmqt;

namespace application {

/** Generic Error Base implementation */
ErrorBase::ErrorBase(QObject *parent) :
    QObject(parent),
    QQmlParserStatus(),
    m_componentCompleted(false),
    m_ready(false),
    m_debugName("Error Base"),
    m_errorChannel(nullptr),
    m_state(Down),
    m_previousState(Down),
    m_fsm(nullptr),
    m_errorString("")
{
    // initialize error channel
    m_errorChannel = new application::ErrorSubscribe(this);
    m_errorChannel->setDebugName(m_debugName + " - error");
    connect(m_errorChannel, &application::ErrorSubscribe::socketUriChanged,
            this, &ErrorBase::errorUriChanged);
    connect(m_errorChannel, &application::ErrorSubscribe::stateChanged,
            this, &ErrorBase::errorChannelStateChanged);
    connect(m_errorChannel, &application::ErrorSubscribe::socketMessageReceived,
            this, &ErrorBase::processErrorChannelMessage);

    connect(m_errorChannel, &application::ErrorSubscribe::heartbeatIntervalChanged,
            this, &ErrorBase::errorHeartbeatIntervalChanged);

    m_fsm = new QStateMachine(this);
    QState *downState = new QState(m_fsm);
    connect(downState, &QState::entered, this, &ErrorBase::fsmDownEntered, Qt::QueuedConnection);
    QState *tryingState = new QState(m_fsm);
    connect(tryingState, &QState::entered, this, &ErrorBase::fsmTryingEntered, Qt::QueuedConnection);
    QState *upState = new QState(m_fsm);
    connect(upState, &QState::entered, this, &ErrorBase::fsmUpEntered, Qt::QueuedConnection);
    connect(upState, &QState::entered, this, &ErrorBase::setConnected, Qt::QueuedConnection);
    connect(upState, &QState::exited, this, &ErrorBase::clearConnected, Qt::QueuedConnection);
    m_fsm->setInitialState(downState);
    m_fsm->start();

    connect(this, &ErrorBase::fsmDownConnect,
            this, &ErrorBase::fsmDownConnectQueued, Qt::QueuedConnection);
    downState->addTransition(this, &ErrorBase::fsmDownConnectQueued, tryingState);
    connect(this, &ErrorBase::fsmTryingErrorUp,
            this, &ErrorBase::fsmTryingErrorUpQueued, Qt::QueuedConnection);
    tryingState->addTransition(this, &ErrorBase::fsmTryingErrorUpQueued, upState);
    connect(this, &ErrorBase::fsmTryingDisconnect,
            this, &ErrorBase::fsmTryingDisconnectQueued, Qt::QueuedConnection);
    tryingState->addTransition(this, &ErrorBase::fsmTryingDisconnectQueued, downState);
    connect(this, &ErrorBase::fsmUpErrorTrying,
            this, &ErrorBase::fsmUpErrorTryingQueued, Qt::QueuedConnection);
    upState->addTransition(this, &ErrorBase::fsmUpErrorTryingQueued, tryingState);
    connect(this, &ErrorBase::fsmUpDisconnect,
            this, &ErrorBase::fsmUpDisconnectQueued, Qt::QueuedConnection);
    upState->addTransition(this, &ErrorBase::fsmUpDisconnectQueued, downState);

    connect(this, &ErrorBase::fsmDownConnect,
            this, &ErrorBase::fsmDownConnectEvent, Qt::QueuedConnection);
    connect(this, &ErrorBase::fsmTryingErrorUp,
            this, &ErrorBase::fsmTryingErrorUpEvent, Qt::QueuedConnection);
    connect(this, &ErrorBase::fsmTryingDisconnect,
            this, &ErrorBase::fsmTryingDisconnectEvent, Qt::QueuedConnection);
    connect(this, &ErrorBase::fsmUpErrorTrying,
            this, &ErrorBase::fsmUpErrorTryingEvent, Qt::QueuedConnection);
    connect(this, &ErrorBase::fsmUpDisconnect,
            this, &ErrorBase::fsmUpDisconnectEvent, Qt::QueuedConnection);
}

ErrorBase::~ErrorBase()
{
}

/** Add a topic that should be subscribed **/
void ErrorBase::addErrorTopic(const QString &name)
{
    m_errorChannel->addSocketTopic(name);
}

/** Removes a topic from the list of topics that should be subscribed **/
void ErrorBase::removeErrorTopic(const QString &name)
{
    m_errorChannel->removeSocketTopic(name);
}

/** Clears the the topics that should be subscribed **/
void ErrorBase::clearErrorTopics()
{
    m_errorChannel->clearSocketTopics();
}

void ErrorBase::startErrorChannel()
{
    m_errorChannel->setReady(true);
}

void ErrorBase::stopErrorChannel()
{
    m_errorChannel->setReady(false);
}

/** Processes all message received on error */
void ErrorBase::processErrorChannelMessage(const QByteArray &topic, const pb::Container &rx)
{

    // react to emc nml error message
    if (rx.type() == pb::MT_EMC_NML_ERROR)
    {
        emcNmlErrorReceived(topic, rx);
    }

    // react to emc nml text message
    if (rx.type() == pb::MT_EMC_NML_TEXT)
    {
        emcNmlTextReceived(topic, rx);
    }

    // react to emc nml display message
    if (rx.type() == pb::MT_EMC_NML_DISPLAY)
    {
        emcNmlDisplayReceived(topic, rx);
    }

    // react to emc operator text message
    if (rx.type() == pb::MT_EMC_OPERATOR_TEXT)
    {
        emcOperatorTextReceived(topic, rx);
    }

    // react to emc operator error message
    if (rx.type() == pb::MT_EMC_OPERATOR_ERROR)
    {
        emcOperatorErrorReceived(topic, rx);
    }

    // react to emc operator display message
    if (rx.type() == pb::MT_EMC_OPERATOR_DISPLAY)
    {
        emcOperatorDisplayReceived(topic, rx);
    }

    emit errorMessageReceived(topic, rx);
}

void ErrorBase::fsmDownEntered()
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

void ErrorBase::fsmDownConnectEvent()
{
#ifdef QT_DEBUG
    DEBUG_TAG(1, m_debugName, "Event CONNECT");
#endif

    m_state = Trying;
    updateTopics();
    startErrorChannel();
}

void ErrorBase::fsmTryingEntered()
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

void ErrorBase::fsmTryingErrorUpEvent()
{
#ifdef QT_DEBUG
    DEBUG_TAG(1, m_debugName, "Event ERROR UP");
#endif

    m_state = Up;
}

void ErrorBase::fsmTryingDisconnectEvent()
{
#ifdef QT_DEBUG
    DEBUG_TAG(1, m_debugName, "Event DISCONNECT");
#endif

    m_state = Down;
    stopErrorChannel();
}

void ErrorBase::fsmUpEntered()
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

void ErrorBase::fsmUpErrorTryingEvent()
{
#ifdef QT_DEBUG
    DEBUG_TAG(1, m_debugName, "Event ERROR TRYING");
#endif

    m_state = Trying;
}

void ErrorBase::fsmUpDisconnectEvent()
{
#ifdef QT_DEBUG
    DEBUG_TAG(1, m_debugName, "Event DISCONNECT");
#endif

    m_state = Down;
    stopErrorChannel();
}

void ErrorBase::errorChannelStateChanged(application::ErrorSubscribe::State state)
{

    if (state == application::ErrorSubscribe::Trying)
    {
        if (m_state == Up)
        {
            emit fsmUpErrorTrying();
        }
    }

    if (state == application::ErrorSubscribe::Up)
    {
        if (m_state == Trying)
        {
            emit fsmTryingErrorUp();
        }
    }
}

/** start trigger */
void ErrorBase::start()
{
    if (m_state == Down) {
        emit fsmDownConnect();
    }
}

/** stop trigger */
void ErrorBase::stop()
{
    if (m_state == Trying) {
        emit fsmTryingDisconnect();
    }
    if (m_state == Up) {
        emit fsmUpDisconnect();
    }
}
}; // namespace application