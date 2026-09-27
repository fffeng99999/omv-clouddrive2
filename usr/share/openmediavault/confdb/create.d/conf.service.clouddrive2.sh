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
# Update the configuration.
# <config>
#   <services>
#     <clouddrive2>
#       <enable>0|1</enable>
#       <configdir>/var/lib/clouddrive2</configdir>
#     </clouddrive2>
#   </services>
# </config>
########################################################################
if ! omv_config_exists "/config/services/clouddrive2"; then
	omv_config_add_node "/config/services" "clouddrive2"
	omv_config_add_key "/config/services/clouddrive2" "enable" "0"
	omv_config_add_key "/config/services/clouddrive2" "configdir" "/var/lib/clouddrive2"
fi

exit 0
