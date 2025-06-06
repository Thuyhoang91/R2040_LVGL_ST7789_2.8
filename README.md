# R2040_LVGL_ST7789_2.8
use vscode to code for rp2040 pi pico connect with st7789 2.8 , supported LVGL to create graphics 
## Table of Contents

*   [Installation](#installation)
*   [Usage](#usage)

## Installation

1.  **Prerequisites:**
    *   vscode
![image](https://github.com/user-attachments/assets/f107d452-1839-4e5e-b623-cc506b882c58)   
    *   Raspberry Pi Pico Visual Studio Code extension

2.  **Clone the repository:**

    ```bash
    dowload lvgl 8.3 https://github.com/lvgl/lvgl/archive/refs/heads/release/v8.3.zip
    ```

3.  **Install dependencies:**

    ```bash
    Click Rasberry Pi
    ![image](https://github.com/user-attachments/assets/1df66fe9-5f8b-40c9-a96f-775ba534d5f6)
    Create new project, done it. then it will require download GCC toolchain and pico-sdk
    ![image](https://github.com/user-attachments/assets/28ef5f70-0a90-4e76-b62c-d51f9c40a557)
    Wait auto downloading and install GCC toolchain and pico-SDK.
    ```
## Usage

To run the project:

1.  **Copy**

    ![image](https://github.com/user-attachments/assets/1de91938-ec12-436f-8da5-2c3c70f4f56f)

    copy lvgl, src, inc , lv_conf.h in project, if use st7789 2.8
    else using AI STUDIO Google to guide create drivers (ili9341 .etc...) for it. ( can use OTHER AI but not effect)
3.  **Research**
   
    research https://aistudio.google.com/app/prompts?state=%7B%22ids%22:%5B%2212igXiULXN-gZ8URQ2F0RWzwiHA5sUaya%22%5D,%22action%22:%22open%22,%22userId%22:%22104288789476001819042%22,%22resourceKeys%22:%7B%7D%7D&usp=sharing

4.  **CmakeLists**

    Add LVGL library
    ![image](https://github.com/user-attachments/assets/8fb47bf9-dc80-4917-8dde-c728c4dedb41)
    Add executable
    ![image](https://github.com/user-attachments/assets/7f263be9-1489-4f00-9bd8-114dfb9a9977)
    Enable output over USB to serial monitor
    ![image](https://github.com/user-attachments/assets/4cf2ed9f-30fc-4957-8510-622b9f098558)
    Add the standard library to the build
    ![image](https://github.com/user-attachments/assets/980e8538-982a-4ac1-ac21-f47a3f5b1733)
    Add  include
    ![image](https://github.com/user-attachments/assets/8ae7db55-cdd0-4c00-ba5a-ebf2ca325e98)

5.  **Compile project**

    click Extend Rasberry Pi to compile
    ![image](https://github.com/user-attachments/assets/d7c1b98a-3509-4f7b-a506-43effec971fe)

6.  **Flash**

    Connect Board to CPU , then hold BOOTSEL button , next push RESET button.
    Copy file.uf2 in build to new director
    ![image](https://github.com/user-attachments/assets/d1c2faca-0798-4ec8-8cc7-22fa1ce56ccc)

    ![image](https://github.com/user-attachments/assets/305f5823-311e-4638-9ea3-5b59897248ec)

    to flash or debug online , guide follow https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf
    read chatter 5 .

Thanks all to watched 
 
    






    
