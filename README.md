# Spotify PHP Controller (WIP)

⚠️ **WARNING: This project is Work In Progress (WIP) and not considered stable.**

Simple PHP script to interact with Spotify API, intended for use with microcontrollers.

## Setup Instructions

1.  **Server Side**:
    *   Open `api.php`: Fill in `YOUR_CLIENT_ID` and `YOUR_CLIENT_SECRET`.
    *   Open `callback.php`: Fill in `YOUR_CLIENT_ID`, `YOUR_CLIENT_SECRET`, and `YOUR_REDIRECT_URI`.

2.  **Hardware Side (Arduino/ESP)**:
    *   Open `spotify/spotify.ino`.
    *   Fill in `YOUR_WIFI_SSID` and `YOUR_WIFI_PASSWORD`.
    *   Update `IPAddress server(...)` with the local IP of the machine running the PHP scripts.

3.  **Authorize**:
    *   Open `callback.php` in your browser to log in to Spotify.
