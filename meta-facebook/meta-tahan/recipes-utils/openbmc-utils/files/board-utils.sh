#!/bin/bash
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA

#shellcheck disable=SC1091
# Do not change this line to openbmc-utils.sh, or it will generate a source loop.
. /usr/local/bin/i2c-utils.sh

PWRCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 12-0060)
SCMCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 1-0035)

BOARD_ID="${PWRCPLD_SYSFS_DIR}/board_id"
VERSION_ID="${PWRCPLD_SYSFS_DIR}/version_id"
SMB1_DC_STATUS="${PWRCPLD_SYSFS_DIR}/smb1_dc_status"
SMB2_DC_STATUS="${PWRCPLD_SYSFS_DIR}/smb2_dc_status"

COME_POWER_EN="${SCMCPLD_SYSFS_DIR}/pwr_come_en"
COME_POWER_OFF="${SCMCPLD_SYSFS_DIR}/pwr_force_off"
COME_POWER_CYCLE_N="${SCMCPLD_SYSFS_DIR}/pwr_cyc_n"
XP5R0V_COME_PG="${SCMCPLD_SYSFS_DIR}/xp5r0v_come_pg"
XP12R0V_COME_PG="${SCMCPLD_SYSFS_DIR}/xp12r0v_come_pg"
CHASSIS_POWER_CYCLE="${PWRCPLD_SYSFS_DIR}/power_cycle_go"

wedge_board_type() {
    echo 'tahan'
}

wedge_board_rev() {
    board_id=$(head -n 1 < "$BOARD_ID" 2> /dev/null)
    version_id=$(head -n 1 < "$VERSION_ID" 2> /dev/null)

    case "$((board_id))" in
        12)
            echo "Board type: Tahan Switching System"
            ;;
        *)
            echo "Board type: unknown value [$board_id]"
            ;;
    esac

    case "$((version_id))" in
        0)
            echo "Revision: EVT-1"
            ;;
        1)
            echo "Revision: EVT-2"
            ;;
        2)
            echo "Revision: EVT-3"
            ;;
        3)
            echo "Revision: DVT-1"
            ;;
        4)
            echo "Revision: DVT-2"
            ;;
        5)
            echo "Revision: PVT"
            ;;
        6)
            echo "Revision: MP"
            ;;
        *) 
            echo "Revision: unknown value [$version_id]"
            ;;
    esac

    return 0
}

userver_power_is_on() {
    if [ ! -e "$XP5R0V_COME_PG" ] || [ ! -e "$XP12R0V_COME_PG" ]; then
        echo "Error: $XP5R0V_COME_PG or $XP12R0V_COME_PG does not exist! Is scbcpld ready??"
        echo "Assuming uServer is off!"
        return 1
    fi

    xp5r0v_sts=$(head -n 1 "$XP5R0V_COME_PG" 2> /dev/null)
    xp12r0v_sts=$(head -n 1 "$XP12R0V_COME_PG" 2> /dev/null)

    if [ $((xp5r0v_sts)) -eq $((0x1)) ] &&
       [ $((xp12r0v_sts)) -eq $((0x1)) ] ; then
        return 0
    fi

    return 1
}

userver_power_on() {
    if ! sysfs_write "$COME_POWER_OFF" 1; then
        return 1
    fi
    if ! sysfs_write "$COME_POWER_EN" 1; then
        return 1
    fi
    return 0
}

userver_power_off() {
    if ! sysfs_write "$COME_POWER_OFF" 0; then
        return 1
    fi
}

userver_reset() {
    if ! sysfs_write "$COME_POWER_CYCLE_N" 0; then
        return 1
    fi

    # $COME_POWER_CYCLE_N will be auto-set to 1 when power cycle completes.
    timeout=10 # 10 seconds
    while [ $((timeout)) -ge 0 ]; do
        val=$(head -n 1 "$COME_POWER_CYCLE_N" 2> /dev/null)
        if [ $((val)) -eq 1 ]; then
            return 0
        fi

        sleep 1
        timeout=$((timeout-1))
    done

    echo "failed to reset userver!"
    logger -p user.crit "failed to reset userver!"
    return 1
}

chassis_power_cycle() {
    if ! sysfs_write "$CHASSIS_POWER_CYCLE" 1; then
        return 1
    fi

    return 0
}

bmc_mac_addr() {
    # Fetch mac addr supporting v4 and v5 format.
    bmc_mac=$(weutil | sed -nE 's/((Local MAC)|(BMC MAC Base)): (.*)/\4/p')
    if [ -z "$bmc_mac" ]; then
        echo "BMC MAC Address Not Found !" 1>&2
        logger -p user.crit "BMC MAC Address Not Found !"
        return 1
    else
        echo "$bmc_mac"
    fi
}

userver_mac_addr() {
    mac_addr_string=$(weutil | grep -E "(X86 CPU MAC Base)|(Local MAC)")
    [[ "$mac_addr_string" =~ (((X86 CPU MAC Base)|(Local MAC)):[[:space:]]*(.*)) ]];
    mac_addr_type="${BASH_REMATCH[2]}"
    mac_addr_value="${BASH_REMATCH[5]}"
    case "$mac_addr_type" in
        # Fetch cpu mac addr based on EEPROM v5.
        "X86 CPU MAC Base")
            echo "$mac_addr_value"
            ;;
        "Local MAC")
            #
            # Fetch cpu mac addr based on EEPROM v4 and older.
            # SCM EEPROM doesn't exist in Tahan, and CPU MAC address is obtained by
            # adding 1 to the BMC MAC address.
            #
            cpu_mac=$(mac_addr_inc "$mac_addr_value")
            echo "$cpu_mac"
            ;;
        *)
            echo "CPU MAC Address Not Found !" 1>&2
            logger -p user.crit "CPU MAC Address Not Found !"
            return 1
            ;;
    esac
}

leakage_detection_status_check() {
    smb1_dc_sts=$(head -n 1 "$SMB1_DC_STATUS" 2> /dev/null)
    smb2_dc_sts=$(head -n 1 "$SMB2_DC_STATUS" 2> /dev/null)
    if [ $((smb1_dc_sts)) -eq $((0x1)) ] &&
       [ $((smb2_dc_sts)) -eq $((0x1)) ] ; then
        return 1
    fi

    return 0
}

user_issued_shutdown_status_check(){
    come_pwr_ctrl_sts=$(head -n 1 "$COME_POWER_OFF" 2> /dev/null)
    if [ $((come_pwr_ctrl_sts)) -eq $((0x1)) ]; then
        return 1
    fi

    return 0
}

