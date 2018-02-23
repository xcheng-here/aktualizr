#include "sig_handler.h"
#include "logging.h"

void sigint_handler(int sig) {
  (void)sig;
  bool v = false;
  // put true if currently set to false
  SigHandler::sigint_marker_.compare_exchange_strong(v, true);
}

std::atomic<bool> SigHandler::sigint_marker_;

SigHandler& SigHandler::get() {
  static SigHandler handler;
  return handler;
}

SigHandler::~SigHandler() {
  if (polling_thread_.joinable()) {
    polling_thread_.join();
  }
}

void SigHandler::start(std::function<void()> on_sigint) {
  boost::unique_lock<boost::mutex> lock(m);

  if (polling_thread_.get_id() != boost::thread::id()) {
    throw std::runtime_error("SigHandler can only be started once");
  }

  polling_thread_ = boost::thread([this, on_sigint]() {
    while (true) {
      bool sigint = sigint_marker_.exchange(false);

      if (sigint) {
        LOG_INFO << "received SIGINT request";
        if (masked_secs_ != 0) {
          LOG_INFO << "SIGINT currently masked for " << masked_secs_ << " seconds, waiting...";
        }
        sigint_pending = true;
      }

      if (sigint_pending && masked_secs_ == 0) {
        LOG_INFO << "calling SIGINT handler";
        on_sigint();
        return;
      }

      boost::this_thread::sleep_for(boost::chrono::seconds(1));

      {
        boost::unique_lock<boost::mutex> lock(m);
        if (masked_secs_ > 0) {
          masked_secs_ -= 1;
        }
      }
    }
  });

  signal(SIGINT, sigint_handler);
}

bool SigHandler::masked() {
  boost::unique_lock<boost::mutex> lock(m);

  return masked_secs_ != 0;
}

void SigHandler::mask(int secs) {
  boost::unique_lock<boost::mutex> lock(m);

  LOG_INFO << "masking SIGINT for " << secs << " seconds";

  masked_secs_ = secs;
}
