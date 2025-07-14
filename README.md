# nrf_peripheral_dmm
<p align="center">
  <br>
  <img width="45%" height="707" alt="image" src="https://github.com/user-attachments/assets/fb81cc89-0e76-4a56-83be-ce0aa05e4439" />
  </br>
  An application/workshop on leveraging an nRF54L15DK to be a Bluetooth Low Energy (BLE) peripheral to read the analog outputs of an nPM2100EK's regulators and viewing them on your mobile device.
</p>



# Prerequisites and Required Material
## Hardware
- nRF54L15 DK
 
  <img src="https://github.com/user-attachments/assets/a302e826-4cca-405f-9982-6da59a2ac740" width=15% >
- nPM2100 EK

   <img src="https://github.com/user-attachments/assets/b0fb8940-37a9-4051-812e-5c0ff0203e20" width=15% >
- Nordic multi-USB cable x 2

  <img src="https://github.com/user-attachments/assets/2121c4c3-cc36-45a3-8c62-b9a9f24ef1bd" width=10% >
- AA Battery (other types work as well) 

- 3x Female-Female jumper wires
> ⬆️ All above provided by Nordic, and you will keep upon completion of the workshop ⬆️

- Bring your own laptop 💻 **setup with software ahead of time**



## Software
> [!IMPORTANT]  
> You must complete the first lesson of the [Nordic DevAcademy](https://academy.nordicsemi.com/courses/nrf-connect-sdk-fundamentals/) for this workshop.
> 
> You must be able to build and be able to flash a blank application. If you do not have a DK, at the very least a successful build system is required.
> **These are large downloads and take a long time. Please complete before the workshop.**
 
### Installing and setting up nRF Connect SDK (NCS) 🔗[LINK](https://academy.nordicsemi.com/courses/nrf-connect-sdk-fundamentals/lessons/lesson-1-nrf-connect-sdk-introduction/topic/exercise-1-1/)

### Extra tools
- nRF Connect for Desktop ([Link](https://www.nordicsemi.com/Products/Development-tools/nRF-Connect-for-Desktop/Download#infotabs))

      - Install Serial Terminal app
  
      - Install nPM PowerUP
  
      - Install Board Configurator
- nRF Connect for Mobile ([Link](https://www.nordicsemi.com/Products/Development-tools/nRF-Connect-for-mobile)) for iOS/Android

# Workshop Outline
  - Intro Slides / chalk talk
    - Intro to [nPM2100](https://www.nordicsemi.com/Products/nPM2100)

    ![image](https://github.com/user-attachments/assets/61a9bba4-a6a8-4c0e-b408-96652dedbf6e) ![image](https://github.com/user-attachments/assets/e9c93a15-8e9c-4d44-9e95-fb33623bb81b)



    - Intro to [nRF54L15](https://www.nordicsemi.com/Products/nRF54L15)

      ![image](https://github.com/user-attachments/assets/cd4bdc06-cc35-4a3f-a709-0603bdeb2a2e) ![image](https://github.com/user-attachments/assets/55432e64-a309-4714-852c-e00bbddfbf54)


    - Intro to nRF Connect for Desktop
    - Intro to VS Code and Nordic VSC Extensions
    - Hands on
   
# Hands on
## Goal and Progression Path
There are a few branches in this repo, here is the intended progression path for you as you walk through this workshop.
```mermaid
graph LR;
  main*-->adc;
  adc-->npm_powerup;
  npm_powerup-->ipc;
  ipc-->ble;
```
> `*` == your current location

## High-level architecture
At a high level, we will write an application for the nRF54L15 SoC  (using two ADC channels on the nRF54L15, one per regulator) to read the analog output of each regulator on the nPM2100, then pipe that data via a BLE connection to our mobile devices. 

So, a little BLE peripheral Multimeter. Using nPM PowerUP, we can change the regulator outputs of the nPM2100 PMIC, and see the regulator voltage measurements change on our mobile device.

<img width="651" height="349" alt="image" src="https://github.com/user-attachments/assets/be2d369e-d812-4149-9516-e8eab22410f2" />


This workshop assumes you've at least completed the first lesson of the nRF Connect SDK Fundamentals in the Nordic DevAcademy.
If you haven't, here is a link, but expect to be left behind! [🔗LINK](https://academy.nordicsemi.com/courses/nrf-connect-sdk-fundamentals/)

## Add the application to VSCode:
- Clone this repo
- Click on the nRF Connect icon in the left hand ribbon
- Click on Open an existing application
- Select the directory of this repository
- Click Open:
<img width="929" height="394" alt="image" src="https://github.com/user-attachments/assets/3db707ad-9182-42a5-81ab-c2cc5a9897bd" />



## Under Applications at the bottom of the left pane 
- Click on Add build configuration 
- Select `nrf54l15dk/nrf54l15/cpuapp` as the board target
- Choose the _Browse_ option for Base Devicetree Overlays and select the `nrf54l15dk_nrf54l15_cpuapp.overlay` file in the `boards/` child directory of the repository.
- Click Build Configuration on the bottom right
<br>
  <img width="924" height="600" alt="image" src="https://github.com/user-attachments/assets/60b0d9fa-2bcd-41fd-9920-4fe24090221d" />
</br>



## Set up the 54L15DK
- Plug your 54L15DK into your machine.
- Open nRF Connect for Desktop, and open the board configurator, and configure the 54L15DK to be 3V3 by changing the VDD option and selecting "Write config"
![image](https://github.com/user-attachments/assets/ddc51fd1-9996-4999-b422-17226a7ace9b)
  
## Move to the adc branch for the next set of instructions: [➡️LINK](https://github.com/droidecahedron/nrf_peripheral_dmm/tree/adc)
