# This file is part of OpenMediaVault.
#
# @license   https://www.gnu.org/licenses/gpl.html GPL Version 3
# @author    fffeng99999
#
# OpenMediaVault is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# any later version.
#
# OpenMediaVault is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# Documentation/Howto:
# https://www.clouddrive2.com/en/help.html
# https://www.clouddrive2.com/en/docker.html
#
# The CloudDrive2 core binary is installed by the deb package at
# /opt/clouddrive2/clouddrive and ships its web console at
# /opt/clouddrive2/wwwroot (the binary loads 'wwwroot' relative to its
# working directory). The configuration/data directory is selected via
# the CLOUDDRIVE_HOME environment variable.

{% set config = salt['omv_conf.get']('conf.service.clouddrive2') %}

{% if config.enable | to_bool %}

create_clouddrive2_data_directory:
  file.directory:
    - name: "{{ config.configdir }}"
    - user: root
    - group: root
    - mode: '0755'
    - makedirs: True

create_clouddrive2_systemd_unit_file:
  file.managed:
    - name: "/etc/systemd/system/clouddrive2.service"
    - source:
      - salt://{{ tpldir }}/files/clouddrive2.service.j2
    - template: jinja
    - context:
        config: {{ config | json }}
    - user: root
    - group: root
    - mode: '0644'

clouddrive2_systemctl_daemon_reload:
  module.run:
    - service.systemctl_reload:

start_clouddrive2_service:
  service.running:
    - name: clouddrive2
    - enable: True
    - watch:
      - file: create_clouddrive2_systemd_unit_file

{% else %}

stop_clouddrive2_service:
  service.dead:
    - name: clouddrive2
    - enable: False

remove_clouddrive2_systemd_unit_file:
  file.absent:
    - name: "/etc/systemd/system/clouddrive2.service"

clouddrive2_systemctl_daemon_reload:
  module.run:
    - service.systemctl_reload:

{% endif %}
