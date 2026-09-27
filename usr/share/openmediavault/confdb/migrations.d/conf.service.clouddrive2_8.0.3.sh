#!/usr/bin/env dash
#
# This file is part of OpenMediaVault.
#
# @license   https://www.gnu.org/licenses/gpl.html GPL Version 3
# @author    fffeng99999
#
# OpenMediaVault is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# any later version.

set -e

. /usr/share/openmediavault/scripts/helper-functions

########################################################################
# Add the 'channel' key (stable|preview) to an existing configuration.
# omv_config_add_key is a no-op if the key already exists.
########################################################################
omv_config_add_key "/config/services/clouddrive2" "channel" "stable"

exit 0
