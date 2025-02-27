// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidLiveData/Kafka/KafkaEventListener.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidLiveData/Kafka/KafkaBroker.h"
#include "MantidLiveData/Kafka/KafkaEventStreamDecoder.h"
#include "MantidLiveData/Kafka/KafkaTopicSubscriber.h"

namespace {
Mantid::Kernel::Logger g_log("KafkaEventListener");
}

namespace Mantid::LiveData {

DECLARE_LISTENER(KafkaEventListener)

KafkaEventListener::KafkaEventListener() : API::LiveListener() {
  declareProperty("BufferThreshold", static_cast<uint64_t>(1000000),
                  "Threshold number of events at which the intermediate event "
                  "buffer will be flushed to the buffered EventWorkspace.");
}

void KafkaEventListener::setAlgorithm(const Mantid::API::IAlgorithm &callingAlgorithm) {
  this->updatePropertyValues(callingAlgorithm);
  // Get the instrument name from StartLiveData so we can sub to correct topics
  if (callingAlgorithm.existsProperty("Instrument")) {
    m_instrumentName = callingAlgorithm.getPropertyValue("Instrument");
  } else {
    g_log.error("KafkaEventListener requires Instrument property to be set in "
                "calling algorithm");
  }
}

/// @copydoc ILiveListener::connect
bool KafkaEventListener::connect(const Poco::Net::SocketAddress &address) {
  if (m_instrumentName.empty()) {
    g_log.error("KafkaEventListener::connect requires a non-empty instrument name");
  }

  auto instrumentInfo = Kernel::ConfigService::Instance().getInstrument(m_instrumentName);

  // Get topics from Facilites.xml if available otherwise use defaults.
  auto topics = instrumentInfo.topicInfoList();

  std::string eventTopic(m_instrumentName + KafkaTopicSubscriber::EVENT_TOPIC_SUFFIX),
      runInfoTopic(m_instrumentName + KafkaTopicSubscriber::RUN_TOPIC_SUFFIX),
      sampleEnvTopic(m_instrumentName + KafkaTopicSubscriber::SAMPLE_ENV_TOPIC_SUFFIX),
      chopperTopic(m_instrumentName + KafkaTopicSubscriber::CHOPPER_TOPIC_SUFFIX),
      monitorTopic(m_instrumentName + KafkaTopicSubscriber::MONITOR_TOPIC_SUFFIX);

  for (const auto &topic : topics) {
    switch (topic.type()) {
    case Kernel::TopicType::Event:
      eventTopic = topic.name();
      break;
    case Kernel::TopicType::Chopper:
      chopperTopic = topic.name();
      break;
    case Kernel::TopicType::Sample:
      sampleEnvTopic = topic.name();
      break;
    case Kernel::TopicType::Run:
      runInfoTopic = topic.name();
      break;
    case Kernel::TopicType::Monitor:
      monitorTopic = topic.name();
      break;
    }
  }

  const std::size_t bufferThreshold = getProperty("BufferThreshold");
  auto broker = std::make_shared<KafkaBroker>(address.toString());
  try {
    m_decoder = std::make_unique<KafkaEventStreamDecoder>(broker, eventTopic, runInfoTopic, sampleEnvTopic,
                                                          chopperTopic, monitorTopic, bufferThreshold);
  } catch (std::exception &exc) {
    g_log.error() << "KafkaEventListener::connect - Connection Error: " << exc.what() << "\n";
    return false;
  }
  return true;
}

/// @copydoc ILiveListener::start
void KafkaEventListener::start(Types::Core::DateAndTime startTime) {
  bool startNow = true;
  // Workaround for existing LiveListener interface
  // startTime of 0 means start from now
  // startTime of 1000000000 nanoseconds from epoch means start from start of
  // run and we do not support starting from arbitrary time in this listener
  if (startTime.totalNanoseconds() == 1000000000) {
    startNow = false;
  } else if (startTime != 0) {
    g_log.warning() << "KafkaLiveListener does not currently support starting "
                       "from arbitrary time."
                    << std::endl;
  }
  m_decoder->startCapture(startNow);
}

/// @copydoc ILiveListener::extractData
std::shared_ptr<API::Workspace> KafkaEventListener::extractData() {
  assert(m_decoder);
  // The first call to extract is very early in the start live data process
  // and we may not be completely ready yet, wait upto a maximum of 5 seconds
  // to become ready
  auto checkEnd = std::chrono::system_clock::now() + std::chrono::seconds(5);
  while (!m_decoder->hasData()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (std::chrono::system_clock::now() > checkEnd) {
      break;
    }
  }
  return m_decoder->extractData();
}

/// @copydoc ILiveListener::isConnected
bool KafkaEventListener::isConnected() { return (m_decoder ? m_decoder->isCapturing() : false); }

/// @copydoc ILiveListener::runStatus
API::ILiveListener::RunStatus KafkaEventListener::runStatus() {
  return m_decoder->hasReachedEndOfRun() ? EndRun : Running;
}

/// @copydoc ILiveListener::runNumber
int KafkaEventListener::runNumber() const { return (m_decoder ? m_decoder->runNumber() : -1); }

/// @copydoc ILiveListener::dataReset
bool KafkaEventListener::dataReset() { return (m_decoder ? m_decoder->dataReset() : false); }
} // namespace Mantid::LiveData
