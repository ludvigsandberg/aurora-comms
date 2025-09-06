# ChatApp

**ChatApp** is a high-performance, lightweight multi-client chat server written in C99. It demonstrates advanced low-level C programming, type-safe and type-generic data structures, CMake-based build systems, and modern containerization with CI/CD workflows. All core data structures—dynamic arrays, hash maps, and more—are implemented from scratch using robust techniques to work around C99 limitations, ensuring type safety and maintainability.

## Features
- **Multi-client TCP chatroom** — Connect via Netcat (nc) or Telnet.
- **Dockerized** — Build, deploy, and run anywhere with minimal setup.
- **CI/CD Ready** — Automated builds and tests ensure reliable development.
- **Custom type-safe**, type-generic data structures — Elegant C99 implementations of dynamic arrays, hash maps, and more without external libraries.
- **Minimal dependencies** — Portable C99 code suitable for various platforms.
- **Unit-tested** — Comprehensive Ceedling tests verify correctness and maintainable code.
- **Clean, modular architecture** — Networking and application logic are fully separated, enabling maintainability, scalability, and straightforward extension of functionality.

## Architecture
**ChatApp** is organized into the following layers:
1. **Networking Layer** — Handles TCP connections, client sessions, and message broadcasting.
2. **Data Structures Layer** — Provides type-safe dynamic arrays and hash maps, fully generic across types.
3. **Application Logic** — Manages chatroom behavior, client state, and message routing.
4. **Build & Deployment Layer** — CMake for builds, Docker for multi-stage images (release, debug, test), and CI/CD integration.

Clear separation of concerns ensures that networking, application logic, and data structures are isolated, making the codebase highly maintainable and extensible.

## Usage
### 1. 🛠️ Build the server:
#### Release
```sh
docker build --target release -t chat-app:release .
```
#### Debug
```sh
docker build --build-arg BUILD_TYPE=Debug --target debug -t chat-app:debug .
```
#### Tests
```sh
docker build --target test -t chat-app:test .
```

---

### 2. 🚀 Run the server:
#### Release
```sh
docker run --rm -it -p 2000:2000 chat-app:release
```
#### Debug
```sh
docker run --rm -it --cap-add=SYS_PTRACE --security-opt seccomp=unconfined -p 2000:2000 chat-app:debug
```
#### Tests
```sh
docker run --rm chat-app:test
```

---

### 3. Connect with Netcat or telnet:
Netcat:
```sh
nc 127.0.0.1 2000
```
Telnet:
```sh
telnet 127.0.0.1 2000
```

## CI/CD & Testing
- **Unit tests** — Ceedling-based tests validate key data structures and selected networking functionality.
- **Continuous Integration** — Dockerized builds and available tests can be integrated into CI pipelines for automated checks.
- **Debugging support** — Debug builds include GDB for in-container debugging.

## Configuration
- **TCP Port** — Defaults to 2000 but can be customized at runtime by providing a command-line argument when starting the server.
- **Build Types** — Release for production-ready deployments and optimized performance, Debug for development, testing, and in-container debugging.

## License
MIT License — free to use and modify.
