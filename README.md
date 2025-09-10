# nPM PowerUP
## Progression Path
```mermaid
graph LR;
  main-->adc;
  adc-->npm_powerup*;
  npm_powerup*-->ipc;
  ipc-->ble;
```
> `*` == your current location

## Preface
Now, we will work on using nPM PowerUP to control the regulators on the nPM2100-EK.

## Step 1
Unbox your nPM2100 EK, and connect it to your PC with your extra USB-C cable.

![image](https://github.com/user-attachments/assets/ac3b1409-de8e-4899-b2cb-9ac7c6adfd16)


## Step 3
Insert your provided battery into its corresponding battery holder, and insert that into the BATTERY INPUT connector on the EK

![image](https://github.com/user-attachments/assets/2869d224-5a36-46c8-89d8-eb26258c1507)

## Step 4
- Using your female-female jumper wires, connect the following:
  - `Port P1 Pin 11` of the nRF54L15 DK to the `VOUT` header on the nPM2100 EK
  - `Port P1 Pin 12` of the nRF54L15 DK to the `LS/LDO OUT` header on the nPM2100 EK
  - and tie the GNDs of the kits together.

![image](https://github.com/user-attachments/assets/d7cd910a-1cfc-484e-952c-9f640de54315)


## Step 5
- Open nRF Connect for Desktop and open nPM PowerUP

![image](https://github.com/user-attachments/assets/22bb55af-fb62-4c20-aa0d-d5ab089d637e)

## Step 6
- Select your EK as the device

![image](https://github.com/user-attachments/assets/43b16b08-48fd-4226-82f5-91e82970d598)

## Step 7
- Enable the LDO, the boost should be defaulted to 3V and LDO should default to 800 mV

<img width="1065" height="509" alt="image" src="https://github.com/user-attachments/assets/f63eadfb-3da6-4c66-91dd-5eada0fa59b4" />


## Step 8
As you change the configurations of these regulators, you should see those changes reflected in the terminal from the nRF54's application!
```
[00:01:07.015,270] <inf> main: CH0: 3072 mV
[00:01:07.015,280] <inf> main: CH1: 808 mV
[00:01:08.015,442] <inf> main: CH0: 3009 mV
[00:01:08.015,447] <inf> main: CH1: 805 mV
```

## Move to the ipc branch for the next set of instructions: [➡️LINK](https://github.com/droidecahedron/nrf_peripheral_dmm/tree/ipc)
