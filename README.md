# webserv

Here is a comprehensive and structured `README.md` template in English for your 42 webserv project. You can copy and paste this into your repository and adjust the placeholders (like your name or specific implementation details) as needed.

---

# webserv

> *This is when you finally understand why a URL starts with HTTP.*

**webserv** is a custom HTTP/1.1 server written from scratch in C++98. This project is part of the core curriculum at [42 School](https://42.fr/). The goal is to build a fully functional, non-blocking HTTP server inspired by NGINX, capable of handling multiple connections, parsing configuration files, serving static websites, and executing CGI scripts.

---

## 🚀 Features

* **HTTP/1.1 Compliance:** Supports `GET`, `POST`, and `DELETE` methods.
* **Non-blocking I/O & Multiplexing:** Utilizes a single-threaded architecture with multiplexing (`poll` / `epoll` / `select` / `kqueue` — *edit based on your implementation*) to handle multiple concurrent clients without hanging.
* **NGINX-like Configuration:** Uses a custom configuration file format to define server behavior, routes, and error pages.
* **CGI Support:** Can execute Common Gateway Interface (CGI) scripts (e.g., PHP, Python) based on file extensions.
* **Multiple Servers & Ports:** Can host multiple virtual servers on different ports or the same port with different server names (Host routing).
* **Static File Serving:** Serves HTML, CSS, JavaScript, images, and other static assets.
* **Directory Listing:** Generates automatic directory indexes if no index file is provided and autoindex is enabled.
* **Upload Handling:** Supports file uploads via multipart/form-data or direct body payloads.
* **Resilience:** High availability and stress-tested to ensure it doesn't crash under load or malformed requests.

---

## 🛠️ Prerequisites

To compile and run this project, you will need:

* `make`
* A C++98 compliant compiler (e.g., `c++`, `g++`, or `clang++`)
* Unix-based OS (Linux or macOS)

---

## ⚙️ Installation & Setup

1. **Clone the repository:**
```bash
git clone https://github.com/yourusername/webserv.git
cd webserv

```


2. **Compile the server:**
```bash
make

```


*This will generate the `webserv` executable.*
3. **Run the server:**
```bash
./webserv config.conf

```

---

## 🧪 Testing

To test the server, you can use a standard web browser by navigating to `http://localhost:8080`, or use command-line tools like `curl`:

**Test a simple GET request:**

```bash
curl -v http://localhost:8080/

```

**Test a POST request:**

```bash
curl -v -X POST -d "Hello World" http://localhost:8080/

```

We also recommend using stress-testing tools like `siege` to verify the server's stability under heavy load:

```bash
siege -b -c 100 -t 1M http://localhost:8080/

```

---

## 📚 Architecture Overview

The core loop of the server relies on I/O multiplexing. Instead of spawning a new thread or process for every incoming connection (which is resource-heavy), **webserv** uses a single thread to monitor all active sockets.

1. **Parse Configuration:** Reads and validates the `.conf` file to set up server sockets.
2. **Setup Sockets:** Binds and listens on the specified ports.
3. **Event Loop:** Uses `poll()` (or equivalent) to wait for read/write events on server and client sockets.
4. **Request Parsing:** Reads incoming data, parses HTTP headers, and constructs a Request object.
5. **Response Generation:** Based on the requested URI, method, and config rules, it either fetches a file, triggers a CGI script, or generates an error page.
6. **Sending:** Writes the formatted HTTP response back to the client socket.


---

*Project completed as part of the 42 School curriculum.*
