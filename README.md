# Basic Login System

Built with:
- Backend: C with OpenSSL and Mongoose
- Frontend: HTML, JavaScript, and TailwindCSS
- Storage: JSON file-based

⚠️ **WARNING: This is a demonstration project only!** 
This implementation is not secure enough for production use. It lacks several critical security features like:
- Session management
- SQL injection protection
- XSS protection
- CSRF protection

## Requirements

### Backend
- GCC compiler
- OpenSSL development libraries
- pkg-config
- Make

### Frontend
- Modern web browser
- Python3 (for development server)

## Installation

1. Install dependencies (Ubuntu/Debian):
```bash
sudo apt update
sudo apt install gcc make pkg-config libssl-dev
```

2. Build the backend:
```bash
cd backend
make clean && make
```

3. Run the backend:
```bash
./server
```

4. Start the frontend (in a new terminal):
```bash
cd frontend
python3 -m http.server 3000
```

5. Access the application:
```
http://localhost:3000
```

## Project Structure

```
fullstack-login/
├── backend/
│   ├── include/
│   │   └── mongoose.h
│   ├── src/
│   │   ├── main.c
│   │   ├── server.c
│   │   ├── server.h
│   │   └── mongoose.c
│   ├── Makefile
│   └── users.json
└── frontend/
    └── index.html
```

## Features
- User registration
- Login authentication
- Password hashing (SHA-256)
- JSON file-based storage
- Simple frontend interface

## Development Notes

- The server runs on port 8080
- Frontend development server runs on port 3000
- Passwords are hashed using SHA-256 (not salted)
- User data is stored in users.json

## Security Considerations

This project is for learning purposes only and has several security issues:
1. Passwords are stored using basic hashing without salting
2. No protection against brute force attacks
3. No secure session management
4. Basic CORS configuration
5. No input validation
6. Plain HTTP (no HTTPS)
7. File-based storage without proper access control

## License

This project uses:
- Mongoose (GPLv2)
- OpenSSL (Apache-2.0)

## Contributing

This is a learning project. Feel free to fork and experiment, but remember this is not suitable for production use.
