# HollerTP (SMTP Protocol)

## Project Description

**HollerTP** is an SMTP (Simple Mail Transfer Protocol) mail service implemented in C. The project aims to provide a lightweight, efficient, and fully functional SMTP server that can handle basic email sending operations. Designed to be fast and secure, hollertp serves as a fundamental tool for understanding the core principles of email transmission over the internet.

## Goals

- **Learn and Implement SMTP Protocol:** Understand the SMTP protocol in depth and implement it from scratch using the C programming language.
- **Create a Lightweight SMTP Server:** Develop a simple yet powerful SMTP server that can send emails efficiently.
- **Enhance Security:** Incorporate basic security measures, such as input validation and proper error handling, to prevent common vulnerabilities.
- **Optimize for Performance:** Ensure the server is optimized for performance, capable of handling multiple email requests concurrently.

## Specifications

- **Protocol:** SMTP (RFC 5321)
- **Programming Language:** C
- **Concurrency:** The server will handle multiple connections using multi-threading or select/poll-based multiplexing.
- **Security:** Basic input validation, error handling, and potential TLS/SSL support (optional).
- **Supported Platforms:** Unix-based systems (Linux, macOS), with potential Windows support.

## Prerequisites
- **GCC Compiler:** Ensure that GCC is installed on your system.
- **PowerShell:** Use PowerShell as an administrator to run some commands, particularly on Windows.
- **Administrator Privileges:** Required to bind the server to port 25.

## Design

### Architecture

1. **Connection Handling:**
   - The server listens on a designated port (typically port 25 for SMTP) and accepts incoming client connections.
   - Each connection is handled in a separate thread or using an event-driven approach (e.g., `select`, `poll`).

2. **SMTP Commands Implementation:**
   - The server processes standard SMTP commands such as HELO/EHLO, MAIL FROM, RCPT TO, DATA, QUIT, etc.
   - The server responds with appropriate status codes based on the SMTP protocol.

3. **Email Data Processing:**
   - The server receives email data and processes it according to SMTP rules.
   - It will parse headers, handle message bodies, and prepare the email for delivery.

4. **Security Measures:**
   - Basic input validation to prevent command injection.
   - Proper error handling to avoid buffer overflows and other common C vulnerabilities.
   - Optional TLS/SSL support for secure email transmission.

5. **Logging:**
   - Implement a logging mechanism to track server activity, errors, and connection details.

6. **Future Enhancements (Optional):**
   - Add TLS/SSL support for encrypted communication.
   - Implement an email queuing system for delayed delivery.
   - Develop a web-based dashboard for monitoring server activity.
   
## Current Status

- The server currently supports basic SMTP commands: HELO/EHLO, MAIL FROM, RCPT TO, DATA, and QUIT.
- The server currently handles connections sequentially in a single-threaded manner. Multi-threading or event-driven concurrency is planned for future updates.
- Basic input validation is in place. More advanced security features, including comprehensive error handling and potential TLS/SSL support, are being tried to implement.
- This application is currently Windows supported as I crashed my linux 3 days ago for running a command that was intended to remove a file but... well.
- This now logs every single command with the date and time and records all transfers of mails.
- This is only a simulator that simulates how an SMTP server works.



## How to Build and Run

1. **Clone the Repository:**
   ```bash
   git clone https://github.com/hollermay/hollertp.git
   cd hollertp

2. **Build the Project:**
   To compile the SMTP server code, use GCC. Run this command in PowerShell with administrator privileges:
   ```bash
   gcc smtp.c -o smtp.exe -lws2_32
   ```
   This command compiles the smtp.c file into an executable named smtp.exe.
   
3. **Run the SMTP Server:**
   After building the project, run the server executable:
   ``` bash
   .\smtp
   ```
   Make sure to run this command as an administrator, as the server needs appropriate permissions to bind to *port 25*.
   *NOTE:If this doesn't work reveal the executable file in explorer then run it as administrator.*
   

5. **Test the SMTP Server:**
   You can test the server using an email client or command-line tools like telnet or netcat.
   ``` bash
   telnet localhost 25
   ```
   You can manually input SMTP commands like **HELO**, **MAIL FROM**, **RCPT TO**, **DATA**, and **QUIT** to interact with the server..

     

## License

This project is licensed under the MIT License. See the [LICENSE](./LICENSE) file for details.

## Contributions

Contributions are welcome! Feel free to fork the repository, create a branch, and submit a pull request.

## References
Below are the references I used while building the project.

https://blog.thelifeofkenneth.com/2012/06/minimalist-c-smtp-mail-server.html
https://www.cloudflare.com/learning/email-security/what-is-smtp/

