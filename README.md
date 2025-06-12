<p align="center">
  <img src="https://github.com/user-attachments/assets/f9970b40-8853-4226-a039-70d478c86104">
</p>


# Required Materials
## Hardware
- nRF54L15 DK
  
  <img src="https://github.com/user-attachments/assets/a302e826-4cca-405f-9982-6da59a2ac740" width=15% height=15% >
- nPM2100 EK

   <img src="https://github.com/user-attachments/assets/b0fb8940-37a9-4051-812e-5c0ff0203e20" width=15% height=15% >
- Nordic multi-USB cable x 2

  <img src="https://github.com/user-attachments/assets/2121c4c3-cc36-45a3-8c62-b9a9f24ef1bd" width=10% height=10%>
- AA Battery (other types work as well) 



- Laptop 💻 setup with software ahead of time
## Software
### Installing and setting up nRF Connect SDK NCS) 🔗[LINK](https://academy.nordicsemi.com/courses/nrf-connect-sdk-fundamentals/lessons/lesson-1-nrf-connect-sdk-introduction/topic/exercise-1-1/)
**These are large downloads and take a long time. Please complete before the workshop.**
### Extra tools
- nRF Connect for Desktop ([Link](https://www.nordicsemi.com/Products/Development-tools/nRF-Connect-for-Desktop/Download#infotabs))
      - Install Serial Terminal app
      - Install nPM PowerUP
      - Install Board Configurator
- nRF Connect for Mobile ([Link](https://www.nordicsemi.com/Products/Development-tools/nRF-Connect-for-mobile)) for iOS/Android

# Workshop Outline
  - Intro Slides / chalk talk
    - Intro to nPM2100 ([Link](https://www.nordicsemi.com/Products/nPM2100))

    ![image](https://github.com/user-attachments/assets/61a9bba4-a6a8-4c0e-b408-96652dedbf6e) ![image](https://github.com/user-attachments/assets/e9c93a15-8e9c-4d44-9e95-fb33623bb81b)



    - Intro to nRF54L15 ([Link](https://www.nordicsemi.com/Products/nRF54L15))

      ![image](https://github.com/user-attachments/assets/cd4bdc06-cc35-4a3f-a709-0603bdeb2a2e) ![image](https://github.com/user-attachments/assets/55432e64-a309-4714-852c-e00bbddfbf54)


    - Intro to nRF Connect for Desktop
    - Intro to VS Code and Nordic VSC Extensions
    - Hands on
   
# Hands on
## Goal and Progresion Path
There are a few branches in this repo, here is the intended progression path for you as you walk through this workshop.
```mermaid
graph LR;
  main*-->adc;
  adc-->npm_powerup;
  npm_powerup-->ipc;
  ipc-->ble;
```
> `*` == your current location

At a high level, we will write an application for the nRF54L15 SoC to read the analog output of each regulator on the nPM2100, then pipe that data via a BLE connection to our mobile devices. (So a little BLE Multimeter)

This workshop assumes you've at least completed the first two lessons of the nRF Connect SDK Fundamentals in the Nordic DevAcademy.
If you haven't, here is a link, but expect to be left behind! [🔗LINK](https://academy.nordicsemi.com/courses/nrf-connect-sdk-fundamentals/)

## Add the application to VSCode:
- Clone this repo
- Click on the nRF Connect icon in the left hand ribbon
- Click on Open an existing application
- Select the directory of this repository
- Click Open:<br><img src="https://github.com/user-attachments/assets/4777afa9-32f5-4167-940f-13b3a8900b4d" width="500"/>


## Under Applications at the bottom of the left pane 
- Click on Add build configuration 
- Select `nrf54l15dk/nrf54l15/cpuapp` as the board target
- Click Build Configuration on the bottom right<br><img src="https://github.com/user-attachments/assets/059d9f1f-0baf-45f3-8678-daca7a731e24" width="500"/>

## Set up the 54L15DK
- Plug in your 54L15DK
- Open nRF Connect for Desktop, and open the board configurator, and configure the 54L15DK to be 3V3 by changing the VDD option and selecting "Write config"
![image](https://github.com/user-attachments/assets/ddc51fd1-9996-4999-b422-17226a7ace9b)
  
## Move to the adc branch for the next set of instructions: [➡️LINK](https://github.com/droidecahedron/Teardown-2025/tree/adc)
