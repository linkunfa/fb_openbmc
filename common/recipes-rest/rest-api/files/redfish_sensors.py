import typing as t
from functools import lru_cache

import aggregate_sensor
import pal
import redfish_chassis_helper
from aiohttp import web
from common_utils import dumps_bytestr
from redfish_base import validate_keys

try:
    fru_name_map = pal.pal_fru_name_map()
except Exception:
    fru_name_map = {}


# controller for /redfish/v1/Chassis/{fru}/Sensors
async def get_redfish_sensors_handler(request):
    expand_members = "$expand" in request.query_string
    fru_name = request.match_info["fru_name"]
    fru_names = _get_fru_names(fru_name)
    if len(fru_names) == 0:
        return web.json_response(
            {"Error": "Invalid FRU {fru_name}".format(fru_name=fru_name)},
            dumps=dumps_bytestr,
            status=400,
        )
    members_json = await _get_sensor_members(
        parent_resource=fru_name, fru_names=fru_names, expand=expand_members
    )
    body = {
        "@odata.type": "#SensorCollection.SensorCollection",
        "Name": "Chassis sensors",
        "Members@odata.count": len(members_json),
        "Members": members_json,
        "@odata.id": "/redfish/v1/Chassis/{fru_name}/Sensors".format(fru_name=fru_name),
    }
    await validate_keys(body)
    return web.json_response(body, dumps=dumps_bytestr)


# controller for /redfish/v1/Chassis/{fru}/Sensors/{fru_name}_{sensor_id}
async def get_redfish_sensor_handler(request):
    fru_name = request.match_info["fru_name"]
    if redfish_chassis_helper.is_libpal_supported():
        sensor_id_and_fru = request.match_info["sensor_id"]
        target_fru_name = sensor_id_and_fru.split("_")[0]
        sensor_id = int(sensor_id_and_fru.split("_")[1])
    else:
        target_fru_name = "BMC"
        sensor_id = request.match_info["sensor_id"]
    sensor = await _get_sensor(fru_name=target_fru_name, sensor_id=sensor_id)
    if sensor:
        body = _render_sensor_body(fru_name, sensor)
    else:
        return web.json_response(
            {
                "Error": "sensor not found fru_name={fru_name}, sensor_id={sensor_id}".format(  # noqa: B950
                    fru_name=fru_name, sensor_id=sensor_id
                )
            },
            dumps=dumps_bytestr,
            status=404,
        )
    await validate_keys(body)
    return web.json_response(body, dumps=dumps_bytestr)


async def _get_sensor_members(
    parent_resource: str, fru_names: t.List[str], expand: bool
):
    members_json = []
    for fru in fru_names:
        if redfish_chassis_helper.is_libpal_supported():
            fru_id = fru_name_map[fru]
            for sensor_id in pal.pal_get_fru_sensor_list(fru_id):
                sensor = await _get_sensor(fru_name=fru, sensor_id=sensor_id)
                if sensor:
                    child = _render_sensor(parent_resource, sensor, expand)
                    members_json.append(child)
        else:
            sensors = redfish_chassis_helper.get_older_fboss_sensor_details(fru)
            for sensor in sensors:
                child = _render_sensor(parent_resource, sensor, expand)
                members_json.append(child)
        if parent_resource == "1":
            # we expose aggregate sensors under the Chassis, so only do this
            # if parent_resource is 1
            for sensor_id in range(aggregate_sensor.aggregate_sensor_count()):
                sensor = _get_sensor("aggregate", sensor_id)
                if sensor:
                    child = _render_sensor(parent_resource, sensor, expand)
                    members_json.append(child)
    return members_json


async def _get_sensor(
    fru_name: str, sensor_id: t.Union[int, str]
) -> t.Optional[redfish_chassis_helper.SensorDetails]:
    if fru_name == "aggregate":
        sensor = redfish_chassis_helper.get_aggregate_sensor(sensor_id)
    else:
        if redfish_chassis_helper.is_libpal_supported():
            fru_id = fru_name_map[fru_name]
            sensor = redfish_chassis_helper.get_pal_sensor(fru_name, fru_id, sensor_id)
            return sensor
        else:
            # i know this is ugly,
            # sensors.py does not support accessing individual sensors by id.
            sensors = redfish_chassis_helper.get_older_fboss_sensor_details(fru_name)
            for candidate in sensors:
                if (
                    candidate.sensor_name.replace("/", "_").replace(" ", "")
                    == sensor_id
                ):
                    return candidate
            return None


def _render_sensor(
    parent_resource: str, sensor: redfish_chassis_helper.SensorDetails, expand: bool
) -> t.Dict[str, t.Any]:
    if expand:
        child = _render_sensor_body(parent_resource, sensor)
    else:
        child = {"@odata.id": _render_sensor_uri(parent_resource, sensor)}
    return child


def _render_sensor_body(
    parent_resource: str, sensor: redfish_chassis_helper.SensorDetails
) -> t.Optional[t.Dict[str, t.Any]]:
    if sensor.reading == redfish_chassis_helper.SAD_SENSOR:
        status_val = {"State": "UnavailableOffline", "Health": "Critical"}
    else:
        status_val = {"State": "Enabled", "Health": "OK"}
    if sensor.sensor_thresh:
        threshold_json = {
            "UpperCaution": int(sensor.sensor_thresh.unc_thresh),
            "UpperCritical": int(sensor.sensor_thresh.ucr_thresh),
            "UpperFatal": int(sensor.sensor_thresh.unr_thresh),
            "LowerCaution": int(sensor.sensor_thresh.lnc_thresh),
            "LowerCritical": int(sensor.sensor_thresh.lcr_thresh),
            "LowerFatal": int(sensor.sensor_thresh.lnr_thresh),
        }
    else:
        threshold_json = {}

    body = {
        "@odata.type": "#Sensor.v1_2_0.Sensor",
        "Id": str(sensor.sensor_number),
        "Name": sensor.sensor_name,
        "Oem": {},
        "PhysicalContext": _get_phy_context(sensor.sensor_name),
        "Status": status_val,
        "Reading": sensor.reading,
        "ReadingUnits": sensor.sensor_unit,
        "ReadingRangeMin": -99999,
        "ReadingRangeMax": 99999,
        "Thresholds": threshold_json,
        "@odata.id": _render_sensor_uri(parent_resource, sensor),
    }
    return body


def _render_sensor_uri(
    parent_resource: str, sensor: redfish_chassis_helper.SensorDetails
) -> str:
    if redfish_chassis_helper.is_libpal_supported():
        return "/redfish/v1/Chassis/{parent_resource}/Sensors/{fru_name}_{sensor_id}".format(  # noqa: B950
            parent_resource=parent_resource,
            fru_name=sensor.fru_name,
            sensor_id=sensor.sensor_number,
        )
    else:
        return "/redfish/v1/Chassis/{parent_resource}/Sensors/{sensor_name}".format(
            parent_resource=parent_resource,
            sensor_name=sensor.sensor_name.replace("/", "_").replace(" ", ""),
        )


@lru_cache()
def _get_phy_context(sensor_name: str) -> str:
    # valid physical contexts:
    # ['Room', 'Intake', 'Exhaust', 'LiquidInlet', 'LiquidOutlet', 'Front', 'Back', # noqa: B950
    #'Upper', 'Lower', 'CPU', 'CPUSubsystem', 'GPU', 'GPUSubsystem', 'FPGA', 'Accelerator', # noqa: B950
    # 'ASIC', 'Backplane', 'SystemBoard', 'PowerSupply', 'PowerSubsystem', 'VoltageRegulator', # noqa: B950
    # 'Rectifier', 'StorageDevice', 'NetworkingDevice', 'ComputeBay', 'StorageBay', 'NetworkBay', # noqa: B950
    # 'ExpansionBay', 'PowerSupplyBay', 'Memory', 'MemorySubsystem', 'Chassis', 'Fan', # noqa: B950
    # 'CoolingSubsystem', 'Motor', 'Transformer', 'ACUtilityInput', 'ACStaticBypassInput', # noqa: B950
    # 'ACMaintenanceBypassInput', 'DCBus', 'ACOutput', 'ACInput', 'TrustedModule', 'Board', 'Transceiver'], # noqa: B950
    if "INLET" in sensor_name:
        return "Intake"
    if "OUTLET" in sensor_name:
        return "Exhaust"
    if "FAN" in sensor_name:
        return "Fan"
    if "DIMM" in sensor_name:
        return "Memory"
    if "SOC" in sensor_name:
        return "CPU"
    if "NVMe" in sensor_name:
        return "StorageDevice"
    return "Chassis"


@lru_cache()
def _get_fru_names(fru_name: str) -> t.List[str]:
    if fru_name == "1":  # Chassis/1 represents the Chassis on all platforms
        if redfish_chassis_helper.is_libpal_supported():
            return redfish_chassis_helper.get_single_sled_frus()
        else:
            return ["BMC"]
    elif "server" in fru_name:
        fru_names = [
            fru_name.replace("server", "slot")
        ]  # we expose slot1-4 as server 1-4 in our routes.
    else:
        fru_names = []
    return fru_names
