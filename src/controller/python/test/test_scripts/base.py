from chip import ChipDeviceCtrl
import threading
import os
import sys
import logging
import time

logger = logging.getLogger('PythonMatterControllerTEST')
logger.setLevel(logging.INFO)

sh = logging.StreamHandler()
sh.setFormatter(
    logging.Formatter(
        '%(asctime)s [%(name)s] %(levelname)s %(message)s'))
sh.setStream(sys.stdout)
logger.addHandler(sh)


def TestFail(message):
    logger.fatal("Testfail: {}".format(message))
    os._exit(1)


def FailIfNot(cond, message):
    if not cond:
        TestFail(message)


class TestTimeout(threading.Thread):
    def __init__(self, timeout: int):
        threading.Thread.__init__(self)
        self._timeout = timeout
        self._should_stop = False
        self._cv = threading.Condition()

    def stop(self):
        with self._cv:
            self._should_stop = True
            self._cv.notify_all()
        self.join()

    def run(self):
        stop_time = time.time() + self._timeout
        logger.info("Test timeout set to {} seconds".format(self._timeout))
        with self._cv:
            wait_time = stop_time - time.time()
            while wait_time > 0 and not self._should_stop:
                self._cv.wait(wait_time)
                wait_time = stop_time - time.time()
        if time.time() > stop_time:
            TestFail("Timeout")


class BaseTestHelper:
    def __init__(self, nodeid: int):
        self.devCtrl = ChipDeviceCtrl.ChipDeviceController(
            controllerNodeId=nodeid)
        self.logger = logger

    def TestKeyExchange(self, ip: str, setuppin: int, nodeid: int):
        self.logger.info("Conducting key exchange with device {}".format(ip))
        if not self.devCtrl.ConnectIP(ip.encode("utf-8"), setuppin, nodeid):
            self.logger.info(
                "Failed to finish key exchange with device {}".format(ip))
            return False
        self.logger.info("Device finished key exchange.")
        return True

    def TestCloseSession(self, nodeid: int):
        self.logger.info(f"Closing sessions with device {nodeid}")
        try:
            self.devCtrl.CloseSession(nodeid)
            return True
        except Exception as ex:
            self.logger.exception(f"Failed to close sessions with device {nodeid}: {ex}")
            return False

    def TestNetworkCommissioning(self, nodeid: int, endpoint: int, group: int, dataset: str, network_id: str):
        self.logger.info("Commissioning network to device {}".format(nodeid))
        try:
            self.devCtrl.ZCLSend("NetworkCommissioning", "AddThreadNetwork", nodeid, endpoint, group, {
                "operationalDataset": bytes.fromhex(dataset),
                "breadcrumb": 0,
                "timeoutMs": 1000}, blocking=True)
        except Exception as ex:
            self.logger.exception("Failed to send AddThreadNetwork command")
            return False
        self.logger.info(
            "Send EnableNetwork command to device {}".format(nodeid))
        try:
            self.devCtrl.ZCLSend("NetworkCommissioning", "EnableNetwork", nodeid, endpoint, group, {
                "networkID": bytes.fromhex(network_id),
                "breadcrumb": 0,
                "timeoutMs": 1000}, blocking=True)
        except Exception as ex:
            self.logger.exception("Failed to send EnableNetwork command")
            return False
        return True

    def TestOnOffCluster(self, nodeid: int, endpoint: int, group: int):
        self.logger.info(
            "Sending On/Off commands to device {} endpoint {}".format(nodeid, endpoint))
        err, resp = self.devCtrl.ZCLSend("OnOff", "On", nodeid,
                                         endpoint, group, {}, blocking=True)
        if err != 0 or resp is None or resp.ProtocolCode != 0:
            self.logger.error(
                "failed to send OnOff.On: error is {} with im response{}".format(err, resp))
            return False
        err, resp = self.devCtrl.ZCLSend("OnOff", "Off", nodeid,
                                         endpoint, group, {}, blocking=True)
        if err != 0 or resp is None or resp.ProtocolCode != 0:
            self.logger.error(
                "failed to send OnOff.Off: error is {} with im response {}".format(err, resp))
            return False
        return True

    def TestResolve(self, fabricid, nodeid):
        self.logger.info(
            "Resolve {} with fabric id: {}".format(nodeid, fabricid))
        try:
            self.devCtrl.ResolveNode(fabricid=fabricid, nodeid=nodeid)
        except Exception as ex:
            self.logger.exception("Failed to resolve. {}".format(ex))

    def TestReadBasicAttribiutes(self, nodeid: int, endpoint: int, group: int):
        basic_cluster_attrs = {
            "VendorName": "TEST_VENDOR",
            "VendorID": 9050,
            "ProductName": "TEST_PRODUCT",
            "ProductID": 65279,
            "UserLabel": "",
            "Location": "",
            "HardwareVersion": 0,
            "HardwareVersionString": "TEST_VERSION",
            "SoftwareVersion": 0,
            "SoftwareVersionString": "prerelease",
        }
        failed_zcl = {}
        for basic_attr, expected_value in basic_cluster_attrs.items():
            try:
                res = self.devCtrl.ZCLReadAttribute(cluster="Basic",
                                                    attribute=basic_attr,
                                                    nodeid=nodeid,
                                                    endpoint=endpoint,
                                                    groupid=group)
                if res is None:
                    raise Exception("Read {} attribute: no value get".format(basic_attr))
                elif res.status != 0:
                    raise Exception(
                        "Read {} attribute: non-zero status code {}".format(basic_attr, res.status))
                elif res.value != expected_value:
                    raise Exception("Read {} attribute: expect {} got {}".format(
                        basic_attr, repr(expected_value), repr(res.value)))
                time.sleep(2)
            except Exception as ex:
                failed_zcl[basic_attr] = str(ex)
        if failed_zcl:
            self.logger.exception(f"Following attributes failed: {failed_zcl}")
            return False
        return True

    def TestWriteBasicAttributes(self, nodeid: int, endpoint: int, group: int):
        basic_cluster_attrs = [
            ("UserLabel", "Test"),
        ]
        failed_zcl = []
        for basic_attr in basic_cluster_attrs:
            try:
                self.devCtrl.ZCLWriteAttribute(cluster="Basic",
                                               attribute=basic_attr[0],
                                               nodeid=nodeid,
                                               endpoint=endpoint,
                                               groupid=group,
                                               value=basic_attr[1])
                time.sleep(2)
            except Exception:
                failed_zcl.append(basic_attr)
        if failed_zcl:
            self.logger.exception(f"Following attributes failed: {failed_zcl}")
            return False
        return True
