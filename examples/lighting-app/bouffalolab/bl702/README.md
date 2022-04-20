# CHIP BL702 Lighting App Example

## Build

### Prerequisite
- Clone connectedhomeip github repo and update all submdoule;
- Install all tools (likely already presetn for CHIP developers).

### Supported Hardware
- BL702 with At lease 2MB flash and PSRAM
### Build CHIP BL702 Ligiting App example
- Export toolchain for BL702
  
    For Linux Developer
    ```shell
    $ export PATH=$PATH:<connectedhomeip_repo_path>/third_party/bouffalolab/bl_iot_sdk/repo/toolchain/riscv/Linux/bin
    ```
    For Mac OSX developer
    ```shell
    $ export PATH=$PATH:<connectedhomeip_repo_path>/third_party/bouffalolab/bl_iot_sdk/repo/toolchain/riscv/Darwin/bin
    ```
    where `<connectedhomeip_repo_path>` is the path to connectedhomeip repo.

- Use GN/Ninja directly

    Under connectedhomeip repo folder, 
    ```shell
    $ source scripts/activate.sh
    ```
- Compile Lighting App

    Under connectedhomeip repo folder, 
    ```shell
    $ ./scripts/examples/gn_bl702_example.sh lighting-app
    ```
    Output file `chip-bl702-lighting-example.bin` is located under <connectedhomeip_repo_path>examples/lighting-app/bouffalolab/bl702/out/debug folder.

- Flash with BLDevCube_path

    Start BLDevCube_path 
    - Select default `Factory params` uder BLDevCube_path Software path;
    - Select Parition Table `<connectedhomeip_repo_path>/examples/platform/bouffalolab/bl702/partitions/partition_cfg_2M_psm.toml`;
    - Select Firmware Bin chip-bl702-lighting-example.bin;
    - Selected Chip Erase if need;
    - Choice Target COM port.
    - Then click Create & Download.

    **NOTE**
    > With Flash operation done, OTA bin file `FW_OTA.bin.xz.hash` with xz compression and hash verification is also generated. <br>
    > For Linux developer, `FW_OTA.bin.xz.hash` is under folder <BLDevCube_path>/chips/bl702/ota; <br>
    > For Mac OSX developer, `FW_OTA.bin.xz.hash` is under folder <BLDevCube_path>/Contents/MacOS/chips/bl702/ota; <br>
    > where `BLDevCube_path` is DevCube installation path. <br>

- Firmware Behavior
  
    Status LED: TX0<br>
    Lighting LED: RX1<br>
    Factory Reset: Short `IO11` to `GND` over 3 seconds<br>

## Test with chip-tool

### Setup Border Router based on Rasp PI
### Clone connectedhomeip & update all submodules on Border Router Rasp PI
### Build chip-tool as [guide](../../../chip-tool/README.md)
### Commissioning
- Power on BL702 with chip lighting app
- BLE commissioning with chip-tool
    ```shell
    ./chip-tool pairing ble-thread <node_id> hex:<thread_operational_dataset> 20202021 3840
    ```
    `node_id` is matter node id, such as 10; `<thread_operational_dataset>` is Border Router Dataset, which to get with command `sudo ot-ctl dataset active -x` on Rasp PI border router.

### Toggle Lighting LED
- After BLE commissioning gets successfully, 
    ```
    $ ./chip-tool onoff toggle <node_id> 1
    ```

### Identify Status LED
- After BLE commissioning gets successfully, 
    ```shell
    ./chip-tool identify identify <identify_duration> <node_id> 1
    ```

which `<identify_duration>` is how many seconds to execute identify command.

## OTA software upgrade with ota-provider-app

### Build ota-provider-app as [guide](../../../ota-provider-app/linux/README.md)

### Create the Matter OTA with Bouffalab OTA bin `FW_OTA.bin.xz.hash`
- Under connectedhomeip repo path
    ```shell
    $ ./src/app/ota_image_tool.py create -v 0xFFF1 -p 0x8005 -vn 1 -vs "1.0" -da sha256 <FW_OTA.bin.xz.hash> lighting-app.ota

    ```
- lighting-app.ota should have greater software version which is defined by macro CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION in CHIPProjectConfig.h

### Start ota-provider-app
- Start ota-provider-app for lighting-app.ota
    ```shell
    $ rm -r /tmp/chip_*
    $ ./chip-ota-provider-app -f <path_to_ota_bin>/lighting-app.ota
    ```
    where `<path_to_ota_bin>` is the folder for lighting-app.ota.
- Provision ota-provider-app with assigned node id to 1
    ```shell
    $ ./chip-tool pairing onnetwork 1 20202021
    $ ./out/chip-tool accesscontrol write acl '[{"fabricIndex": 1, "privilege": 5, "authMode": 2, "subjects": [112233], "targets": null}, {"fabricIndex": 1, "privilege": 3, "authMode": 2, "subjects": null, "targets": null}]' 1 0
    ```

### Start ota software upgrade
- BLE commission BL702 lighting if not commissioned.
- Start OTA software upgrade prcess
    ```shell
    ./chip-tool otasoftwareupdaterequestor announce-ota-provider 1 0 0 0 <node_id_to_lighting_app> 0
    ```
    where `<node_id_to_lighting_app>` is node id of BL702 lighting app.
- After OTA software upgrade gets done, BL702 will get reboot automatically.
