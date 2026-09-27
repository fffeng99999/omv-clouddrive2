<?php

/**
 * This file is part of OpenMediaVault.
 *
 * @license   https://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    fffeng99999
 *
 * OpenMediaVault is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * OpenMediaVault is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

namespace Engined\Rpc;

/**
 * CloudDrive2 RPC service.
 *
 * The settings object (conf.service.clouddrive2) is stored in the OMV
 * configuration database and deployed by the Salt state
 * /srv/salt/omv/deploy/clouddrive2 which generates the systemd unit.
 * The 'get'/'set' methods simply forward to the core 'Config' service
 * (same pattern as the openmediavault-filebrowser plugin). The
 * start/stop/restart methods provide immediate control of the service
 * (same pattern as the k3s/podman plugins) in addition to the
 * declarative enable + 'Apply changes' workflow.
 */
class OMVRpcServiceCloudDrive2 extends \OMV\Rpc\ServiceAbstract
{
    public function getName()
    {
        return "CloudDrive2";
    }

    public function initialize()
    {
        $this->registerMethod("get");
        $this->registerMethod("set");
        $this->registerMethod("start");
        $this->registerMethod("stop");
        $this->registerMethod("restart");
        $this->registerMethod("getVersion");
    }

    public function get($params, $context)
    {
        return \OMV\Rpc\Rpc::call("Config", "get", [
            "id" => "conf.service.clouddrive2"
        ], $context);
    }

    public function set($params, $context)
    {
        return \OMV\Rpc\Rpc::call("Config", "set", [
            "id" => "conf.service.clouddrive2",
            "data" => $params
        ], $context);
    }

    /**
     * Start the CloudDrive2 service immediately.
     */
    public function start($params, $context)
    {
        $this->validateMethodContext($context, [
            "role" => OMV_ROLE_ADMINISTRATOR
        ]);
        $systemCtl = new \OMV\System\SystemCtl("clouddrive2");
        $systemCtl->start();
    }

    /**
     * Stop the CloudDrive2 service immediately.
     */
    public function stop($params, $context)
    {
        $this->validateMethodContext($context, [
            "role" => OMV_ROLE_ADMINISTRATOR
        ]);
        $systemCtl = new \OMV\System\SystemCtl("clouddrive2");
        $systemCtl->stop();
    }

    /**
     * Restart the CloudDrive2 service immediately.
     */
    public function restart($params, $context)
    {
        $this->validateMethodContext($context, [
            "role" => OMV_ROLE_ADMINISTRATOR
        ]);
        $systemCtl = new \OMV\System\SystemCtl("clouddrive2");
        $systemCtl->restart();
    }

    /**
     * Return the installed CloudDrive2 version. The version is recorded
     * at package build time in /opt/clouddrive2/VERSION (the upstream
     * binary is not executed to avoid accidentally starting the service).
     */
    public function getVersion($params, $context)
    {
        $this->validateMethodContext($context, [
            "role" => OMV_ROLE_ADMINISTRATOR
        ]);
        $result = [
            "installed" => false,
            "version" => ""
        ];
        if (!file_exists("/opt/clouddrive2/clouddrive")) {
            return $result;
        }
        $result["installed"] = true;
        if (file_exists("/opt/clouddrive2/VERSION")) {
            $version = @file_get_contents("/opt/clouddrive2/VERSION");
            $result["version"] = (false === $version) ? "" : trim($version);
        }
        return $result;
    }
}
