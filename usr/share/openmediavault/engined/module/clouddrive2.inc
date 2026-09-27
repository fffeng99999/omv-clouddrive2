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

namespace Engined\Module;

/**
 * CloudDrive2 engine module.
 *
 * Marks the module dirty whenever the configuration changes so that
 * 'Apply changes' re-runs the Salt state, and reports the service
 * status to the global Services monitoring (dashboard).
 */
class CloudDrive2 extends \OMV\Engine\Module\ServiceAbstract implements \OMV\Engine\Notify\IListener, \OMV\Engine\Module\IServiceStatus
{
    public function getName()
    {
        return "clouddrive2";
    }

    public function getStatus()
    {
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get("conf.service.clouddrive2");
        $systemCtl = new \OMV\System\SystemCtl("clouddrive2");
        return [
            "name" => $this->getName(),
            "title" => gettext("CloudDrive2"),
            "enabled" => $object->get("enable"),
            "running" => $systemCtl->isActive()
        ];
    }

    public function bindListeners(\OMV\Engine\Notify\Dispatcher $dispatcher)
    {
        $dispatcher->addListener(
            OMV_NOTIFY_MODIFY,
            "org.openmediavault.conf.service.clouddrive2",
            [$this, "setDirty"]
        );
    }
}
