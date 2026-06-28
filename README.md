# IMPORTANT NOTE:
this README.md is AI-generated, if any issues are found, please report them

Watering System
==================

Intelligent automatic plant watering system based on Raspberry Pi Pico.

**FastAPI** • **React + TypeScript** • **Raspberry Pi Pico**

Features
--------

*   Automatic watering based on soil moisture and schedules
*   Modern web dashboard with real-time telemetry
*   Historical data and interactive charts
*   Multi-device and group management
*   Full Docker support
*   Wireless communication with Pico devices

Project Structure
-----------------

watering/
├── app/          # FastAPI backend
├── front/        # React + TypeScript frontend
├── pico-client/  # Raspberry Pi Pico firmware
├── serv/         # C server for Pico
├── common/       # Shared components
└── tools/        # Utilities

Prerequisites
-------------

*   `picotool`
*   Python 3
*   Docker & Docker Compose (recommended)

Quick Start
-----------

git clone https://github.com/kornelu123/watering.git
cd watering

cd app
docker compose up --build -d

Development
-----------

### Frontend

cd front
npm install
npm run dev

### Backend

cd app
pip install -r requirements.txt
uvicorn main:app --reload

Flashing the Pico
-----------------

cd pico-client
./build.sh
./rpiflash.sh

* * *

Professional IoT plant watering solution.
Built with ❤️ for modern gardening.
