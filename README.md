# AmazeUI Documentation

## Overview

AmazeUI is a C++ UI framework for Windows desktop applications, built on DirectX 12 and DirectXTK. It provides comprehensive UI controls, animation effects, and rendering capabilities, focusing on performance and visual effects.

### Key Features
- Custom UI controls (buttons, labels, text boxes, grids, charts, etc.)
- Animation system for interactive effects
- 2D and 3D rendering powered by DirectX 12
- Win32-style message-driven event handling
- FreeType-based text rendering
- Image and texture resource management

### Architecture
- **Common.h/cpp**: Provides core utilities and framework environment definitions, including clipboard operations and basic tools.
- **UIUtility.h/cpp**: Contains helper functions and utilities for UI-related operations, such as layout calculations and resource management.
- **UIDXFoundation.h/cpp**: Implements foundational DirectX integration, including rendering setup and resource management.
- **UIElement.h/cpp**: Defines the base class for UI elements, including properties like position, size, and rendering logic.
- **UIWindow.h/cpp**: Manages window creation, event handling, and rendering for the application.
- **UIWidget.h/cpp**: Implements specific UI controls and widgets, such as buttons, sliders, and text boxes.
- **UIAnimation.h/cpp**: Provides animation management, including adding, updating, and removing animations for UI elements.
- **UIApplication.h/cpp**: Serves as the entry point for the application, managing the main loop, initialization, and shutdown processes.

## Installation and Usage

### Requirements
- Windows 10 or higher
- Visual Studio 2019 or higher
- DirectX 12 SDK

### Installation Steps
1. Clone the repository:
   ```bash
   git clone https://github.com/ooping/AmazeUI.git
   ```
2. Open the solution file `DirectXTK12_Demo.sln` in Visual Studio.
3. Build and run the project.

### Demos
- **DEMO - 2D**: Demonstrates 2D rendering capabilities, including shapes, images, and animations.
  <img width="1024" alt="69e24a779be683e9aaf904de60d2831" src="https://github.com/user-attachments/assets/8bc4e152-74c1-46bb-a6e5-2a4a954b2612" />
- **DEMO - 3D**: Showcases 3D rendering features, such as models, transformations, and lighting.
  <img width="1024" alt="6ba8e79762e8ee66a0c2191b8554cfe" src="https://github.com/user-attachments/assets/a0146fdb-b60d-413b-be12-c0a4250b3fa8" />
- **DEMO - Controls**: Highlights various UI controls, including buttons, sliders, and text inputs.
  <img width="1024" alt="68ddeef0985bebe2ab9275d2eb060e8" src="https://github.com/user-attachments/assets/08c968e4-3aca-4b2e-9230-aacff0d868db" />


## Support the Project

If you like this project, you can support its development by donating:
- [Donate via PayPal](https://www.paypal.com/paypalme/ooping)
- Donate via Alipay or WeChat Pay:

| <img src="https://github.com/user-attachments/assets/72ef1af3-d035-4dd8-8e91-3fcde638be6c" alt="Alipay QR Code" width="300" style="margin-right: 20px;"> | <img src="https://github.com/user-attachments/assets/57325080-fe6b-4c0f-b106-6fd2960c6a44" alt="WeChat QR Code" width="300"> |
|:--:|:--:|


## Contributing

Contributions are welcome! Please follow these steps:
1. Fork the repository.
2. Create a new branch and make your changes.
3. Submit a pull request.

## License

This project is licensed under the [MIT License](LICENSE).

## Contact

- Author: OOPING
- Email: yancheng.huang@outlook.com
