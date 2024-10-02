# cpp_server

A C++ project for creating a server that integrates with several key libraries such as WebSocketPP, MongoDB C++ Driver, cpp-jwt, RapidJSON, and Boost. This project sets up all necessary dependencies using vcpkg and requires MongoDB running on port `27017`.

## Prerequisites

- **MongoDB**: Make sure MongoDB is installed and running on port `27017` before starting the project.
  - You can check the MongoDB service with:
    ```bash
    sudo systemctl status mongod
    ```
    or start it with:
    ```bash
    sudo systemctl start mongod
    ```

- **vcpkg**: This project uses [vcpkg](https://github.com/microsoft/vcpkg) to manage dependencies. The setup scripts will automatically download and configure vcpkg if it's not already installed.

## Setup Instructions

### Windows

1. Clone the repository:
   ```bash
   git clone https://github.com/jahid-git/cpp_server.git
   cd cpp_server
   ```

2. Run the setup script:
   ```bash
   .\setup.bat
   ```

   This script will:
   - Clone and bootstrap `vcpkg` into a local `.vcpkg` folder.
   - Install the required libraries: `websocketpp`, `mongo-cxx-driver`, `cpp-jwt`, `rapidjson`, and `boost`.

### Linux / macOS

1. Clone the repository:
   ```bash
   git clone https://github.com/jahid-git/cpp_server.git
   cd cpp_server
   ```

2. Make the setup script executable:
   ```bash
   chmod +x setup.sh
   ```

3. Run the setup script:
   ```bash
   ./setup.sh
   ```

   This script will:
   - Clone and bootstrap `vcpkg` into a local `.vcpkg` folder.
   - Install the required libraries: `websocketpp`, `mongo-cxx-driver`, `cpp-jwt`, `rapidjson`, and `boost`.

## Libraries Used

- [WebSocketPP](https://github.com/zaphoyd/websocketpp): WebSocket client/server library for C++.
- [MongoDB C++ Driver](https://github.com/mongodb/mongo-cxx-driver): Official MongoDB driver for C++.
- [cpp-jwt](https://github.com/arun11299/cpp-jwt): JSON Web Token library for C++.
- [RapidJSON](https://github.com/Tencent/rapidjson): A fast JSON parser/generator for C++.
- [Boost](https://www.boost.org/): A collection of libraries for C++.


## MongoDB Connection

Ensure that MongoDB is running on the default port `27017` before starting the project. If MongoDB is running on a different port or host, you will need to update the connection settings in your C++ code.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Author

**Jahid Hasan**  
GitHub: [@jahid-git](https://github.com/jahid-git)
