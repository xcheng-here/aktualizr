#include "check.h"

#include <random>
#include <string>

#include "context.h"
#include "executor.h"
#include "primary/aktualizr.h"
#include "primary/reportqueue.h"
#include "primary/sotauptaneclient.h"
#include "storage/invstorage.h"
#include "storage/sqlstorage.h"
#include "uptane/uptanerepository.h"
#include "utilities/events.h"

namespace fs = boost::filesystem;

class EphemeralStorage : public SQLStorage {
 public:
  EphemeralStorage(const StorageConfig &config, bool readonly) : SQLStorage(config, readonly) {}
  void storeRoot(const std::string &data, Uptane::RepositoryType repo, Uptane::Version version) override {
    (void)data;
    (void)repo;
    (void)version;
  };
  void storeNonRoot(const std::string &data, Uptane::RepositoryType repo, Uptane::Role role) override {
    (void)data;
    (void)repo;
    (void)role;
  };

  static std::shared_ptr<INvStorage> newStorage(const StorageConfig &config) {
    return std::make_shared<EphemeralStorage>(config, false);
  }
};

class CheckForUpdate {
  Config config;
  std::shared_ptr<Aktualizr> aktualizr;

  static Config copyConfig(const Config src) {
    Config copy {src};
    const fs::path srcDb = src.storage.sqldb_path.get(src.storage.path);
    const fs::path dstDb = fs::temp_directory_path() / fs::unique_path();
    fs::copy(srcDb, dstDb);
    copy.storage.sqldb_path = BasedPath{dstDb};
    LOG_DEBUG << "Copied " << srcDb << " to " << dstDb;
    return copy;
  }
 public:
  CheckForUpdate(Config config_)
      : config {copyConfig(config_)}, aktualizr{std::make_shared<Aktualizr>(config)} {}

  void operator()() {
    try {
      aktualizr->Initialize();
      aktualizr->CampaignCheck();
    } catch (const std::exception &e) {
      LOG_ERROR << "Unable to get new targets: " << e.what();
    } catch (...) {
      LOG_ERROR << "Unknown error occured while checking for updates";
    }
  }
};

class CheckForUpdateTasks {
  std::vector<Config> configs;

  std::mt19937 rng;

  std::uniform_int_distribution<size_t> gen;

 public:
  CheckForUpdateTasks(const boost::filesystem::path baseDir)
      : configs{loadDeviceConfigurations(baseDir)}, gen(0UL, configs.size() - 1) {
    std::random_device seedGen;
    rng.seed(seedGen());
  }

  CheckForUpdate nextTask() { return CheckForUpdate{configs[gen(rng)]}; }
};

void checkForUpdates(const boost::filesystem::path &baseDir, const unsigned int rate, const unsigned int nr,
                     const unsigned int parallelism) {
  LOG_INFO << "Target rate: " << rate << "op/s, operations: " << nr << ", workers: " << parallelism;
  std::vector<CheckForUpdateTasks> feeds(parallelism, CheckForUpdateTasks{baseDir});
  std::unique_ptr<ExecutionController> execController;
  if (nr == 0) {
    execController = std_::make_unique<InterruptableExecutionController>();
  } else {
    execController = std_::make_unique<FixedExecutionController>(nr);
  }
  Executor<CheckForUpdateTasks> exec{feeds, rate, std::move(execController), "Check for updates"};
  exec.run();
}
