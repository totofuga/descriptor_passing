#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

ssize_t write_fd ( int fd, int write_fd );
int myopen(char *path, int flag);
ssize_t read_fd ( int fd );

int main () {

	char buf[255];
	int n;

	int fd = myopen("test.c", O_RDONLY);

	while( ( n = read(fd, buf, 254) ) > 0 ) {
		buf[n] = 0;
		printf("%s", buf);
	} 

	return 0;
}

int myopen(char *path, int flag) {

	int sock_fd[2];
	pid_t childpid;
	char *str_socket_fileno;
	char *str_open_flag;
	int fd;
	int wait_state;
	int state;
	
	if(socketpair(PF_LOCAL, SOCK_STREAM, 0, sock_fd) < 0 ) {
		perror("");
		exit(1);
	}

	if ( (childpid = fork()) == 0 ) {
		close(sock_fd[0]);
		
		if( (fd = open(path, flag)) < 0 ) {
			perror("");
			exit(1);
		}

		if ( write_fd(sock_fd[1], fd) < 0 ) {
			perror("");
			exit(1);
		}
		exit(0);
	}

	close(sock_fd[1]);
	if ( waitpid(childpid, &state, 0) == -1 ) {
		perror("");
		exit(1);
	}
	
	if ( ! WIFEXITED(state) ||  WEXITSTATUS(state) != 0 ) {
		printf("child exit failure");
		exit(1);
	}

	fd = read_fd(sock_fd[0]);
	close(sock_fd[0]);

	return fd;
}

ssize_t write_fd ( int fd, int write_fd ) {
	struct msghdr msg;

	struct iovec iov[1];
	union {
		struct cmsghdr cm;
		char c[CMSG_SPACE(sizeof(int))];
	} u;

	struct cmsghdr *cmsptr;

	msg.msg_controllen = sizeof(u.c);
	msg.msg_control    = u.c;

	cmsptr = CMSG_FIRSTHDR(&msg);

	cmsptr->cmsg_level = SOL_SOCKET;
	cmsptr->cmsg_type  = SCM_RIGHTS;
	cmsptr->cmsg_len   = CMSG_LEN(sizeof(int));
	*((int*)CMSG_DATA(cmsptr)) = write_fd;

	msg.msg_name = NULL;
	msg.msg_namelen = 0;

	iov[0].iov_base = "";
	iov[0].iov_len  = 1;

	msg.msg_iov = iov;
	msg.msg_iovlen = 1;

	return sendmsg(fd, &msg, 0);
}

ssize_t read_fd ( int fd ) {
	struct msghdr msg;
	
	struct iovec iov[1];
	char c;
	int n;
	
	union {
		struct cmsghdr cm;
		char c[CMSG_SPACE(sizeof(int))];
	} u;

	struct cmsghdr *cmptr;

	msg.msg_controllen = sizeof(u.c);
	msg.msg_control    = u.c;

	msg.msg_name = NULL;
	msg.msg_namelen = 0;

	iov[0].iov_base = &c;
	iov[0].iov_len = 1;

	msg.msg_iov = iov;
	msg.msg_iovlen = 1;

	if ( (n = recvmsg(fd, &msg, 0)) < 0 ) {
		return n;
	}

	if ( (cmptr = CMSG_FIRSTHDR(&msg) ) != NULL &&
	      cmptr->cmsg_len == CMSG_LEN(sizeof(int))) {
		
		return *((int*)CMSG_DATA(cmptr));
	}

	return -1;
}
