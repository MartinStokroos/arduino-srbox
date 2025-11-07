/*
 * File: srbox-tester.c
 * 
 * Serial Response Box Tester
 *
 * Compile with: $ gcc -o srbox-tester srbox-tester.c
 *
 * Use with PST srbox: $ ./srbox-tester /dev/ttyS0
 * Use with Arduino srbox: $ ./srbox-tester /dev/ttyACM0
 *
 * This program first blinks all the lamps once, and then it shows the pressed button codes.
 * Terminate by pressing Enter or Ctrl-C.
 * 
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s /dev/ttyS0\n", argv[0]);
        return 1;
    }

    int zero_cnt = 0;
    const char *portname = argv[1];
    int fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        perror("Error opening serial port");
        return 1;
    }

    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        perror("Error from tcgetattr");
        close(fd);
        return 1;
    }

    // Configure serial port: 19200 8N1, no flow control
    cfsetospeed(&tty, B19200);
    cfsetispeed(&tty, B19200);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit chars
    tty.c_iflag &= ~IGNBRK;                     // disable break processing
    tty.c_lflag = 0;                            // no signaling chars, no echo
    tty.c_oflag = 0;                            // no remapping, no delays
    tty.c_cc[VMIN]  = 1;                        // read blocks for at least 1 char
    tty.c_cc[VTIME] = 0;                        // no read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);     // shut off xon/xoff ctrl
    tty.c_cflag |= (CLOCAL | CREAD);            // ignore modem controls, enable reading
    tty.c_cflag &= ~(PARENB | PARODD);          // no parity
    tty.c_cflag &= ~CSTOPB;                     // one stop bit
    tty.c_cflag &= ~CRTSCTS;                    // no RTS/CTS flow control

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("Error from tcsetattr");
        close(fd);
        return 1;
    }

    printf("Opening %s at 19200 8N1...\n", portname);
    fflush(stdout);

    // --- Flash lamps before reading ---
    unsigned char lamp_addr = 0x60;
    unsigned char lamp_byte;
    
    lamp_byte = lamp_addr;
    if (write(fd, &lamp_byte, 1) != 1) {
      	perror("Error sending lamp byte");    
    }

    for (int n=0; n<=5; n++)
    {
    	lamp_byte = lamp_addr + (1 << n);
    	sleep(1);
    	if (write(fd, &lamp_byte, 1) != 1) {
        	perror("Error sending lamp byte");
    	}
    }
    
    lamp_byte = lamp_addr;
    if (write(fd, &lamp_byte, 1) != 1) {
      	perror("Error sending lamp byte");    
    }
    
    // Set stdin to non-blocking for keypress detection
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    printf("Listening... Press any key to exit.\n");
    fflush(stdout);

    // --- Send 0xA0 before reading ---
    unsigned char start_byte = 0xA0;
    if (write(fd, &start_byte, 1) != 1) {
        perror("Error sending start byte (0xA0)");
    } else {
        printf("Sent start byte: A0\n");
    }

    unsigned char buf[1];
    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        FD_SET(STDIN_FILENO, &readfds);
        int maxfd = (fd > STDIN_FILENO ? fd : STDIN_FILENO) + 1;

        int rv = select(maxfd, &readfds, NULL, NULL, NULL);
        if (rv < 0) {
            perror("select()");
            break;
        }

        // Check for keypress
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            break; // exit loop
        }

        // Read serial data
        if (FD_ISSET(fd, &readfds)) {
            int n = read(fd, buf, 1);
            if (buf[0] == 0) {
              zero_cnt++;
              if (zero_cnt > 100) zero_cnt = 100;
            }
            else {  
              zero_cnt = 0;
            }
            if (n > 0) {
                if (zero_cnt <= 10) {
                  printf("%02X\n\r", buf[0]);
                }
                fflush(stdout);
                
            }
        }
    }

    // --- Send 0x20 before exiting ---
    unsigned char end_byte = 0x20;
    if (write(fd, &end_byte, 1) != 1) {
        perror("Error sending end byte (0x20)");
    } else {
        printf("\nSent end byte: 20\n");
    }

    printf("Terminating.\n");
    close(fd);
    return 0;
}

