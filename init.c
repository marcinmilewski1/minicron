/*
 * init.c
 *
 *  Created on: 13 Nov 2014
 *      Author: marcin
 */

#include "parser.h"
#include "utils.h"

void skeleton_daemon() {
	 	 /* Our process ID and Session ID */
	        pid_t pid, sid;

	        /* Fork off the parent process */
	        pid = fork();
	        if (pid < 0) {
	                exit(EXIT_FAILURE);
	        }
	        /* If we got a good PID, then
	           we can exit the parent process. */
	        if (pid > 0) {
	                exit(EXIT_SUCCESS);
	        }

	        /* Change the file mode mask */
	       // umask(0);

	        /* Open any logs here */
	        openlog ("minicron", LOG_PID, LOG_DAEMON);

	        /* Create a new SID for the child process */
	        sid = setsid();
	        if (sid < 0) {
	        	syslog(LOG_ERR, "Cannot set session id.");
	        	exit(EXIT_FAILURE);
	        }


	        /* Close out the standard file descriptors*/
	        // NARAZIE NIE ZAMYKAC !
			//close(STDIN_FILENO);
	        //close(STDOUT_FILENO);
	        //close(STDERR_FILENO);

}
