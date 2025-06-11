#   Examples like SquareLine or AI to generate code
use Squareline Studio to design Guis by drag or drop
Or require AI create UIs .

# SquareLine Studio

![image](https://github.com/user-attachments/assets/965c936c-f339-41e3-a139-d10188429444)

1.   Open tool , choose examples, export to ui, pass in project
     
      ![image](https://github.com/user-attachments/assets/d216c4d5-27e9-4f99-aaf5-e3e5f0f01baf)


      ![image](https://github.com/user-attachments/assets/9df097df-24c6-40cc-be3b-9510dddf881d)
      

2.   instead of CMakeLists of ui .
   
       ![image](https://github.com/user-attachments/assets/f3f09219-e225-4f55-aa43-cd93bfb0aab3)

3.   add_sub and add link_library of CMakeLists in project.

      ![image](https://github.com/user-attachments/assets/da3d8dd4-54de-43a6-81c6-d0220d33eb06)

     ![image](https://github.com/user-attachments/assets/3416a9c8-377c-4707-8e4c-b735cf7873d8)


7.   declare #include ui.h in main, then run by add ui_init() in main.

      ![image](https://github.com/user-attachments/assets/80fb9f6c-1f2d-4a3e-a674-9643ca62469a)

     ![image](https://github.com/user-attachments/assets/68e5ef63-e9e4-4c5a-bd37-7c49928ecd10)


9.   compile, connect, drag file.uf2 .

# Require Ai generating code

1.   **To simple code**

     https://m365.cloud.microsoft/chat/?fromcode=edgentp&redirectid=8e8a6704ce4542ecac8eead7584833b7&auth=2&internalredirect=CCM&login_hint=truongthanh%40minhanhthu.edu.vn

     ![image](https://github.com/user-attachments/assets/7c586c20-8094-489a-8fb4-10973393abbc)

   
      Open copilot  then chat "only lvgl create app guis" , copy code then modify few code to complete
     
2.   **To advanced code**

     research https://aistudio.google.com/app/prompts?state=%7B%22ids%22:%5B%221UTOqJ_T2ycxENLGAP6FTSPYVNYVojrpt%22%5D,%22action%22:%22open%22,%22userId%22:%22104288789476001819042%22,%22resourceKeys%22:%7B%7D%7D&usp=sharing

     ![image](https://github.com/user-attachments/assets/f44b678d-0263-415c-b98d-e5f56a174ecc)

   
      Open AI STUDIO GOOGLE or other AI that you beleave.
      chat "only lvgl create app guis".

3.   **chat "guide link to main or project"**
   
      if you clean things,only create file.h then include in main call functions to run.
