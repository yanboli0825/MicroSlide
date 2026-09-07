# MicroSlide

MicroSlide is a Windows desktop application for digital pathology slide scanning and analysis. It is built around a layered Qt-based architecture that connects device control, image acquisition, quality screening, AI inference, and case management in one workflow. The UI coordinates microscope operation, scanner input, and result review, while background modules handle image buffering, preprocessing, and model execution.

The software uses a multi-threaded pipeline to keep acquisition and analysis responsive. Microscope frames are filtered and queued, worker threads perform patch extraction and ONNX inference, and the results are merged into image-level and video-level diagnosis output. For supported cases, an additional MMR branch runs on selected features. This design lets the UI stay interactive while the heavy processing happens in the background.

At the center of the system is an ONNX inference pipeline. MicroSlide extracts useful image patches from slide frames, converts them into feature embeddings, and runs the appropriate diagnosis model based on the slide source. The pipeline also supports video-level aggregation and, for supported gut cases, additional MMR prediction. This keeps the diagnostic flow consistent while still allowing different tissue types to use different models.

The application is designed for practical slide-work operations. It supports live microscope preview, capture filtering, saving diagnostic images and metadata, optional database upload, and simple case-level confirmation by the operator. System settings such as thresholds, save paths, and database connection details can be adjusted from the built-in configuration window.

## Features

- Microscope live preview with exposure, white balance, and magnification controls
- Scanner-based PID capture to start a new case quickly
- Image filtering by sharpness, similarity, and effective area
- AI inference for slide-level and patch-level diagnosis
- Optional MMR prediction for supported gut cases
- Case image saving, JSON feature export, and database upload support
- Built-in system configuration and file test tools

## Project Notes

- Built with C++17, Qt 6, OpenCV, ONNX Runtime, and several device SDKs
- Main entry point: `src/main.cpp`
- Runtime settings are stored in `assets/config/config.ini`
