#include <iostream>
#include <string>
#include <vector>
#include <boost/filesystem.hpp>

#include "primary/aktualizr.h"
#include "uptane/virtualsecondary.h"

int main(int argc, char *argv[]) {

  try {
    boost::filesystem::path cfg_path("/var/sota/sota_local.toml");
    Config cfg(cfg_path);

    Aktualizr aktualizr(cfg);
    Uptane::SecondaryConfig sconfig;
    sconfig.secondary_type = Uptane::SecondaryType::kVirtual;
    sconfig.ecu_serial = "9b6abd606a761074df0092606465ddc9";
    sconfig.ecu_hardware_id = "TCU";
    sconfig.full_client_dir = "/var/sota/ecus/tcu";
    sconfig.ecu_private_key = "sec.private";
    sconfig.ecu_public_key = "sec.public";
    sconfig.target_name_path = "target_name";
    sconfig.metadata_path = "/var/sota/ecus/tcu/metadata";
    auto secondary = std::make_shared<Uptane::VirtualSecondary>(sconfig);
    aktualizr.AddSecondary(secondary);

    aktualizr.Initialize();

    std::vector<Uptane::Target> updates;
    aktualizr.SendDeviceData();
    auto fut_result = aktualizr.CheckUpdates();
    aktualizr.Download(updates);
    aktualizr.Install(updates);
    updates.clear();
    aktualizr.CampaignCheck();
  } catch (const std::exception &ex) {
    LOG_ERROR << "Fatal error in hmi_stub: " << ex.what();
  }

  return 0;
}
