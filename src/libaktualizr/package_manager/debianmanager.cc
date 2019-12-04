#include "package_manager/debianmanager.h"

#include <stdio.h>
#include <unistd.h>

Json::Value DebianManager::getInstalledPackages() const {
  // Currently not implemented
  return Json::Value(Json::arrayValue);
}

data::InstallationResult DebianManager::install(const Uptane::Target &target) const {
  std::lock_guard<std::mutex> guard(mutex_);
  LOG_INFO << "Installing " << target.filename() << " as Debian package...";
  std::string cmd = "dpkg -i ";
  std::string output;
  TemporaryDirectory package_dir("deb_dir");
  auto target_file = storage_->openTargetFile(target);

  boost::filesystem::path deb_path = package_dir / target.filename();
  target_file->writeToFile(deb_path);
  target_file->rclose();

  int status = Utils::shell(cmd + deb_path.string(), &output, true);
  if (status == 0) {
    LOG_INFO << "... Installation of Debian package successful";
    storage_->savePrimaryInstalledVersion(target, InstalledVersionUpdateMode::kCurrent);
    return data::InstallationResult(data::ResultCode::Numeric::kOk, "Installing debian package was successful");
  }
  LOG_ERROR << "... Installation of Debian package failed";
  return data::InstallationResult(data::ResultCode::Numeric::kInstallFailed, output);
}

Uptane::Target DebianManager::getCurrent() const {
  boost::optional<Uptane::Target> current_version;
  storage_->loadPrimaryInstalledVersions(&current_version, nullptr);

  if (!!current_version) {
    return *current_version;
  }

  return Uptane::Target::Unknown();
}
