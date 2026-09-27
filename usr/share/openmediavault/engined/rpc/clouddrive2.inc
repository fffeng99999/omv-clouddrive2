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
    /**
     * Cache file storing the latest known upstream version per channel
     * (written by checkUpdate, read by get for the read-only form field).
     */
    const UPDATE_INFO_FILE = "/var/lib/openmediavault/clouddrive2-update-info.json";

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
        $this->registerMethod("checkUpdate");
        $this->registerMethod("update");
    }

    /**
     * Return the settings object enriched with read-only status fields
     * for the web interface (service state, bundled version, port).
     */
    public function get($params, $context)
    {
        $object = \OMV\Rpc\Rpc::call("Config", "get", [
            "id" => "conf.service.clouddrive2"
        ], $context);
        // Read-only information shown in the form (not stored in the
        // configuration database; set() strips these fields again).
        $systemCtl = new \OMV\System\SystemCtl("clouddrive2");
        $running = $systemCtl->isActive();
        // English msgid; translated by the frontend 'translate' filter.
        $object["statusText"] = $running ? "Running" : "Stopped";
        $object["version"] = "";
        if (file_exists("/opt/clouddrive2/VERSION")) {
            $version = @file_get_contents("/opt/clouddrive2/VERSION");
            $object["version"] = (false === $version) ? "" : trim($version);
        }
        $object["port"] = 19798;
        if (!isset($object["channel"]) || "" === $object["channel"]) {
            $object["channel"] = "stable";
        }
        // Read-only update information cached by checkUpdate(). The
        // cache stores the latest version per channel; whether an
        // update is available is computed against the current version
        // so it becomes 'Up to date' automatically after an update.
        $object["updateStatusText"] = "Not checked";
        $object["updateLatest"] = "";
        $cached = @file_get_contents(self::UPDATE_INFO_FILE);
        if (false !== $cached) {
            $info = json_decode($cached, true);
            if (is_array($info)) {
                $latest = $info[$object["channel"]] ?? "";
                if ("" !== $latest) {
                    $object["updateLatest"] = $latest;
                    $object["updateStatusText"] = ($latest !== $object["version"])
                        ? "Update available" : "Up to date";
                }
            }
        }
        return $object;
    }

    public function set($params, $context)
    {
        // Only persist the fields defined in the datamodel; drop the
        // read-only status fields sent back by the form.
        $data = array_intersect_key($params, [
            "enable" => null,
            "configdir" => null,
            "channel" => null
        ]);
        return \OMV\Rpc\Rpc::call("Config", "set", [
            "id" => "conf.service.clouddrive2",
            "data" => $data
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

    /**
     * Check whether a newer CloudDrive2 release is available for the
     * given channel by querying the upstream GitHub releases.
     * @param params 'channel' => 'stable' | 'preview'.
     * @return An array with the fields 'current', 'latest', 'available'
     *   and 'error'.
     */
    public function checkUpdate($params, $context)
    {
        $this->validateMethodContext($context, [
            "role" => OMV_ROLE_ADMINISTRATOR
        ]);
        $this->validateMethodParams($params, "rpc.clouddrive2.checkupdate");
        $channel = $params["channel"];

        $current = "";
        if (file_exists("/opt/clouddrive2/VERSION")) {
            $version = @file_get_contents("/opt/clouddrive2/VERSION");
            $current = (false === $version) ? "" : trim($version);
        }

        $result = [
            "current" => $current,
            "latest" => "",
            "available" => false,
            "error" => ""
        ];

        // Resolve the newest non-draft release of the requested channel.
        // The GitHub API lists releases newest-first; the first matching
        // 'prerelease' flag wins.
        $want = ("preview" === $channel) ? "true" : "false";
        $cmd = new \OMV\System\Process("curl", [
            "-fsSL", "--max-time", "30",
            "https://api.github.com/repos/cloud-fs/cloud-fs.github.io/releases"
        ]);
        $cmd->setRedirect2to1();
        $cmd->setQuiet(true);
        $output = [];
        $exitStatus = 0;
        $cmd->execute($output, $exitStatus);
        if (0 !== $exitStatus) {
            $result["error"] = "Failed to query the GitHub releases API.";
            return $result;
        }
        $tag = "";
        $draft = "";
        foreach ($output as $line) {
            if (1 === preg_match('/"tag_name":\s*"([^"]*)"/', $line, $m)) {
                $tag = $m[1];
            } elseif (1 === preg_match('/"draft":\s*(true|false)/', $line, $m)) {
                $draft = $m[1];
            } elseif (1 === preg_match('/"prerelease":\s*(true|false)/', $line, $m)) {
                if ("true" !== $draft && $m[1] === $want) {
                    break;
                }
                $tag = "";
            }
        }
        if ("" === $tag) {
            $result["error"] = "Could not determine the latest release.";
            return $result;
        }
        $latest = ltrim($tag, "v");
        $result["latest"] = $latest;
        $result["available"] = ($latest !== $current);
        // Cache the result so the form can show it read-only.
        $info = [];
        $cached = @file_get_contents(self::UPDATE_INFO_FILE);
        if (false !== $cached) {
            $decoded = json_decode($cached, true);
            if (is_array($decoded)) {
                $info = $decoded;
            }
        }
        $info[$channel] = $latest;
        @file_put_contents(self::UPDATE_INFO_FILE, json_encode($info));
        return $result;
    }

    /**
     * Download the latest CloudDrive2 release for the given channel,
     * replace the program files and restart the service. Executed as a
     * background task; the progress is shown in a task dialog.
     * @param params 'channel' => 'stable' | 'preview'.
     * @return The name of the background process status file.
     */
    public function update($params, $context)
    {
        $this->validateMethodContext($context, [
            "role" => OMV_ROLE_ADMINISTRATOR
        ]);
        $this->validateMethodParams($params, "rpc.clouddrive2.checkupdate");
        $channel = $params["channel"];

        return $this->execBgProc(function ($bgStatusFilename, $bgOutputFilename) use ($channel) {
            $cmd = new \OMV\System\Process("/usr/sbin/omv-clouddrive2-update", [$channel]);
            $cmd->setRedirect2to1();
            if (0 !== ($exitStatus = $this->exec(
                $cmd,
                $output,
                $bgOutputFilename
            ))) {
                throw new \OMV\ExecException($cmd, $output, $exitStatus);
            }
            return $output;
        });
    }
}
