/*  Copyright 2014 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.maidsafe.net/licenses

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#include "maidsafe/vault_manager/tools/commands/begin.h"

#include "maidsafe/common/config.h"
#include "maidsafe/common/log.h"
#include "maidsafe/common/make_unique.h"

#include "maidsafe/vault_manager/tools/local_network_controller.h"
#include "maidsafe/vault_manager/tools/commands/choose_vault_manager_port.h"
#include "maidsafe/vault_manager/tools/commands/choose_test_root_dir.h"
#include "maidsafe/vault_manager/tools/commands/choose_path_to_bootstrap.h"

namespace maidsafe {

namespace vault_manager {

namespace tools {

Begin::Begin(LocalNetworkController* local_network_controller)
    : Command(local_network_controller, "Initial options.",
              "\nPlease choose from the following options ('" + kQuitCommand_ + "' to quit):\n\n"
              "  1. Start a new network on this machine.\n"
              "  2. Connect to an existing VaultManager on this machine.\n"
              "  3. Connect to an existing Network.\n" + kPrompt_,
              "MaidSafe Local Network Controller " + kApplicationVersion() + ": Main Options"),
      choice_(0) {}

void Begin::GetChoice() {
  TLOG(kDefaultColour) << kInstructions_;
  while (!DoGetChoice(choice_, static_cast<int*>(nullptr), 1, 3))
    TLOG(kDefaultColour) << '\n' << kInstructions_;
}

void Begin::HandleChoice() {
  if (choice_ == 1) {
    local_network_controller_->new_network = true;
    local_network_controller_->current_command =
        maidsafe::make_unique<ChooseTestRootDir>(local_network_controller_);
  } else if (choice_ == 2) {
    local_network_controller_->current_command =
        maidsafe::make_unique<ChooseVaultManagerPort>(local_network_controller_, true);
  } else if (choice_ == 3) {
    local_network_controller_->current_command =
        maidsafe::make_unique<ChoosePathToBootstrap>(local_network_controller_);
  } else {
    assert(false);  // TODO(Team) Implement other options
  }


  TLOG(kDefaultColour) << kSeparator_;
}

}  // namespace tools

}  // namespace vault_manager

}  // namespace maidsafe
