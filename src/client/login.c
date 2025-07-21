#include <fcntl.h>
#include <errno.h>
#include "login.h"

int get_login_file() {
	
	int login_fd = open("user_login.json", O_EXCL|O_CREAT|O_RDWR, 0666);

	if (login_fd == -1) {
		
		if (errno == EEXIST) {
			return LOGIN_FILE_DOES_NOT_EXIST;
		}

		return LOGIN_ERROR;
	}

	return login_fd;
}

int login_handler() {
	
	int login_fd = get_login_file();

	if (login_fd == LOGIN_ERROR)
		return LOGIN_ERROR;

	if (login_fd == LOGIN_FILE_DOES_NOT_EXIST) {

		// Registra
	} else {

		// Login automatico
	}
}
