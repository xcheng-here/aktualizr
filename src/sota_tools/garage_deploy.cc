#include <string>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include "accumulator.h"
#include "authenticate.h"
#include "check.h"
#include "deploy.h"
#include "garage_common.h"
#include "garage_tools_version.h"
#include "logging/logging.h"
#include "ostree_http_repo.h"

namespace po = boost::program_options;

int main(int argc, char **argv) {
  logger_init();

  int verbosity;
  std::string ostree_commit;
  std::string name;
  boost::filesystem::path fetch_cred;
  boost::filesystem::path push_cred;
  std::string hardwareids;
  std::string cacerts;
  int max_curl_requests;
  RunMode mode = RunMode::kDefault;
  po::options_description desc("garage-deploy command line options");
  // clang-format off
  desc.add_options()
    ("help", "print usage")
    ("version", "Current garage-deploy version")
    ("verbose,v", accumulator<int>(&verbosity), "Verbose logging (use twice for more information)")
    ("quiet,q", "Quiet mode")
    ("commit", po::value<std::string>(&ostree_commit)->required(), "OSTree commit to deploy")
    ("name", po::value<std::string>(&name)->required(), "Name of image")
    ("fetch-credentials,f", po::value<boost::filesystem::path>(&fetch_cred)->required(), "path to source credentials")
    ("push-credentials,p", po::value<boost::filesystem::path>(&push_cred)->required(), "path to destination credentials")
    ("hardwareids,h", po::value<std::string>(&hardwareids)->required(), "list of hardware ids")
    ("cacert", po::value<std::string>(&cacerts), "override path to CA root certificates, in the same format as curl --cacert")
    ("jobs", po::value<int>(&max_curl_requests)->default_value(30), "maximum number of parallel requests")
    ("dry-run,n", "check arguments and authenticate but don't upload");
  // clang-format on

  po::variables_map vm;

  try {
    po::store(po::parse_command_line(argc, reinterpret_cast<const char *const *>(argv), desc), vm);

    if (vm.count("help") != 0u) {
      LOG_INFO << desc;
      return EXIT_SUCCESS;
    }
    if (vm.count("version") != 0) {
      LOG_INFO << "Current garage-deploy version is: " << garage_tools_version();
      exit(EXIT_SUCCESS);
    }
    po::notify(vm);
  } catch (const po::error &o) {
    LOG_ERROR << o.what();
    LOG_ERROR << desc;
    return EXIT_FAILURE;
  }

  // Configure logging
  if (verbosity == 0) {
    // 'verbose' trumps 'quiet'
    if (static_cast<int>(vm.count("quiet")) != 0) {
      logger_set_threshold(boost::log::trivial::warning);
    } else {
      logger_set_threshold(boost::log::trivial::info);
    }
  } else if (verbosity == 1) {
    logger_set_threshold(boost::log::trivial::debug);
    LOG_DEBUG << "Debug level debugging enabled";
  } else if (verbosity > 1) {
    logger_set_threshold(boost::log::trivial::trace);
    LOG_TRACE << "Trace level debugging enabled";
  } else {
    assert(0);
  }

  Utils::setUserAgent(std::string("garage-deploy/") + garage_tools_version());

  if (vm.count("dry-run") != 0u) {
    mode = RunMode::kDryRun;
  }

  if (max_curl_requests < 1) {
    LOG_FATAL << "--jobs must be greater than 0";
    return EXIT_FAILURE;
  }

  ServerCredentials fetch_credentials(fetch_cred);
  TreehubServer fetch_server;
  if (authenticate(cacerts, fetch_credentials, fetch_server) != EXIT_SUCCESS) {
    LOG_FATAL << "Authentication with fetch server failed";
    return EXIT_FAILURE;
  }

  ServerCredentials push_credentials(push_cred);
  TreehubServer push_server;
  if (authenticate(cacerts, push_credentials, push_server) != EXIT_SUCCESS) {
    LOG_FATAL << "Authentication with push server failed";
    return EXIT_FAILURE;
  }

  OSTreeRepo::ptr src_repo = std::make_shared<OSTreeHttpRepo>(&fetch_server);
  try {
    OSTreeHash commit(OSTreeHash::Parse(ostree_commit));
    // Since the fetches happen on a single thread in OSTreeHttpRepo, there
    // isn't much reason to upload in parallel, but why hold the system back if
    // the fetching is faster than the uploading?
    if (!UploadToTreehub(src_repo, push_server, commit, mode, max_curl_requests)) {
      LOG_FATAL << "Upload to treehub failed";
      return EXIT_FAILURE;
    }

    if (mode == RunMode::kDefault || mode == RunMode::kPushTree) {
      if (!push_credentials.CanSignOffline()) {
        LOG_FATAL << "Provided push credentials are missing required components to sign Targets metadata.";
        return EXIT_FAILURE;
      }
      if (!OfflineSignRepo(ServerCredentials(push_credentials.GetPathOnDisk()), name, commit, hardwareids)) {
        return EXIT_FAILURE;
      }

      if (CheckRefValid(push_server, ostree_commit, mode, max_curl_requests) != EXIT_SUCCESS) {
        LOG_FATAL << "Check if the ref is present on the server or in targets.json failed";
        return EXIT_FAILURE;
      }
    } else {
      LOG_INFO << "Dry run. Not attempting offline signing.";
    }
  } catch (OSTreeCommitParseError &e) {
    LOG_FATAL << e.what();
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
// vim: set tabstop=2 shiftwidth=2 expandtab:
