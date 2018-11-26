#include <iostream>
#include <map>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include <openssl/ssl.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/signals2.hpp>

#include "config/config.h"
#include "logging/logging.h"
#include "primary/aktualizr.h"
#include "uptane/secondaryfactory.h"
#include "utilities/utils.h"

namespace po = boost::program_options;
namespace fs = boost::filesystem;

po::variables_map parse_options(int argc, char *argv[]) {
  po::options_description desc;
  // clang-format off
  desc.add_options()
      ("credentials,c", po::value<std::string>()->default_value("credentials.zip"), "path to credentials")

      ("working-dir,d", po::value<std::string>()->default_value("."),
      "working directory")

      ("loglevel", po::value<int>()->default_value(2),
       "log level 0-5 (trace, debug, info, warning, error, fatal)")

      ("version,v", "Current aktualizr version")
  ;
  // clang-format on
  try {
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    if (vm.count("version") != 0) {
      std::cout << "Current aktualizr version is: " << AKTUALIZR_VERSION << "\n";
      exit(EXIT_SUCCESS);
    }
    return vm;
  } catch (const po::error &ex) {
    LOG_ERROR << "command line option error: " << ex.what();
    exit(EXIT_FAILURE);
  }
}

std::string generate_device_id(void) {
  std::string prefix{"LB3TS90D1X1"};
  std::uniform_int_distribution<int> dist(0, 999999);
  std::random_device rd;
  std::stringstream stream;

  stream << prefix << std::setw(6) << std::setfill('0') << dist(rd);
  return stream.str();
}

int main(int argc, char *argv[]) {
  try {
    logger_init();
    auto vm = parse_options(argc, argv);

    Config primary;
    primary.logger.loglevel = vm["loglevel"].as<int>();
    primary.pacman.type = PackageManager::kNone;
    primary.provision.provision_path = vm["credentials"].as<std::string>();
    primary.provision.device_id = generate_device_id();
    primary.provision.primary_ecu_hardware_id = std::string("primary");
    primary.storage.path = fs::path(vm["working-dir"].as<std::string>()) / fs::path(primary.provision.device_id);
    primary.uptane.running_mode = RunningMode::kManual;
    primary.postUpdateValues();
    LOG_INFO << "Provisioning device " << primary.provision.device_id << " in " << primary.storage.path
             << " with credentials " << primary.provision.provision_path;

    const std::vector<std::string> ecus{"ASDM", "CEM", "DIM", "IHU", "SODL", "SRS", "VDDM"};
    for (const auto &ecu_id : ecus) {
      Uptane::SecondaryConfig ecu;
      ecu.secondary_type = Uptane::SecondaryType::kVirtual;
      ecu.ecu_hardware_id = ecu_id;
      ecu.full_client_dir = primary.storage.path / fs::path("ecus") / fs::path(ecu_id);
      ecu.metadata_path = ecu.full_client_dir / fs::path("metadata");
      ecu.target_name_path = ecu.full_client_dir / fs::path("target_name");
      ecu.firmware_path = ecu.full_client_dir / fs::path("firmware");
      ecu.ecu_public_key = std::string("sec.public");
      ecu.ecu_private_key = std::string("sec.private");
      primary.uptane.secondary_configs.push_back(ecu);
    }

    Aktualizr aktualizr(primary);
    aktualizr.Initialize();
    aktualizr.SendDeviceData().wait();
    aktualizr.CheckUpdates().wait();
  } catch (const std::exception &ex) {
    LOG_ERROR << "Fatal error in dev-factory: " << ex.what();
  }

  return EXIT_SUCCESS;
}
